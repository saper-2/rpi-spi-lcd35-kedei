# Oryginal (provided by seller) source codes / diff's

* ili9341.c - file from http://heikki.virekunnas.fi/2015/raspberry-pi-tft/
* ili9341_patch.diff - file from http://heikki.virekunnas.fi/2015/raspberry-pi-tft/ 
Those two above work until RPi kernel 4.1.y 

* KeDei35-diff-20151105.diff - file from MunhozThiago (from issue in fbtft: https://github.com/notro/fbtft/issues/215 )


In the diff is a little newer ```ili9341``` (This is not really ili9341 in LCD but the file is taken from ili9341 driver and moddified to fit KeDei lcd) driver because it have now backlight control :)