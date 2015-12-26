// ************ SPI TEST APP **************
// Test bcm - SPI transmission controlled by bcm2835 library (http://www.airspayce.com/mikem/bcm2835/index.html) - this is the fastest routine
// Language: C (not C++ like previous tests)
// ----------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <cstring>
//#include <iostream>
#include <errno.h> // error handling
#include <sys/time.h> // for delay function
#include <stdint.h> // aliases for int types (unsigned char = uint8_t, etc...)
#include <unistd.h>
// for spi
#include <fcntl.h> // file control options
#include <sys/ioctl.h> // I/O control routines ( ioctl() function)
//#include <linux/spi/spidev.h> // SPI options
#include <bcm2835.h>

//#define _DEBUG_

int delayus(int us) {
	struct timespec tim, timr;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)(us * 1000L);
	
	return nanosleep(&tim, &timr);
}

int delayms(int ms) {
	struct timespec tim, timr;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)(ms * 1000000L);
	
	return nanosleep(&tim, &timr);
}

int delays(int s) {
	struct timespec tim, timr;
	tim.tv_sec = s;
	tim.tv_nsec = 0;
	
	return nanosleep(&tim, &timr);
}


/* ************************************************************
	BASIC SPI OPERATIONS 
   ************************************************************ */

/*
	Name: spi_open
	Description: Init bcm2835 lib & config SPI . Print to stdout bcm2835 lib version
	Parameters:
	Returns:
		0 - on success, non-zero - fail
*/
int spi_open(void) {
	int r;
	uint32_t v;
	
	r = bcm2835_init(); // return 0 on fail
	if (r != 1) return -1;
	
	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // MSB bit goes first
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); // mode 0
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8); // SPI_CLK 31.25MHz - MAX
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1); // CS1 - for now, with TP CS0 will be used too
	// setup both SPI.CSx
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // CS0 - active Low
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW); // CS1 - active Low
	
	v = bcm2835_version();
	fprintf(stdout, "bcm2835 library version: %u (0x%08x)\n", v,v);
	
	return 0; // OK!
	
}

/*
	Name: spi_close
	Description: De-initialize bcm2835 lib
	Parameters:
	Returns:
		0 - on success, non-zero - fail
*/
int spi_close(void) {
	
	bcm2835_spi_end();
	int r = bcm2835_close();
	if (r != 1) {
		return -1; // can't close
	}
	
	#ifdef _DEBUG_
		fprintf(stdout, "BCM2835_SPI.CLOSE: OK.\n");
	#endif
	
	return r;
}

/*
	Name: spi_transmit
	Description: Send data over SPI, and receive at the same time. Received data is put into transmitted data buffer.
	Parameters:
		1. devsel : int - selects which device to communicate - 0=touch panel , 1=lcd
		2. data : pointer(array) uint8_t - data to be sent, and buffer for data to be received
		3. len : int - bytes count to be sent from buffeer
	Returns:
		0 - on success, negative - fail, see function content for error return values. Also check errno for more information.
		positive - number of bytes received
		
*/
int spi_transmit(int devsel, uint8_t *data, int len) {

	if (devsel == 0) {
		bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
		#ifdef _DEBUG_
			fprintf(stdout, "spi_transmit.CS=CS0 , ");
		#endif
	} else {
		bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
		#ifdef _DEBUG_
			fprintf(stdout, "spi_transmit.CS=CS1 , ");
		#endif
	}
	
	#ifdef _DEBUG_
		fprintf(stdout, "data[%i]=",len);
		for(int i=0;i<len;i++) {
			uint8_t t = *(data+i);
			fprintf(stdout,"%02X ",t);
		}
		fprintf(stdout, "\n");
	#endif

	
	bcm2835_spi_transfern((char*)data, len);
	
	return len;
}


/* **********************************************************************************
	LCD ROUTINES
   ********************************************************************************** */

// check spi_init() function
//#define LCD_SPI_DEVICE "/dev/spidev0.1"
//#define LCD_SPI_MODE SPI_MODE_0
//#define LCD_SPI_SPEED 35000000
//#define LCD_SPI_BITS_PER_WORD 8
//#define SPI_DELAY_USECS 0

#define LCD_CS 1
#define TOUCH_CS 0

#define LCD_WIDTH 480
#define LCD_HEIGHT 320
   
void lcd_reset(void) {
	uint8_t buff[4] = { 0,0,0,0 };
	
	#ifdef _DEBUG_
		printf("LCD_RESET\n");
	#endif	
	
	// Select LCD
	//bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
	
	// set Reset LOW
	spi_transmit(LCD_CS, &buff[0], 4);
	
	delayms(50);
	
	// set Reset High
	buff[0]=buff[1]=buff[2] = 0;
	buff[3]=0x01;
	spi_transmit(LCD_CS, &buff[0], 4);
	
	#ifdef _DEBUG_
		printf("LCD_RESET end.\n");
	#endif	
	
	delayms(200);
}	

void lcd_data(uint16_t data) {
	uint8_t b1[4], b2[4];
	
	memset(&b1,0,sizeof(b1));
	memset(&b2,0,sizeof(b2));
	
	// setup buffers
	b2[1] = b1[1] =  data>>8;
	b2[2] = b1[2] = data&0x00ff;
	b1[3] = 0x15; // 0x15 - DATA_BE const from ili9341.c (BE is short form "before")
	b2[3] = 0x1F; // 0x1F - DATA_AF const from ili9341.c (AF is short form "after")
	
	// Select LCD
	//bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
	// send it - prepare
	spi_transmit(LCD_CS, &b1[0], 4);//bcm2835_spi_transfern(&b1, 4);
	// send it - store in LCD
	spi_transmit(LCD_CS, &b2[0], 4); //bcm2835_spi_transfern(&b2, 4);
	
}

void lcd_cmd(uint16_t cmd) {
	uint8_t b1[4], b2[4];
	
	memset(&b1,0,sizeof(b1));
	memset(&b2,0,sizeof(b2));
	
	// setup buffers
	b2[1] = b1[1] =  cmd>>8;
	b2[2] = b1[2] = cmd&0x00ff;
	b1[3] = 0x11; // 0x11 - CMD_BE const from ili9341.c (BE is short form "before")
	b2[3] = 0x1B; // 0x1B - CMD_AF const from ili9341.c (AF is short form "after")
	
	// Select LCD
	//bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
	// send it - prepare
	spi_transmit(LCD_CS, &b1[0], 4);//bcm2835_spi_transfern(&b1, 4);
	// send it - store in LCD
	spi_transmit(LCD_CS, &b2[0], 4); //bcm2835_spi_transfern(&b2, 4);
	
}

void lcd_setptr(void) {
	lcd_cmd(0x002b);
	lcd_data(0x0000); 
	lcd_data(0x0000); // 0
	lcd_data(0x0001);
	lcd_data(0x003f); //319
	
	lcd_cmd(0x002a);
	lcd_data(0x0000);
	lcd_data(0x0000); // 0
	lcd_data(0x0001);
	lcd_data(0x00df); // 479
	
	lcd_cmd(0x002c);
}
	
void lcd_setarea(uint16_t x, uint16_t y) {
	lcd_cmd(0x002b);
	lcd_data(y>>8);
	lcd_data(0x00ff&y);
	lcd_data(0x0001);
	lcd_data(0x003f);

	lcd_cmd(0x002a);
	lcd_data(x>>8) ;
	lcd_data(0x00ff&x) ;
	lcd_data(0x0001);
	lcd_data(0x00df);
	lcd_cmd(0x002c);
}

void lcd_setarea2(uint16_t sx, uint16_t sy, uint16_t x, uint16_t y) {
	
	if (sx>479)	sx=0;
	if (sy>319) sy=0;
	if (x>479) x=479;
	if (y>319) y=319;
	
	lcd_cmd(0x002b);
	lcd_data(sy>>8) ;
	lcd_data(0x00ff&sy);
	lcd_data(y>>8);
	lcd_data(0x00ff&y);

	lcd_cmd(0x002a);
	lcd_data(sx>>8) ;
	lcd_data(0x00ff&sx) ;
	lcd_data(x>>8);
	lcd_data(0x00ff&x);
	
	lcd_cmd(0x002c);
}

void lcd_fill(uint16_t color565) {
	lcd_setptr();
	for(int x=0; x<153601;x++) {
		lcd_data(color565);
	}
}

void lcd_fill2(uint16_t sx, uint16_t sy, uint16_t x, uint16_t y, uint16_t color565) {
	uint16_t tmp=0;
	int cnt;
	if (sx>479) sx=0;
	if (sy>319) sy=0;
	if (x>479)  x=479;
	if (y>319)  y=319;
	
	if (sx>x) {
		tmp=sx;
		sx=x;
		x=tmp;
	}
	
	if (sy>y) {
		tmp=sy;
		sy=y;
		y=tmp;
	}
	
	cnt = (y-sy) * (x-sx);
	lcd_setarea2(sx,sy,x,y);
	for(int t=0;t<cnt;t++) {
		lcd_data(color565);
	}	
}

void lcd_init(void) {
	lcd_reset();
	delayms(100);
	lcd_cmd(0x0000);
	delayms(1);

	lcd_cmd(0x00B0);
	lcd_data(0x0000);
	lcd_cmd(0x0011);
	delayms(50); //mdelay(50);

	lcd_cmd(0x00B3);
	lcd_data(0x0002);
	lcd_data(0x0000);
	lcd_data(0x0000);
	lcd_data(0x0000);

	lcd_cmd(0x00C0);
	lcd_data(0x0010);//13
	lcd_data(0x003B);//480
	lcd_data(0x0000);
	lcd_data(0x0002);
	lcd_data(0x0000);
	lcd_data(0x0001);
	lcd_data(0x0000);//NW
	lcd_data(0x0043);

	lcd_cmd(0x00C1);
	lcd_data(0x0008);//w_data(0x0008);
	lcd_data(0x0016);//w_data(0x0016);//CLOCK
	lcd_data(0x0008);
	lcd_data(0x0008);

	lcd_cmd(0x00C4);
	lcd_data(0x0011);
	lcd_data(0x0007);
	lcd_data(0x0003);
	lcd_data(0x0003);

	lcd_cmd(0x00C6);
	lcd_data(0x0000);

	lcd_cmd(0x00C8); //GAMMA
	lcd_data(0x0003);
	lcd_data(0x0003);
	lcd_data(0x0013);
	lcd_data(0x005C);
	lcd_data(0x0003);
	lcd_data(0x0007);
	lcd_data(0x0014);
	lcd_data(0x0008);
	lcd_data(0x0000);
	lcd_data(0x0021);
	lcd_data(0x0008);
	lcd_data(0x0014);
	lcd_data(0x0007);
	lcd_data(0x0053);
	lcd_data(0x000C);
	lcd_data(0x0013);
	lcd_data(0x0003);
	lcd_data(0x0003);
	lcd_data(0x0021);
	lcd_data(0x0000);

	lcd_cmd(0x0035);
	lcd_data(0x0000);

	lcd_cmd(0x0036);  
	lcd_data(0x0028);

	lcd_cmd(0x003A);
	lcd_data(0x0055); //55 lgh

	lcd_cmd(0x0044);
	lcd_data(0x0000);
	lcd_data(0x0001);

	lcd_cmd(0x00B6);
	lcd_data(0x0000);
	lcd_data(0x0002); //220 GS SS SM ISC[3:0]
	lcd_data(0x003B);

	lcd_cmd(0x00D0);
	lcd_data(0x0007);
	lcd_data(0x0007); //VCI1
	lcd_data(0x001D); //VRH

	lcd_cmd(0x00D1);
	lcd_data(0x0000);
	lcd_data(0x0003); //VCM
	lcd_data(0x0000); //VDV

	lcd_cmd(0x00D2);
	lcd_data(0x0003);
	lcd_data(0x0014);
	lcd_data(0x0004);



	lcd_cmd(0xE0);  
	lcd_data(0x1f);  
	lcd_data(0x2C);  
	lcd_data(0x2C);  
	lcd_data(0x0B);  
	lcd_data(0x0C);  
	lcd_data(0x04);  
	lcd_data(0x4C);  
	lcd_data(0x64);  
	lcd_data(0x36);  
	lcd_data(0x03);  
	lcd_data(0x0E);  
	lcd_data(0x01);  
	lcd_data(0x10);  
	lcd_data(0x01);  
	lcd_data(0x00);  

	lcd_cmd(0XE1);  
	lcd_data(0x1f);  
	lcd_data(0x3f);  
	lcd_data(0x3f);  
	lcd_data(0x0f);  
	lcd_data(0x1f);  
	lcd_data(0x0f);  
	lcd_data(0x7f);  
	lcd_data(0x32);  
	lcd_data(0x36);  
	lcd_data(0x04);  
	lcd_data(0x0B);  
	lcd_data(0x00);  
	lcd_data(0x19);  
	lcd_data(0x14);  
	lcd_data(0x0F);  

	lcd_cmd(0xE2);
	lcd_data(0x0f);
	lcd_data(0x0f);

	lcd_data(0x0f);

	lcd_cmd(0xE3);
	lcd_data(0x0f);
	lcd_data(0x0f);

	lcd_data(0x0f);

	lcd_cmd(0x13);

	lcd_cmd(0x0029);
	delayms(20); //mdelay(20);

	lcd_cmd(0x00B4);
	lcd_data(0x0000);
	delayms(20); //mdelay(20);
	lcd_cmd(0x002C);
	lcd_cmd(0x002A); 
	lcd_data(0x0000);
	lcd_data(0x0000);

	lcd_data(0x0001);
	lcd_data(0x000dF);

	lcd_cmd(0x002B);  
	lcd_data(0x0000);
	lcd_data(0x0000);
	lcd_data(0x0001);
	lcd_data(0x003f); 

	lcd_cmd(0x002c); 
}


uint64_t get_ticks(void) {
	struct timeval t;
	gettimeofday(&t, NULL);
	
	return (uint64_t)( (uint64_t)(t.tv_sec*1000000) + t.tv_usec);
}


int main(int argc, char **argv)
{
	int r;
	uint64_t td_b, td_e, td;
	uint64_t total_time_demo_start, total_time_demo_end;
	
	r = spi_open();
	if (r < 0) {
		fprintf(stderr, "Unable to open bmc2835 lib & config SPI bus.\n");
		return 1;
	} else {
		fprintf(stdout, "bcm2835 library & SPI configured.\n");
	}
	
	total_time_demo_start = get_ticks();
	
	fprintf(stdout, "LCD init. ");
	td_b = get_ticks();
	lcd_init();
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	
	fprintf(stdout, "Fill black. ");
	td_b = get_ticks();
	lcd_fill(0x0000);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout, "Fill red. ");
	td_b = get_ticks();
	lcd_fill(0xF800);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout, "Fill green. ");
	td_b = get_ticks();
	lcd_fill(0x07E0);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout, "Fill blue. ");
	td_b = get_ticks();
	lcd_fill(0x001F);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	
	fprintf(stdout, "Fill white. ");
	td_b = get_ticks();
	lcd_fill(0xffff);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout, "Fill black. ");
	td_b = get_ticks();
	lcd_fill(0x0000);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	
	total_time_demo_end = get_ticks();
	td = (uint64_t)(total_time_demo_end - total_time_demo_start);
	fprintf(stdout,"\nTotal LCD test time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	
	spi_close();
	fprintf(stdout, "bcm2835 library closed.\n");
	
	return 0;
}