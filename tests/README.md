# Example codes for initializing KeDei 3.5" TFT LCD on SPI without custom kernel

Code init only LCD, as for touch I do not "touch" it :smile:

Python script requires https://github.com/lthiery/SPI-Py

For C++ code, no extra library required (expect standard dev files that are on RPi preinstalled).
Just compile it with:
```g++ -o lcd-test lcd_test.cpp```

And run with sudo:
```pi@raspiv2 ~/spi $ sudo ./lcd-test```

Both programs are *slow* (very slow :sad: ) , because code access SPI through spi_bcm2835 kernel module - this is bottleneck.

I have put 4 version, first one is the "first one" :) for next I did some changes, see header of c++ file for more info.

# using libbcm2835
<b>notro</b> advised to use bcm2835 lib, so I did, and now I get some rather good results.
I have created new program with bcm2835 lib from http://www.airspayce.com/mikem/bcm2835/index.html
This lib speed-up SPI about 10 times :smile:

Compile:
``` gcc -std=c11 -o lcd-test-bcmc -D_POSIX_C_SOURCE=200112L -Lbcm2835 -Ibcm2835 -lbcm2835 bcm2835/bcm2835.c lcd_test_bcm.c```

and run with sudo:
```sudo ./lcd-test-bcmc```

I had problems with linking to bcm2835 lib from C++ (ld was complaining *undefined reference* for every one function call from bcm2835 lib), so I had to port my program from C++ to C (actually for unknown reason I can't use globally installed lib bcm2835 too :sad: ).

# Movies

This is first movie captured while running first test: https://www.youtube.com/watch?v=_pR_oAXKIs0
This is movie from running ```lcd-test-bcmc``` : https://www.youtube.com/watch?v=yhm2lYVQIQw