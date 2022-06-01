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

Step 1 - Setting up the cloud_client sample

1. In VSCode, click on 'Add an existing application' and select the NCS_INSTALL_FOLDER\v1.9.1\nrf\samples\nrf9160\cloud_client sample
2. Find the "cloud_client" application in the application list, and click on 'Add Build Configuration'. 
3. Select the board 'thingy91_nrf9160_ns'
4. Check the 'Enable debug options' box
5. Click on 'Build Configuration'
6. Open the build output in the terminal window, and wait for the code to build
7. Ensure that the nRF9160DK and the Thingy91 are connected as shown in the picture, and powered on
8. Open the nRF Terminal window and connect to the comport of the Thingy91. 
9. Flash the code into the Thingy91, and ensure that the boot message shows up in the nRF Terminal
10. Open nRF Cloud
11. If you haven't already, add your Thingy91 device to the cloud, and ensure that the SIM card is inserted into the Thingy91 and registered through the cloud
12. Verify that you can open the device in the nRF Cloud interface, and that you can see the Terminal window
13. Try to send a message from the cloud to the Thingy91 by entering a text in the terminal and pressing Send. Please note that the text must be JSON formatted. For the rest of this workshop we will use simple JSON commands on the form {"TYPE":"VALUE"}, where TYPE and VALUE can be any string. 
14. Try to send a simple message like {"hi":"all"}, and verify that the message shows up in the nRF Terminal:

    *I: Data received from cloud: {"hi":"all"}*

