# Example codes for initializing KeDei 3.5" TFT LCD on SPI without custom kernel

Code init only LCD, as for touch I do not "touch" it :smile:

Python script requires https://github.com/lthiery/SPI-Py

For C++ code, no extra library required (expect standard dev files that are on RPi preinstalled).
Just compile it with:
```g++ -o lcd-test lcd_test.cpp```

And run with sudo:
```pi@raspiv2 ~/spi $ sudo ./lcd-test```

Both programs are *slow* (very slow :sad: ) , because code access SPI through spi_bcm2835 kernel module - this is bottleneck.

This is how the screen looks like:<br/>
<img src="https://github.com/saper-2/rpi-spi-lcd35-kedei/blob/master/lcd-photos/tft35-front.jpg" width="300px" /> <img src="https://github.com/saper-2/rpi-spi-lcd35-kedei/blob/master/lcd-photos/tft35-back.jpg" width="300px" />

In dir lcd-photos there is also schematic by reverse engineering pcb :)
