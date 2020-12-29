## Downloading the firmware
Navigate to the Arduino "sketchbook" folder, or wherever else you want the project folder to be downloaded to. Then, use the following command to download the repository.
```
git clone https://github.com/ajakef/gem-firmware
```

## Compiling the firmware
Open the Arduino IDE, then open the Gem firmware with File > Sketchbook > Gem.

Then, select the correct board to compile for:
* Tools > Board > Arduino AVR Boards > Arduino Pro or Pro Mini
* Tools > Processor > ATmega328p (3.3V, 8 MHz)

Finally, click the check-mark compile button in the top left corner.

Notice that if you use the "Compiler Warnings: ALL" setting, it will show a lot of warnings. They are nearly all (if not actually all) associated with the dependencies.