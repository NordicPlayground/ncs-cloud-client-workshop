Cloud Client nRF91 Workshop
---------------------------

This repository contains a modified version of the cloud_client sample in the nRF Connect SDK which is intended to show how relatively simple examples can be extended with additional functionality, in order to implement more advanced use cases. 

The goal of this workshop is to flash a Thingy91 with the cloud_client sample and connect to it from the nRF Cloud interface. 
Then the example will be extended to take temperature readings from the BME680 sensor on the Thingy91, and send them to the cloud upon request. From the cloud the user can either request a single temperature reading, or start a timer that will read the temperature repeatedly at 30 second intervals. 
Secondly the Common Application Framework LED module will be enabled in order to control the RGB LED on the Thingy91, and separate commands will be added to allow the LED to be turned on and off from the cloud interface. 
The final task is to build upon these functionalities to implement a thermostat feature where a temperature threshold can be configured from the cloud interface, at which point the LED on the Thingy should show whether or not the measured temperature is above or below the set threshold. 

## HW Requirements
- Thingy91
- nRF9160DK for programming and debugging (or another compatible programmer)
- 10-pin SWD programming cable, like [this one](https://www.adafruit.com/product/1675)
- 2x USB Micro-B cables (one for the Thingy91 and one for the nRF9160DK)

## SW Requirements
- nRF Connect for Desktop
   - Toolchain manager (Windows and Mac only)
   - LTE Link Monitor
- Visual Studio Code (from here on referred to as VSCode)
   - nRF Connect for VSCode extension
- nRF Connect SDK v1.9.1 (installed through the Toolchain manager)

For instructions on how to install these items, please follow the exercise [here](https://academy.nordicsemi.com/topic/exercise-1-1/)

## Workshop steps

### Step 1 - Setting up the cloud_client sample
-----------------------------------------------

Open [https://nrfcloud.com/](https://nrfcloud.com/), and create a user if you haven't already done so. Make sure you install the SIM card in your Thingy91 and add it to the cloud, as described [here](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_thingy91_gsg.html#creating-an-nrf-cloud-account).

In VSCode, in the Welcome page of the nRF Connect interface, click on 'Create a new application from sample'.
 Hint: A faster way to do this is to bring up the command palette in VSCode (by pressing F1), typing 'new application', and pressing Enter. 

Ensure that 'Freestanding' application is selected and that the nRF Connect SDK version and selected toolchain is based on v1.9.1. Select the cloud_client sample as a reference, and give the application a new name such as 'cloud_client_workshop':

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_create_application.JPG" width="400">

Find the "cloud_client_workshop" application in the Applications list, and click on 'Add Build Configuration'. 

Select the board 'thingy91_nrf9160_ns'. Check the 'Enable debug options' box, and click on 'Build Configuration':

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_create_build.JPG" width="400">

Open the build output in the terminal window, and wait for the code to finish building. 

Ensure that the nRF9160DK and the Thingy91 are connected as described [here](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_thingy91_gsg.html#updating-firmware-through-external-debug-probe). Also make sure they are powered on.

To show log output from the Thingy91 it is necessary to connect to one of two virtual CDC comports enumerated by the Thingy when it is connected over USB. The nRF9160DK will also enumerate various comports, and the easiest way to find out which comport is for log output from the Thingy you can use the LTE Link Monitor app available through nRF Connect for Desktop. Simply connect to the Thingy91 from the LTE Link Monitor, and check which comport the link monitor opens:
   
<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_lte_link_monitor.JPG" width="500">

Close the LTE Link Monitor, and go back to VSCode. Select the nRF Terminal, and click on the 'Start Terminal With New Configuration' button:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_start_terminal.JPG" width="600">

When prompted to select connection mode at the top of VSCode, select 'Serial Port'. 
When prompted to select the comport select the comport that was used by the LTE Link Monitor earlier. 
When prompted to choose configuration select '115200 8n1'. 
  
Flash the code into the Thingy91, and ensure that the boot message shows up in the nRF Terminal:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_nrfterminal_boot.JPG" width="400">

In the build overview in VSCode, expand the 'Application' group and double click the main.c file to open it:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_select_main.JPG" width="200">

It is advantageous to disable PSM mode in the LTE modem during development, as it will greatly reduce the latency when communicating with the cloud. To do this navigate to line 229 in main.c. Delete the *#if defined(CONFIG_POWER_SAVING_MODE_ENABLE)* line, and the corresponding *#endif* on line 243. 
Change the *err = lte_lc_psm_req(true);* line to *err = lte_lc_psm_req(false);* in order to disable PSM mode. 
In order for this change to take effect you need to build and flash the code again. Just pressing Flash is sufficient, as this will automatically trigger a build if the code has changed. 

Verify that you can open the device in the nRF Cloud interface, and that you can see the Terminal window

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_cloud_terminal.jpg" width="800">

Try to send a message from the cloud to the Thingy91 by entering a text in the terminal and pressing Send. Please note that the text must be JSON formatted. For the rest of this workshop we will use simple JSON commands on the form {"TYPE":"VALUE"}, where TYPE and VALUE can be any string. As an example you can send {"message":"hi"}

Verify that the message shows up in the nRF Terminal:

*I: Data received from cloud: {"message":"hi"}*

### Step 2 - Integrate the BME680 environment sensor 
----------------------------------------------------
In the following step we are going to enable the BME680 environment sensor on the Thingy91, in order to read out the local temperature. The sensor will be enabled through the Kconfig interface, and the code from the [BME680 Zephyr sample](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.9.1/zephyr/samples/sensor/bme680/README.html) will be copied into the cloud_client project to verify that the sensor works. 

Start by opening the Kconfig configurator in VSCode:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_kconfig.jpg" width="300">

Search for 'bme680' in the search field:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_kconfig_bme680.JPG" width="800">

Enable 'Sensor Drivers' and 'BME680 sensor', and click 'Save to file':

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_kconfig_enable_save_to_file.JPG" width="800">

Verify that the following lines were added to your prj.conf file:
```C
CONFIG_SENSOR=y
CONFIG_BME680=y
CONFIG_DEBUG_THREAD_INFO=y
CONFIG_DEBUG_OPTIMIZATIONS=y
```
Add the following include to the top of your main.c file:
```C
#include <drivers/sensor.h>
```

Add the following variables just above the main() function, around line 276 of main.c:
```C
const struct device *dev;
struct sensor_value temp, press, humidity, gas_res;
```

Add the following code to your main() function, just after the *LOG_INF("Cloud client has started");* line:
```C
dev = device_get_binding(DT_LABEL(DT_INST(0, bosch_bme680)));
if(dev == 0) {
   LOG_ERR("BME680 sensor not found");
}
```

At the very end of the main function, after *k_work_schedule(&connect_work, K_NO_WAIT);*, add the following code (directly from the BME680 sample):
```C
while (1) {
   k_sleep(K_MSEC(3000));

   sensor_sample_fetch(dev);
   sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
   sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
   sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
   sensor_channel_get(dev, SENSOR_CHAN_GAS_RES, &gas_res);

   printf("T: %d.%06d; P: %d.%06d; H: %d.%06d; G: %d.%06d\n",
         temp.val1, temp.val2, press.val1, press.val2,
         humidity.val1, humidity.val2, gas_res.val1,
         gas_res.val2);
}
```

Build and flash the code, and verify that the environment readings are printed in the nRF Terminal:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_nrfterminal_env_readings.JPG" width="400">

Code available here: [Code snapshot after step 2](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_2)

### Step 3 - Add a function to decode messages from the cloud
-------------------------------------------------------------
In this step a function will be added to decode the messages received from the cloud, and if the {"temp":"read"} command is received a message will be printed to the log.

Add the following function somewhere above the *void cloud_event_handler(const struct cloud_backend * const backend, const struct cloud_event * const evt, void * user_data)* function in main.c:

```C
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
```
Demonstrate that the *decode_cloud_message(..)* function works, by adding the following code inside the *cloud_event_handler(..)* function, inside the *CLOUD_EVT_DATA_RECEIVED* switch case:

```C
// Upon receiving the message {"temp":"read"} from the cloud, initiate a temperature reading
if(decode_cloud_message(&evt->data.msg, "temp", "read")) {
   LOG_INF("Temperature read command received");
}
```
Build and flash the code, and verify that you get the following nRF Terminal output when sending the {"temp":"read"} command from the cloud:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s3_temp_command_received.JPG" width = "400">

Code available here: [Code snapshot after step 3](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_3)

### Step 4 - Send a temperature update with timestamp to the cloud
------------------------------------------------------------------
For this step a function will be added to send a temperature update to the cloud with a timestamp, following the standard format for nRF Cloud temperature samples.
When the {"temp":"read"} command is received the function will be triggered, sending a temperature reading immediately. 
The DATE_TIME library will be enabled to facilitate the reading of an accurate time and date. 
The loop in main printing out environment information every 3 seconds will be removed, as this code was only added for testing purposes. 

Start by opening the Kconfig configurator. Search for 'date_time', and enable the Date time library. Click 'Save to file'. 
After doing this *CONFIG_DATE_TIME=y* should be added to the prj.conf file (it is also possible to simply add this line manually to prj.conf, without using the Kconfig configurator).

Add the following include to the top of main.c:
```C
#include <date_time.h>
```

It is not recommended to run a lot of code directly from an event handler, since event handlers are often running in interrupt context. From interrupt context there are many drivers and libraries that are unavailable (or unsafe to use), and you also risk blocking other interrupts in the system which could lead to system instabilities or crashes. 
To avoid this issue it is possible to register a work item, which will be triggered from the event handler (by using k_work_submit(..)), but where the actual work function will be run from the thread context. For more information about work queues and work items please refer to the [documentation](https://docs.zephyrproject.org/2.6.0/reference/kernel/threads/workqueue.html). 

To implement this, start by declaring a work function and a work item by adding the following code at the top of main.c (below *static K_SEM_DEFINE(lte_connected, 0, 1);* ):
```C
static void read_temp_work_fn(struct k_work *work);
static K_WORK_DEFINE(read_temp_work, read_temp_work_fn);
```

Just above main() in main.c add the following function definition. This is the work function that will be triggered whenever the temperature should be sampled and sent to the cloud:
```C
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
```

In order to ensure that read_temp_work_fn(..) is called when the {"temp":"read"} command is received, enter the following code line inside the *cloud_event_handler(..)* function, just after the *LOG_INF("Temperature read command received");* line:
```C
k_work_submit(&read_temp_work);
```
The entire *CLOUD_EVT_DATA_RECEIVED* case should now look like this:
```C
case CLOUD_EVT_DATA_RECEIVED:
	LOG_INF("CLOUD_EVT_DATA_RECEIVED");
	LOG_INF("Data received from cloud: %.*s",
		evt->data.msg.len,
		log_strdup(evt->data.msg.buf));

	// Upon receiving the message {"temp":"read"} from the cloud, initiate a temperature reading
	if(decode_cloud_message(&evt->data.msg, "temp", "read")) {
		LOG_INF("Temperature read command received");
		k_work_submit(&read_temp_work);
	}
	break;
```

Remove the while loop that you added ad the end of main.c in an earlier step, to avoid spamming the log with environment readings.

Build and flash the code. Once the Thingy91 connects to the cloud again send the {"temp":"read"} command from the cloud, and verify that you get a temperature update in return:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s4_temp_in_cloud.JPG" width="500">

Code available here: [Code snapshot after step 4](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_4)

### Step 5 - Update device status in the cloud
----------------------------------------------
nRF Cloud allows devices to send a device status message, which gives some information about the device to the cloud interface. In order to properly display temperature data in the cloud it is necessary to enable temperature in the ui configuration part of the device status message, and this step will handle that. 

At the top of main.c, add the following include folder:
```C
#include "nrf_cloud_codec.h"
```

Add the following code above the *bool decode_cloud_message(..)* function in main.c:
```C
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
```

The function defined above should be called as soon as the cloud is connected and ready. Inside the cloud_event_handler(..) function in main.c, at the end of the CLOUD_EVT_READY case, add the following code:
```C
// When the cloud is ready the device status can be sent to the cloud, to enable the cloud temperature UI
k_work_submit(&set_device_status_work);
```

Build and flash the code again. Try to read the temperature after the device has connected to the cloud, and verify that a temperature graph window will appear in the cloud. If the temperature is read several times the graph should reflect this:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s5_cloud_temp_graph.jpg" width="800">

Code available here: [Code snapshot after step 5](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_5)

### Step 6 - Add a function to read the temperature at regular intervals
------------------------------------------------------------------------
In order to better utilize the graph functionality in nRF Cloud a timer will be added to read out the temperature automatically every 30 seconds. 
Two new cloud commands will be added in order to allow the user to enabled or disable automatic temperature readings through the cloud interface. 
To start the timer send {"temp":"timer"}. To stop the timer send {"temp":"stop"}

First a Zephyr timer will be defined in order to trigger a callback at regular intervals. The timer callback function will trigger the temperature read function that was implemented in step 4. 
Paste the following code in main.c, towards the top of the file, just below *static K_WORK_DEFINE(read_temp_work, read_temp_work_fn);* :
```C
static void read_temp_timer_fn(struct k_timer *timer)
{
	k_work_submit(&read_temp_work);
}
static K_TIMER_DEFINE(read_temp_timer, read_temp_timer_fn, NULL);
```

In the *cloud_event_handler(..)* function, replace the current *CLOUD_EVT_DATA_RECEIVED* case with the following, to respond to two more commands from the cloud:
```C
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
```

Build and flash the code. Verify that you can start regular temperature readings by sending {"temp":"timer"}, and stop them again by sending {"temp":"stop"}

Code available here: [Code snapshot after step 6](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_6)

### Step 7 - Control the RGB LED on the Thingy91 using the CAF module
---------------------------------------------------------------------
For this step the Common Application Framework (CAF) LED module will be enabled in order to control the RGB LED on the Thingy91. 

Start by adding the following lines to the bottom of prj.conf:
```C
CONFIG_CAF=y
CONFIG_CAF_LEDS=y
CONFIG_PWM=y
CONFIG_LED=y
CONFIG_LED_PWM=y 
```

Add the following include to the top of main.c:
```C
#include <event_manager.h>
```

Towards the top of main.c, just below the *LOG_MODULE_REGISTER(cloud_client, CONFIG_CLOUD_CLIENT_LOG_LEVEL);* line, add the following:
```C
#define MODULE main
#include <caf/events/module_state_event.h>
```

The Common Application Framework is based around a module called the Event Manager, which provides a generic event framework for sending status information between different modules in the application. To initialize the event manager add the following code to the main() function in main.c, just below the *LOG_INF("Cloud client has started");* line:
```C
event_manager_init();
module_set_state(MODULE_STATE_READY);
```

In order for the CAF LED module to work the LED pins need to be described in the device tree. The Thingy91 board files does not include these definitions, which means they have to be added as a device tree overlay. 

Start by expanding the 'Input files' section in the build overview, and click on the 'No overlay files, Click to create one' button. 
A message box requesting you to run a pristine build will show up, but in order to save some time don't click this yet. 

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s7_add_overlay.JPG" width="200">

The new overlay file should show up in the 'Input files' section. Double click on the file to open it:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s7_open_overlay_file.JPG" width="200">

Insert the following code in the overlay file:
```C
/ {
	pwmleds0 {
		compatible = "pwm-leds";
		status = "okay";
		pwm_led0: led_pwm_0 {
			status = "okay";
			pwms = <&pwm0 29>;
			label = "LED0 red";
		};
		pwm_led1: led_pwm_1 {
			status = "okay";
			pwms = <&pwm0 30>;
			label = "LED0 green";
		};
		pwm_led2: led_pwm_2 {
			status = "okay";
			pwms = <&pwm0 31>;
			label = "LED0 blue";
		};
	};
};   
```

Now run a pristine build, either by pressing the button in the popup box, or by clicking the 'Pristine Build' button in the Actions menu.

Code available here: [Code snapshot after step 7](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_7)

### Step 8 - Add commands to turn on and off the LED from the cloud
-------------------------------------------------------------------
In the following step two additional commands will be added to the application, allowing the LED to be turned on or off by sending direct commands from the cloud. 

Add the following include to the top of main.c:
```C
#include <caf/events/led_event.h>
```

The LED module uses LED events to control the LED, by defining different LED effects that can be run on the LED. These effects make it easy to set the LED to different colors, or to have the LED blink or pulse between two colors automatically. To set up some standard LED effects, and add a function for sending a new LED event to the LED module, add the following code to main.c, just below the *static bool cloud_connected;* line:
```C
// Define various LED effects to be used by the application
const struct led_effect led_effect_red = LED_EFFECT_LED_ON(LED_COLOR(255, 0, 0));
const struct led_effect led_effect_blink_red = LED_EFFECT_LED_BLINK(100, LED_COLOR(255, 0, 0));
const struct led_effect led_effect_pulse_red = LED_EFFECT_LED_BREATH(100, LED_COLOR(255,0,0));
const struct led_effect led_effect_blue = LED_EFFECT_LED_ON(LED_COLOR(0, 0, 255));
const struct led_effect led_effect_blink_blue = LED_EFFECT_LED_BLINK(100, LED_COLOR(0, 0, 255));
const struct led_effect led_effect_pulse_blue = LED_EFFECT_LED_BREATH(100, LED_COLOR(0, 0, 255));
const struct led_effect led_effect_off = LED_EFFECT_LED_OFF();

// This function is used to send LED events to the LED module, in order to set the LED in different states
static void send_led_event(const struct led_effect *effect)
{
	struct led_event *event = new_led_event();

	event->led_id = 0;
	event->led_effect = effect;
	EVENT_SUBMIT(event);
}
```

In order to allow the LED to be controlled from the cloud interface, add the following code at the end of the *CLOUD_EVT_DATA_RECEIVED* case in the *cloud_event_handler(..)* function:
```C
else if(decode_cloud_message(&evt->data.msg, "led", "blink red")) {
	LOG_INF("Setting LED to blink red");
	send_led_event(&led_effect_blink_red);
} else if(decode_cloud_message(&evt->data.msg, "led", "off")) {
	LOG_INF("Turning off LED");
	send_led_event(&led_effect_off);
}
```

Build and flash the code. After the device has connected to the cloud verify that the LED can be controlled by sending the {"led":"blink red"} and {"led":"off"} commands. 

Code available here: [Code snapshot after step 8](https://github.com/NordicPlayground/ncs-cloud-client-workshop/tree/step_8)

### Step 9 - Add a thermostat function
--------------------------------------
In order to expand on the functionality introduced in the previous steps, and combine the temperature sensor, LED control and cloud interface elements into one feature, the goal of this step is to implement a simple thermostat function.
A new command should be added introduced allowing a threshold temperature to be configured through the cloud. As an example, if the command {"thermostat":"28"} is set, the threshold should be set to 28 degrees. 
Then whenever the temperature sensor is read the temperature should be compared to the threshold value. If the actual temperature is higher the LED should be set to blink red, and if the temperature is lower it should blink blue. 

This is a free form exercise, and the code needed to complete the step will not be provided. 

### Bonus steps
---------------
In the unlikely event that all the steps above have been completed, with time to spare, here are some suggestions for further improvements
- To save cloud bandwidth, add some code to the *read_temp_work_fn(..)* function to have the function skip the cloud update unless the temperature has changed more than a specific amount (1.0C for instance). 
- Add support for the remaining environment readings supported by the BME680 sensor, such as humidity and air pressure
