# Example codes for initializing KeDei 3.5" TFT LCD on SPI without custom kernel

Code init only LCD, as for touch I do not "touch" it :smile:

Python script requires https://github.com/lthiery/SPI-Py

* No extra library required for ```lcd-test``` to ```lcd-test4``` (expect standard dev files that are on RPi preinstalled).
* python and ```lcd-test``` ... ```lcd-test4``` are slow because they utilize /dev/spidev0.x 
* ```lcd-test-``` with ```bcm``` / ```bcm3b``` using bcm2835.h library from http://www.airspayce.com/mikem/bcm2835/index.html and those test run about 10 times faster than test using ```/dev/spidev0.x``` :)
* check each file header for more info

To run test program use sudo:
```pi@raspiv2 ~/spi $ sudo ./lcd-test...```


Compile by executing scripts:
```pi@raspiv2 ~/lcd/spi/tests $ ./clean.sh
pi@raspiv2 ~/lcd/spi/tests $ ./build-tests.sh
```

Note:
I had problems with linking to bcm2835 lib from C++ (and C) but the problem was that ```gcc``` and ```g++``` require ```-l``` to be last argument :zonk: (won't comment this...)


# Movies

This is first movie captured while running first test: https://www.youtube.com/watch?v=_pR_oAXKIs0

This is movie from running ```lcd-test-bcmc``` : https://www.youtube.com/watch?v=yhm2lYVQIQw
