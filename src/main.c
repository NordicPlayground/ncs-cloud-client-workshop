/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>
#include <dk_buttons_and_leds.h>
#include <drivers/sensor.h>
#include <date_time.h>
#include "nrf_cloud_codec.h"
#include <event_manager.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(cloud_client, CONFIG_CLOUD_CLIENT_LOG_LEVEL);

#define MODULE main
#include <caf/events/module_state_event.h>

static struct cloud_backend *cloud_backend;
static struct k_work_delayable cloud_update_work;
static struct k_work_delayable connect_work;

static K_SEM_DEFINE(lte_connected, 0, 1);

static void read_temp_work_fn(struct k_work *work);
static K_WORK_DEFINE(read_temp_work, read_temp_work_fn);

static void read_temp_timer_fn(struct k_timer *timer)
{
	k_work_submit(&read_temp_work);
}
static K_TIMER_DEFINE(read_temp_timer, read_temp_timer_fn, NULL);

/* Flag to signify if the cloud client is connected or not connected to cloud,
 * used to abort/allow cloud publications.
 */
static bool cloud_connected;

static void connect_work_fn(struct k_work *work)
{
	int err;

	if (cloud_connected) {
		return;
	}

	err = cloud_connect(cloud_backend);
	if (err) {
		LOG_ERR("cloud_connect, error: %d", err);
	}

	LOG_INF("Next connection retry in %d seconds",
	       CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS);

	k_work_schedule(&connect_work,
		K_SECONDS(CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS));
}

static void cloud_update_work_fn(struct k_work *work)
{
	int err;

	if (!cloud_connected) {
		LOG_INF("Not connected to cloud, abort cloud publication");
		return;
	}

	LOG_INF("Publishing message: %s", log_strdup(CONFIG_CLOUD_MESSAGE));

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.buf = CONFIG_CLOUD_MESSAGE,
		.len = strlen(CONFIG_CLOUD_MESSAGE)
	};

	/* When using the nRF Cloud backend data is sent to the message topic.
	 * This is in order to visualize the data in the web UI terminal.
	 * For Azure IoT Hub and AWS IoT, messages are addressed directly to the
	 * device twin (Azure) or device shadow (AWS).
	 */
	if (strcmp(CONFIG_CLOUD_BACKEND, "NRF_CLOUD") == 0) {
		msg.endpoint.type = CLOUD_EP_MSG;
	} else {
		msg.endpoint.type = CLOUD_EP_STATE;
	}

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}

#if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
	k_work_schedule(&cloud_update_work,
			K_SECONDS(CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL));
#endif
}

// In order for the cloud to display temperature data in a graph it is necessary to send a device status message where the 
// temperature flag is set to 1 in the ui_info field of the device_status structure
static void set_device_status_work_fn(struct k_work *work)
{
	int err;
	static struct nrf_cloud_svc_info_ui ui_info = {.temperature = 1};
	static struct nrf_cloud_svc_info svc_info = {.ui = &ui_info, .fota = 0};
	static struct nrf_cloud_device_status dev_status = {.svc = &svc_info, .modem = 0};
	static struct nrf_cloud_data status_cloud_data;

	err = nrf_cloud_device_status_encode(&dev_status, &status_cloud_data, true);
	if(err) {
		LOG_ERR("Error generating cloud device status message: %i", err);
		return;
	}

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.buf = (char *)status_cloud_data.ptr,
		.len = status_cloud_data.len,
		.endpoint.type = CLOUD_EP_STATE
	};	

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}
}

K_WORK_DEFINE(set_device_status_work, set_device_status_work_fn);

// This function expects a cloud message on the format {"TYPE":"VALUE"}, where TYPE and VALUE are strings
// If TYPE mathces target_type_str, and VALUE matches target_value_str, the function returns true
bool decode_cloud_message(const struct cloud_msg *message, const uint8_t *target_type_str, const uint8_t *target_value_str)
{
	static uint8_t type_string[64];
	static uint8_t value_string[64];
	int type_index = 0, value_index = 0, delimiter_counter = 0;

	// Go through the cloud message looking for the " delimiters, and moving the TYPE and VALUE string into temporary variables
	for(int i = 0; i < message->len; i++) {
		if(message->buf[i] == '\"') delimiter_counter++;
		else {
			switch(delimiter_counter) {
				case 0: break; // Do nothing, still waiting for the first delimiter
				case 1:
					type_string[type_index++] = message->buf[i]; // Copy the type string
					break;
				case 2: break; // Do nothing, waiting for the third delimiter
				case 3:
					// Copy the value string
					value_string[value_index++] = message->buf[i];
					break;
				default: break; // If the delimiter is 4 or more we are at the end of the message
			}
		}
	}
	// Add null termination to the strings
	type_string[type_index] = 0;
	value_string[value_index] = 0;

	// Return true if both the type and value strings match
	return strcmp(type_string, target_type_str) == 0 && strcmp(value_string, target_value_str) == 0;
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);
	ARG_UNUSED(backend);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTING:
		LOG_INF("CLOUD_EVT_CONNECTING");
		break;
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		cloud_connected = true;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		(void)k_work_cancel_delayable(&connect_work);
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
#if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
		k_work_reschedule(&cloud_update_work, K_NO_WAIT);
#endif

		// When the cloud is ready the device status can be sent to the cloud, to enable the cloud temperature UI
		k_work_submit(&set_device_status_work);
		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		cloud_connected = false;
		k_work_reschedule(&connect_work, K_NO_WAIT);
		break;
	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		LOG_INF("Data received from cloud: %.*s",
			evt->data.msg.len,
			log_strdup(evt->data.msg.buf));
		
		// Upon receiving the message {"temp":"read"} from the cloud, initiate a temperature reading
		if(decode_cloud_message(&evt->data.msg, "temp", "read")) {
			LOG_INF("Temperature read command received");
			k_work_submit(&read_temp_work);
		} else if(decode_cloud_message(&evt->data.msg, "temp", "timer")) {
			LOG_INF("Starting continuous temperature readouts");
			// Start the temperature read timer with an interval of 30 seconds
			k_timer_start(&read_temp_timer, K_MSEC(0), K_MSEC(30000));
		} else if(decode_cloud_message(&evt->data.msg, "temp", "stop")) {
			LOG_INF("Stopping continuous temperature readouts");
			// Stop the temperature read timer
			k_timer_stop(&read_temp_timer);
		}
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_INF("CLOUD_EVT_PAIR_REQUEST");
		break;
	case CLOUD_EVT_PAIR_DONE:
		LOG_INF("CLOUD_EVT_PAIR_DONE");
		break;
	case CLOUD_EVT_FOTA_DONE:
		LOG_INF("CLOUD_EVT_FOTA_DONE");
		break;
	case CLOUD_EVT_FOTA_ERROR:
		LOG_INF("CLOUD_EVT_FOTA_ERROR");
		break;
	default:
		LOG_INF("Unknown cloud event type: %d", evt->type);
		break;
	}
}

static void work_init(void)
{
	k_work_init_delayable(&cloud_update_work, cloud_update_work_fn);
	k_work_init_delayable(&connect_work, connect_work_fn);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_strdup(log_buf));
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_INF("Active LTE mode changed: %s",
			evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
			evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
			evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
			"Unknown");
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		LOG_INF("Modem domain event, type: %s",
			evt->modem_evt == LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE ?
				"Light search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_SEARCH_DONE ?
				"Search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP ?
				"Reset loop detected" :
			evt->modem_evt == LTE_LC_MODEM_EVT_BATTERY_LOW ?
				"Low battery" :
			evt->modem_evt == LTE_LC_MODEM_EVT_OVERHEATED ?
				"Modem is overheated" :
				"Unknown");
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
#if defined(CONFIG_NRF_MODEM_LIB)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		int err;

#if defined(CONFIG_POWER_SAVING_MODE_ENABLE)
		/* Requesting PSM before connecting allows the modem to inform
		 * the network about our wish for certain PSM configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d",
				err);
		}

		LOG_INF("PSM mode requested");
#endif

		err = lte_lc_modem_events_enable();
		if (err) {
			LOG_ERR("lte_lc_modem_events_enable failed, error: %d",
				err);
			return;
		}

		err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Modem could not be configured, error: %d",
				err);
			return;
		}

		/* Check LTE events of type LTE_LC_EVT_NW_REG_STATUS in
		 * lte_handler() to determine when the LTE link is up.
		 */
	}
#endif
}

#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		k_work_reschedule(&cloud_update_work, K_NO_WAIT);
	}
}
#endif

const struct device *dev;
struct sensor_value temp, press, humidity, gas_res;

#define CLOUD_TEMPERATURE_UPDATE_STRING "{\n\"appId\": \"TEMP\",\n\"messageType\": \"DATA\",\n\"data\": \"%d.%06d\",\n\"ts\": %d%09d\n}"

static void read_temp_work_fn(struct k_work *work)
{
	int err;
	static uint8_t cloud_temp_message[256];
	int64_t unix_time = 0;

	// Read the current time using the date_time library
	err = date_time_now(&unix_time);
	if (err) {
		LOG_ERR("Failed to get time: %d", err);
		return;
	}

	// Read the temperature using the sensor API
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

	// Prepare the temperature update by combining the updated temperature sample and the current datetime with the static JSON string defined above
	// Since the sprintf function in Zephyr doesn't support conversion of 64-bit values, the datetime conversion is split in two
	sprintf(cloud_temp_message, CLOUD_TEMPERATURE_UPDATE_STRING, temp.val1, temp.val2, 
			(uint32_t)(unix_time / 1000000000), (uint32_t)(unix_time % 1000000000));
	LOG_INF("Publishing message: %s", log_strdup(cloud_temp_message));

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.buf = cloud_temp_message,
		.len = strlen(cloud_temp_message)
	};

	if (strcmp(CONFIG_CLOUD_BACKEND, "NRF_CLOUD") == 0) {
		msg.endpoint.type = CLOUD_EP_MSG;
	} else {
		msg.endpoint.type = CLOUD_EP_STATE;
	}

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		LOG_ERR("cloud_send failed, error: %d", err);
	}
}

void main(void)
{
	int err;

	LOG_INF("Cloud client has started");

	event_manager_init();
	module_set_state(MODULE_STATE_READY);

	dev = device_get_binding(DT_LABEL(DT_INST(0, bosch_bme680)));
	if(dev == 0) {
		LOG_ERR("BME680 sensor not found");
	}

	cloud_backend = cloud_get_binding(CONFIG_CLOUD_BACKEND);
	__ASSERT(cloud_backend != NULL, "%s backend not found",
		 CONFIG_CLOUD_BACKEND);

	err = cloud_init(cloud_backend, cloud_event_handler);
	if (err) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			err);
	}

	work_init();
	modem_configure();

#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
#endif
	LOG_INF("Connecting to LTE network, this may take several minutes...");

	k_sem_take(&lte_connected, K_FOREVER);

	LOG_INF("Connected to LTE network");
	LOG_INF("Connecting to cloud");

	k_work_schedule(&connect_work, K_NO_WAIT);
}
