# v5.0

I don't own this version LCD so I don't have any photos.
It is not compatible from previous "drivers" .

If somebody get diff or ili9341.c file the please share with me I will attach it to src .

# Driver code
For this display a great job did Conjur from RPi forum. He managed to reverse-engineering the init sequence.
I put here his latest code for ESP8266 (and his photos - sorrrrrry :D ) , but it can be easily moved to RPi (python/C/C++).

For the job of moving his code to RPi took the FREE WING from RPi forum. He published his code on github:
https://github.com/FREEWING-JP/RaspberryPi_KeDei_35_lcd_v50

He also put together a driver using tinydrm from notro, more info: https://www.raspberrypi.org/forums/viewtopic.php?p=1136870#p1136870

# Credits
We can't forget about other people who helped in reverse-engineering init seq. for kedei displays:
* **1n4148** from RPi forum ( https://www.raspberrypi.org/forums/viewtopic.php?p=1002856#p1002856 )