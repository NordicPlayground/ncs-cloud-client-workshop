Cloud Client nRF91 Workshop
---------------------------

This repository contains a modified version of the cloud_client sample in the nRF Connect SDK which is intended to show how relatively simple examples can be extended with additional functionality, in order to implement more advanced use cases. 

The goal of this workshop is to flash a Thingy91 with the cloud_client sample and connect to it from the nRF Cloud interface. 
Then the example will be extended to take temperature readings from the BME680 sensor on the Thingy91, and send them to the cloud upon request. From the cloud the user can either request a single temperature reading, or start a timer that will read the temperature repeatedly at 30 second intervals. 
Secondly the Core Application Framework LED module will be enabled in order to control the RGB LED on the Thingy91, and separate commands will be added to allow the LED to be turned on and off from the cloud interface. 
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

Workshop steps
--------------

### Step 1 - Setting up the cloud_client sample

1. In VSCode, click on 'Add an existing application' and select the NCS_INSTALL_FOLDER\v1.9.1\nrf\samples\nrf9160\cloud_client sample
2. Find the "cloud_client" application in the application list, and click on 'Add Build Configuration'. 
3. Select the board 'thingy91_nrf9160_ns'
4. Check the 'Enable debug options' box
5. Click on 'Build Configuration'
6. Open the build output in the terminal window, and wait for the code to build
7. Ensure that the nRF9160DK and the Thingy91 are connected as described [here](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_thingy91_gsg.html#updating-firmware-through-external-debug-probe). Also make sure they are powered on
8. Open the nRF Terminal window and connect to the comport of the Thingy91. Make sure to avoid connecting to one of the comports set up by the nRF9160DK:

   <img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_comports_to_avoid.JPG" width="300">
10. Flash the code into the Thingy91, and ensure that the boot message shows up in the nRF Terminal

    <img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_nrfterminal_boot.JPG" width="400">
11. Open [https://nrfcloud.com/](https://nrfcloud.com/), and create a user if you haven't already done so
12. Make sure you install the SIM card in your Thingy91 and add it to the cloud, as described [here](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_thingy91_gsg.html#creating-an-nrf-cloud-account)
13. Verify that you can open the device in the nRF Cloud interface, and that you can see the Terminal window

    <img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s1_cloud_terminal.jpg" width="800">
13. Try to send a message from the cloud to the Thingy91 by entering a text in the terminal and pressing Send. Please note that the text must be JSON formatted. For the rest of this workshop we will use simple JSON commands on the form {"TYPE":"VALUE"}, where TYPE and VALUE can be any string. 
    As an example you can send {"message":"hi"}
14. Verify that the message shows up in the nRF Terminal:

    *I: Data received from cloud: {"message":"hi"}*

### Step 2 - Integrate the BME680 environment sensor 
In the following step we are going to enable the BME680 environment on the Thingy91, in order to read out the temperature. The sensor will be enabled through the Kconfig interface, and the code from the [BME680 Zephyr sample](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.9.1/zephyr/samples/sensor/bme680/README.html) will be copied into the cloud_client project to verify that the sensor works. 

Open the Kconfig configurator in VSCode

Search for 'bme680' in the search field:

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_kconfig_bme680.JPG" width="800">

Enable 'Sensor Drivers' and 'BME680 sensor', and click 'Save to file':

<img src="https://github.com/NordicPlayground/ncs-cloud-client-workshop/blob/workshop_with_instructions/pics/s2_kconfig_enable_save_to_file.JPG" width="800">

Verify that the following lines were added to your prj.conf file:
```C
CONFIG_SRAM_SIZE=128
CONFIG_SRAM_BASE_ADDRESS=0x20020000
CONFIG_SENSOR=y
CONFIG_NRFX_TWIM2=y
CONFIG_BME680=y
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
