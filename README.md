# Changelog
* 2016-06-18
	* some minor changes in structure of repo, added some info about versions & photos
* 2016-08-05
	* I decided to add change log section :D
	* Added info for reverse engineered init for v5 lcd in ```tests```
* 2019-06-21
	* Updated some info (including v6.3)
 
# Driver for user software to controll KeDei 3.5" TFT LCD on SPI without custom kernel

I had in plans to create a custom driver but I decided this is not worth my time. I'm going to "re-interface" this lcd to be supported by fbtft. I have a prototype pcb with fbtft-spi interface ready (I have to send this project to pcb prototype service).

** I might periodically update this repo but I consider kedei display as waste of time and money so don't bother me about them :wink: **

# LCD Module
There is 8 version already each one is built on different LCD display, constant is:
- SPI to LCD interface
- draw commands (cmds for setting draw window in lcd)

I have moved data from `lcd-photos` to `lcd-module-info` and groupped informations (and kernels) by lcd version.

# Test code
I have written for v1 some test routines that init lcd and draw few colors using different drivers (python, libbcm2835, linux-spi).

# Adapter FBTFT test
Unfortunatly, because of my "spider-circuit" I've broken my lcd (I've made a some short :cry: ), but my v1 pcb adapter (one-side pcb with wire jumpers) works identically like the spider-like circuit so I assume the circuit is ok and connections too, but lcd gone bad... See photos in ```hw-hack\v2.1\results``` :)

# Links
- KeDei kernel/sysimage files: http://en.kedei.net/raspberry/raspberry.html
- I have created a "backup" repo for kedei sysimages: https://www.mediafire.com/folder/t709uvp6a3ftb/kedei-sysimg and kernels: https://www.mediafire.com/folder/t709uvp6a3ftb/kedei-sysimg
- Topic on raspberrypi forum: https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=124961 - you can get there help, but first check whole thread because your question might be already answered.

# EOF
This is probably end of me playing with KeDei lcds. From this point, I'm going to buy next displays that are nativly supported by fbtft :smile: (and fbtft is now part of official raspberry pi kernel :smile: ).

##What lcds are compatible?
Check this page: https://github.com/notro/fbtft/wiki/LCD-Modules

# EOT