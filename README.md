## Downloading the firmware
Open the Arduino IDE and open File > Preferences, and find the folder shown in the "Sketchbook location" field. Then, download the firmware from the Github site by clicking the green "Code" button and the "Download ZIP". Unzip the file, rename the resulting folder to "gem-firmware", and move it to the sketchbook folder. 

## Compiling the firmware
Once you've moved and renamed the gem-firmware folder, you should be able to open it. Restart the Arduino IDE, then open the Gem firmware with File > Sketchbook > gem-firmware.

Then, select the correct board to compile for:
* Tools > Board > Arduino AVR Boards > Arduino Pro or Pro Mini
* Tools > Processor > ATmega328p (3.3V, 8 MHz)

If you have worked with the gem-firmware program before, please verify that you are working with the correct firmware version by opening the "version.h" tab and ensuring that the version matches the one you just downloaded.

Finally, click the check-mark compile button in the top left corner.

Notice that if you use the "Compiler Warnings: ALL" setting, it will show a lot of warnings. They are nearly all (if not actually all) associated with the dependencies and don't need to be worried about.

## Installing the firmware to a Gem
Align your FTDI programmer with the Gem's six-pin header just below the micro-SD card. The "GND" label on the programmer should line up with "GND" on the Gem board, and the "RST" or "DTR" label on the programmer should line up with the "RST" label on the Gem board.

Select the serial port in the Tools > Port menu. The Arduino IDE can be clumsy about serial port selection, so you may have to re-do this later if an error occurs, including possibly having to unplug and plug in the connector.

Click the right-arrow "Upload" button in the top left of the Arduino IDE window. Note that this compiles the code and then uploads it. It will take a minute or two to upload and verify the program. If successful, the output window at the bottom will say ```avrdude done. Thank you.```

