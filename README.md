# Driver for user software to controll KeDei 3.5" TFT LCD on SPI without custom kernel

I have moved all test routines into tests directory.

I am going to create a simple driver to be included into user software. I plan to create some simple drawing routines and touch panel control.
Code init only LCD, as for touch I do not "touch" it :smile:


# LCD Module
This is how the screen looks like:<br/>
<img src="https://github.com/saper-2/rpi-spi-lcd35-kedei/blob/master/lcd-photos/tft35-front.jpg" width="300px" /> <img src="https://github.com/saper-2/rpi-spi-lcd35-kedei/blob/master/lcd-photos/tft35-back.jpg" width="300px" />

In dir lcd-photos there is also schematic by reverse engineering pcb :)
I put there in src dir driver & diff files , more info in that dir.
