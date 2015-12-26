# Driver for user software to controll KeDei 3.5" TFT LCD on SPI without custom kernel

I had in plans to create a custom driver but I decided this is not worth my time. I'm going to "re-interface" this lcd to be supported by fbtft. I have a prototype pcb with fbtft-spi interface ready (I have to send this project to pcb prototype service).

# LCD Module
There is 3 version already each one is built on different LCD display, constant is:
- SPI to LCD interface
- draw commands (cmds for setting draw window in lcd)

I have moved data from `lcd-photos` to `lcd-module-info` and groupped informations (and kernels) by lcd version.

# Test code
I have written for v1 some test routines that init lcd and draw few colors using different drivers (python, libbcm2835, linux-spi).

