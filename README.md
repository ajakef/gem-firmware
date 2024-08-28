## Installing the Arduino IDE
First, download and install the Arduino IDE from [here](https://www.arduino.cc/en/software#download)

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

## Obtaining a programming adapter
To connect to the Gem, you need a 3.3-V USB-FTDI adapter with the pinout GND-CTS-Vcc-TX-RX-RTS. If your adapter has "DTR" in place of "RTS", that's fine. Both the power and signal voltages MUST be set to 3.3 V to avoid damaging the Gem. Before purchasing an adapter, please confirm that it is shaped correctly so that it's easy to plug in to the Gem's 6-pin header on the left side, with a pinout that matches the Gem.

Such adapters can be found at many online stores. They typically cost under USD 20 and often require soldering a plastic header to the adapter circuit board (a beginner-friendly job), and may require cutting a trace and soldering two pads to set the voltage to 3.3V.

Two good examples are [here](https://www.adafruit.com/product/284) (must bend the plastic header to be perpendicular to the board, and set the power voltage to 3.3V) and [here](https://www.sparkfun.com/products/9873).

## Installing the firmware to a Gem
Align your FTDI programmer with the Gem's six-pin header just below the micro-SD card. The "GND" label on the programmer should line up with "GND" on the Gem board, and the "RST" or "DTR" label on the programmer should line up with the "RST" label on the Gem board.

Select the serial port in the Tools > Port menu. The Arduino IDE can be clumsy about serial port selection, so you may have to re-do this later if an error occurs, including possibly having to unplug and plug in the connector. Windows users may need to install a driver (see Troubleshooting below).

Click the right-arrow "Upload" button in the top left of the Arduino IDE window. Note that this compiles the code and then uploads it. It will take a minute or two to upload and verify the program. If successful, the window below the code will say ```Done Uploading.```.

The following order of steps has been found to be most reliable for programming Gems without trouble:
* Connect power through the Gem's barrel connector before doing anything else.
* Wait for the red light on the GPS to start blinking on and off every second. Sometimes this takes tens of seconds.
* With the FTDI connector already plugged into the computer by USB, connect it to the 6-pin header on the bottom board.
* Watch the serial monitor for text. If text does not appear, try pushing the white "reset" button on the bottom of the bottom board. If text appears then, and uploading fails the first time, you may need to hold "reset" after clicking upload and timing its release just before compilation ends and upload stops.
* Click "upload".

## Troubleshooting
* `Error compiling`: Check that the board selected in Tools>Boards is correct.
* Serial port is grayed out: 
  * Windows: try installing the FTDI driver following [these directions](https://learn.sparkfun.com/tutorials/how-to-install-ftdi-drivers/all). 
  * Try disconnecting and reconnecting the serial connector from the computer and board, and try again.
  * Make sure that your USB cord is not a "charging only" cord, which are very common, not marked differently, and useless for communication.
    * Follow the exact steps in the section above "Installing the firmware to a Gem"
  
* Error in upload: `programmer is not responding`, `not in sync):
  * Follow the exact steps in the section above "Installing the firmware to a Gem"
  * Try holding the white reset button in the corner of the bottom board and releasing right before the Arduino IDE stops compiling and starts uploading.