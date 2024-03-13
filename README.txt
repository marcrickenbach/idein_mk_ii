Idein MK ii


Re-iteration of Eurorack version. 

Idein is built around an all-light sensor which serves as the core logic of the device. 

On each reading, sensor data is compared to user-defined thresholds, flipping an appropriate bit in a 'note' value if the threshold is surpassed. On a rising clock pulse whatever note data is stored is sent out for processing. 

This note value determines the fundamental frequency that is produced in the voice module, with further processing and modifications determined by the ongoing sensor reading information. This allows for the light information to not only determine the fundamental frequency, but any harmonics, wave-shaping, filtering or other effects we later add.

In addition to the main synthesis engine, we'll also be streaming MIDI data over USB. This will be the last feature we implement, but the idea is that the light sensor data, as well as potentiometer readings, will be outputting a constant CC controls (is this feasible for sensor data?). Eventually, I'd like for this to all be configurable over a USB-WebApp. An on-board EEPROM will save configurations. 


As of 3/13/24: 

Waiting on PCB to arrive. Once assembled, we'll start testing. As yet, I've set up the general structure of the firmware. The UI should more or less be up and running soon after initial flash, as should the ADC/Pot module. I've set up the i2s in the codec module, but have yet to test it. It is currently set up to execute the Zephyr example code, triggering a sine wave output on a button press. 



Controls: 

Set-Threshold: Hold down color button while adjusting corresponding slide potentiometer.
Change Bit-Flip (for different sequence possibilities): Short press on 5 buttons (color and white). 
Loop Sequence: hold shift button. Turning encoder while holding shift button changes length of loop. Releasing Shift releases loop. 
