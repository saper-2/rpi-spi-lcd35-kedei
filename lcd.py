#!/usr/bin/python

import spi
import time

# LCD is on spi0 CE1
SPI_DEVICE = "/dev/spidev0.1"
# clock 24MHz
SPI_SPEED = 24000000

# sending 0x00 before is just for clearing shift registers from leftovers :)

def lcd_reset():
	# for reset I use slow clock - just my habit
	stat = spi.openSPI(SPI_DEVICE,speed=1000000,bits=8)
#	print "LCD RESET(1), SPI_open=",stat
	spi.transfer((0,0,0,0)) # set 0 on U1-Q2
	spi.closeSPI()
	time.sleep(0.05) # 50ms reset
	# unreset
	stat = spi.openSPI(SPI_DEVICE,speed=1000000,bits=8)
#	print "LCD RESET(2), SPI_open=",stat
	spi.transfer((0,0,0,2)) # set 1 on U1-Q2
	spi.closeSPI()
	time.sleep(0.1) # 100ms after reset
	return

def lcd_data(data):
	stat = spi.openSPI(SPI_DEVICE,speed=SPI_SPEED,bits=8)
#	print "LCD DATA(1)=%X, SPI_open=" % (data) , stat
	spi.transfer((0,data>>8,0x00ff&data,0x15))	# 0x15 - DATA_BE const from ili9341.c (BE is short form "before")
	spi.closeSPI()
#	time.sleep(0.00001) # 10us
	stat = spi.openSPI(SPI_DEVICE,speed=SPI_SPEED,bits=8)
#	print "LCD DATA(2)=%X, SPI_open=" % (data) , stat
	spi.transfer((0,data>>8,0x00ff&data,0x1F)) # 0x1F - DATA_AF const from ili9341.c (AF is short form "after")
	spi.closeSPI()
	return
	
def lcd_cmd(cmd):
	stat = spi.openSPI(SPI_DEVICE,speed=SPI_SPEED,bits=8)
#	print "LCD CMD(1)=%X, SPI_open=" % (cmd) , stat
	spi.transfer((0,cmd>>8,0x00ff&cmd,0x11)) # 0x11 - CMD_BE const from ili9341.c 
	spi.closeSPI()
#	time.sleep(0.0001) # 100us
	stat = spi.openSPI(SPI_DEVICE,speed=SPI_SPEED,bits=8)
#	print "LCD CMD(2)=%X, SPI_open=" % (cmd) , stat
	spi.transfer((0,cmd>>8,0x00ff&cmd,0x1B)) # x01B - CMD_AF const from ili9341.c 
	spi.closeSPI()
	return

# this function does really nothing , just set LCD bounds
def lcd_setptr():
	lcd_cmd(0x002b)
	lcd_data(0x0000) 
	lcd_data(0x0000) # 0
	lcd_data(0x0001)
	lcd_data(0x003f) #319
	
	lcd_cmd(0x002a)
	lcd_data(0x0000)
	lcd_data(0x0000) # 0
	lcd_data(0x0001)
	lcd_data(0x00df) # 479
	
	lcd_cmd(0x002c);
	return
	
def lcd_setarea(x,y):
	lcd_cmd(0x002b)
	lcd_data(y>>8) 
	lcd_data(0x00ff&y)
	lcd_data(0x0001)
	lcd_data(0x003f)

	lcd_cmd(0x002a)
	lcd_data(x>>8) 
	lcd_data(0x00ff&x) 
	lcd_data(0x0001)
	lcd_data(0x00df)
	lcd_cmd(0x002c)
	return

def lcd_setarea2(sx,sy,x,y):
	
	if sx>479:
		sx=0
	
	if sy>319:
		sy=0
		
	if x>479:
		x=479
	
	if y>319:
		y=319
	
	lcd_cmd(0x002b)
	lcd_data(sy>>8) 
	lcd_data(0x00ff&sy)
	lcd_data(y>>8)
	lcd_data(0x00ff&y)

	lcd_cmd(0x002a)
	lcd_data(sx>>8) 
	lcd_data(0x00ff&sx) 
	lcd_data(x>>8)
	lcd_data(0x00ff&x)
	
	lcd_cmd(0x002c)
	return

	
def lcd_fill(color565):
	lcd_setptr()
	for x in range(0,153601):
		lcd_data(color565)
	
	return

def lcd_fill2(sx,sy,x,y,color565):
	tmp=0
	if sx>479:
		sx=0
	
	if sy>319:
		sy=0
		
	if x>479:
		x=479
	
	if y>319:
		y=319
	
	if sx>x:
		tmp=sx
		sx=x
		x=tmp
	
	if sy>y:
		tmp=sy
		sy=y
		y=tmp
	
	cnt = (y-sy) * (x-sx)
	lcd_setarea2(sx,sy,x,y)
	for x in range(0,cnt):
		lcd_data(color565)
	
	return
	

lcd_reset()
time.sleep(0.1)
lcd_cmd(0x0000)
time.sleep(0.0001)

lcd_cmd(0x00B0)
lcd_data(0x0000)
lcd_cmd(0x0011)
time.sleep(0.05) #mdelay(50)

lcd_cmd(0x00B3)
lcd_data(0x0002)
lcd_data(0x0000)
lcd_data(0x0000)
lcd_data(0x0000)

lcd_cmd(0x00C0)
lcd_data(0x0010)#13
lcd_data(0x003B)#480
lcd_data(0x0000)
lcd_data(0x0002)
lcd_data(0x0000)
lcd_data(0x0001)
lcd_data(0x0000)#NW
lcd_data(0x0043)

lcd_cmd(0x00C1)
lcd_data(0x0008)#w_data(0x0008)
lcd_data(0x0016)#w_data(0x0016)#CLOCK
lcd_data(0x0008)
lcd_data(0x0008)

lcd_cmd(0x00C4)
lcd_data(0x0011)
lcd_data(0x0007)
lcd_data(0x0003)
lcd_data(0x0003)

lcd_cmd(0x00C6)
lcd_data(0x0000)

lcd_cmd(0x00C8) #GAMMA
lcd_data(0x0003)
lcd_data(0x0003)
lcd_data(0x0013)
lcd_data(0x005C)
lcd_data(0x0003)
lcd_data(0x0007)
lcd_data(0x0014)
lcd_data(0x0008)
lcd_data(0x0000)
lcd_data(0x0021)
lcd_data(0x0008)
lcd_data(0x0014)
lcd_data(0x0007)
lcd_data(0x0053)
lcd_data(0x000C)
lcd_data(0x0013)
lcd_data(0x0003)
lcd_data(0x0003)
lcd_data(0x0021)
lcd_data(0x0000)

lcd_cmd(0x0035)
lcd_data(0x0000)

lcd_cmd(0x0036)  
lcd_data(0x0028)

lcd_cmd(0x003A)
lcd_data(0x0055) #55 lgh

lcd_cmd(0x0044)
lcd_data(0x0000)
lcd_data(0x0001)

lcd_cmd(0x00B6)
lcd_data(0x0000)
lcd_data(0x0002) #220 GS SS SM ISC[3:0]
lcd_data(0x003B)

lcd_cmd(0x00D0)
lcd_data(0x0007)
lcd_data(0x0007) #VCI1
lcd_data(0x001D) #VRH

lcd_cmd(0x00D1)
lcd_data(0x0000)
lcd_data(0x0003) #VCM
lcd_data(0x0000) #VDV

lcd_cmd(0x00D2)
lcd_data(0x0003)
lcd_data(0x0014)
lcd_data(0x0004)



lcd_cmd(0xE0)  
lcd_data(0x1f)  
lcd_data(0x2C)  
lcd_data(0x2C)  
lcd_data(0x0B)  
lcd_data(0x0C)  
lcd_data(0x04)  
lcd_data(0x4C)  
lcd_data(0x64)  
lcd_data(0x36)  
lcd_data(0x03)  
lcd_data(0x0E)  
lcd_data(0x01)  
lcd_data(0x10)  
lcd_data(0x01)  
lcd_data(0x00)  

lcd_cmd(0XE1)  
lcd_data(0x1f)  
lcd_data(0x3f)  
lcd_data(0x3f)  
lcd_data(0x0f)  
lcd_data(0x1f)  
lcd_data(0x0f)  
lcd_data(0x7f)  
lcd_data(0x32)  
lcd_data(0x36)  
lcd_data(0x04)  
lcd_data(0x0B)  
lcd_data(0x00)  
lcd_data(0x19)  
lcd_data(0x14)  
lcd_data(0x0F)  

lcd_cmd(0xE2)
lcd_data(0x0f)
lcd_data(0x0f)

lcd_data(0x0f)

lcd_cmd(0xE3)
lcd_data(0x0f)
lcd_data(0x0f)

lcd_data(0x0f)


lcd_cmd(0x13)


lcd_cmd(0x0029)
time.sleep(0.02) #mdelay(20)

lcd_cmd(0x00B4)
lcd_data(0x0000)
time.sleep(0.02) #mdelay(20)
lcd_cmd(0x002C)
lcd_cmd(0x002A) 
lcd_data(0x0000)
lcd_data(0x0000)

lcd_data(0x0001)
lcd_data(0x000dF)

lcd_cmd(0x002B)  
lcd_data(0x0000)
lcd_data(0x0000)
lcd_data(0x0001)
lcd_data(0x003f) 

lcd_cmd(0x002c) 

lcd_fill(0x0000)
time.sleep(5)

for color in range(0, 65535):
	lcd_fill2(180,110,300,210,color)
	color=color+1
	time.sleep(0.1)
	
time.sleep(5)
lcd_fill(0x0000)






