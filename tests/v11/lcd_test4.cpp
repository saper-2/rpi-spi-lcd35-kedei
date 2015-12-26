// ************ SPI TEST APP **************
// Test 4 - SPI transmission directly from lcd_cmd & lcd_data in one call ioctl with buff of 2 messages.
// ----------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <iostream>
#include <errno.h> // error handling
#include <sys/time.h> // for delay function
#include <stdint.h> // aliases for int types (unsigned char = uint8_t, etc...)
#include <unistd.h>
// for spi
#include <fcntl.h> // file control options
#include <sys/ioctl.h> // I/O control routines ( ioctl() function)
#include <linux/spi/spidev.h> // SPI options

//#define _DEBUG_

#define LCD_SPI_DEVICE "/dev/spidev0.1"
#define LCD_SPI_MODE SPI_MODE_0
#define LCD_SPI_SPEED 24000000
#define LCD_SPI_BITS_PER_WORD 8
#define SPI_DELAY_USECS 0

#define LCD_WIDTH 480
#define LCD_HEIGHT 320


int delayus(int us) {
	struct timespec tim, timr;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)(us * 1000);
	
	return nanosleep(&tim, &timr);
}

int delayms(int ms) {
	struct timespec tim, timr;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)(ms * 1000000);
	
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
	Description: Open SPI device and configure mode
	Parameters:
		1. h : pointer int32 - return device handle
		2. spidev : string - spi device
		3. mode : uint8_t - spi mode
		4. bits : uint8_t - bits per word (normally 8)
		5. speed : uint32_t - SPI clk speed in MHz
	Returns:
		0 - on success, non-zero - fail, see function content for error return values. Alos check errno for more information.
*/
int spi_open(int *h, std::string spidev, uint8_t mode, uint8_t bits, uint32_t speed) {
	int r;
	uint8_t t8;
	uint32_t t32;
	//*h = 0; // reset device handle
	
	// open spi device
	*h = open(spidev.c_str(), O_RDWR); // open spi device for R/W
	if (*h < 0) { 
		// can't open
		*h = 0;
		return -1; // -1 for unable to open device
	}
	
	#ifdef _DEBUG_
		fprintf(stdout, "SPI.SETUP: OPEN=%s , MODE=%u , BITS_PER_WORD=%u , SPI_CLK_SPEED_HZ=%u \n", spidev.c_str(), mode, bits, speed);
	#endif
	
	// setup SPI mode
	r = ioctl(*h, SPI_IOC_WR_MODE, &mode); // set mode
	if (r < 0) {
		return -2; // -2 for unable to set SPI mode
	}
	
	r = ioctl(*h, SPI_IOC_RD_MODE, &t8); // read mode
	if (r < 0) {
		return -3;
	}
	
	if (t8 != mode) {
		fprintf(stderr, "SPI_SETUP(-4): Mode check fail. Set 0x%X but got 0x%X\n", mode, t8);
		return -4; // mode mismatch
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: Mode set." << std::endl;
	#endif
	
	
	// set bits per word
	r = ioctl(*h, SPI_IOC_WR_BITS_PER_WORD, &bits); // set bits
	if (r < 0) {
		return -5;
	}
	
	r = ioctl(*h, SPI_IOC_RD_BITS_PER_WORD, &t8); // read bits
	if (r < 0) {
		return -6;
	}
	
	if (t8 != bits) {
		fprintf(stderr, "SPI_SETUP(-7): Bits per word check fail. Set 0x%X but got 0x%X\n", bits, t8);
		return -7; // mode mismatch
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: Bits per word set." << std::endl;
	#endif
	
	
	// set SPI clock speed
	r = ioctl(*h, SPI_IOC_WR_MAX_SPEED_HZ, &speed); // set bits
	if (r < 0) {
		return -8;
	}
	
	r = ioctl(*h, SPI_IOC_RD_MAX_SPEED_HZ, &t32); // read bits
	if (r < 0) {
		return -9;
	}
	
	if (t32 != speed) {
		fprintf(stderr, "SPI_SETUP(-10): SPI Clock check fail. Set %d but got %d\n", speed, t32);
		return -10; // mode mismatch
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: Clock speed set." << std::endl;
	#endif
	
	
	return 0; // OK!
	
}

/*
	Name: spi_close
	Description: Close SPI device
	Parameters:
		1. h : pointer int32 - opened device handle. After closing handle is reset to 0.
	Returns:
		0 - on success, non-zero - fail, see function content for error return values. Alos check errno for more information.
*/
int spi_close(int *h) {
	if (*h == 0) return 0; // closed, or alredy closed
	
	int r = close(*h);
	if (r < 0) {
		return -1; // can't close
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.CLOSE: OK." << std::endl;
	#endif
	
	*h=0; // rest device handle
	return r;
}

/*
	Name: spi_transmit
	Description: Send data over SPI, and receive at the same time. Received data is put into transmitted data buffer.
	Parameters:
		1. h : pointer int32 - opened device handle. After closing handle is reset to 0.
		2. data : pointer(array) uint8_t - data to be sent, and buffer for data to be received
		3. len : int - bytes count to be sent from buffeer
		4. spi_speed : uint32_t - spi speed override for transfer
		5 . spi_bits : uin8_t - spi bits per word override for transfer
	Returns:
		0 - on success, negative - fail, see function content for error return values. Alos check errno for more information.
		positive - number of bytes received
		
*/
int spi_transmit(int *h, uint8_t *data, int len, uint32_t spi_speed, uint8_t spi_bits) {
	int r;
	
	struct spi_ioc_transfer buf;
	
	if (*h == 0) return -1; // device not opened
	
	memset(&buf, 0, sizeof(buf));
	buf.tx_buf = (uint64_t)data;
	//buf.rx_buf = (uint64_t)data;
	buf.len = len;
	buf.speed_hz = spi_speed;
	buf.bits_per_word = spi_bits;
	buf.delay_usecs = 0;
	buf.cs_change = 0;
	
	#if defined(_DEBUG_)
		std::cout << "SPI TRANSMIT(" << len << ") data=";
		for (int i=0;i<len;i++) printf("%02x ",*(data+i));
		std::cout << " - SPI_SPEED=" << spi_speed << " BITS_PER_WORD=" << ((int)spi_bits) << " ... ";
	#endif
	
	//buf[len-1].cs_change=1;
	// send it
	r = ioctl(*h, SPI_IOC_MESSAGE(1), &buf);	
	//r = ioctl(*h, SPI_IOC_MESSAGE(len), &buf);	
	
	if (r < 0) {
		#if defined(_DEBUG_)
			std::cout << "ERROR" << std::endl;
			fprintf(stderr, "   Return=%d errno=%d str=%s", r, errno, strerror(errno));
			std::cout << std::endl;
		#else 
			fprintf(stderr, "SPI.TRANSMIT ERRROR (%d,%d) : %s", r, errno, strerror(errno));
		#endif
		return -2;
	}
	
	#if defined(_DEBUG_)
		std::cout << "OK";
		std::cout << std::endl;
	#endif
	
	return r;	
}


/* **********************************************************************************
	LCD ROUTINES
   ********************************************************************************** */   
void lcd_reset(int *spih) {
	uint8_t buff[4] = { 0,0,0,0 };
	uint8_t r;
	
	#ifdef _DEBUG_
		printf("LCD_RESET\n");
	#endif	
	
	// set Reset LOW
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_RESET_1 error (%d) : %s", errno, strerror(errno));
	}
	
	delayms(50);
	
	// set Reset High
	buff[0]=buff[1]=buff[2] = 0;
	buff[3]=0x01;
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_RESET_2 error (%d) : %s", errno, strerror(errno));
	}
	
	#ifdef _DEBUG_
		printf("LCD_RESET end.\n");
	#endif	
	
	
	delayms(200);
}	

#define SPI_DELAY_USECS_DATA SPI_DELAY_USECS
void lcd_data(int *spih, uint16_t data) {
	uint8_t b1[4], b2[4];
	struct spi_ioc_transfer buf[2];
	int r;
	
	if (*spih == 0) {
		fprintf(stderr, "LCD.DATA - SPI not opened.");
		return; // device not opened
	}
	
	memset(&b1,0,sizeof(b1));
	memset(&b2,0,sizeof(b2));
	memset(&buf,0,sizeof(buf));
	
	// setup buffers
	b2[1] = b1[1] =  data>>8;
	b2[2] = b1[2] = data&0x00ff;
	b1[3] = 0x15; // 0x15 - DATA_BE const from ili9341.c (BE is short form "before")
	b2[3] = 0x1F; // 0x1F - DATA_AF const from ili9341.c (AF is short form "after")
	
	buf[0].tx_buf = (uint64_t)b1;
	buf[0].len = sizeof(b1);
	buf[0].speed_hz = LCD_SPI_SPEED;
	buf[0].bits_per_word = LCD_SPI_BITS_PER_WORD;
	buf[0].delay_usecs = SPI_DELAY_USECS_DATA;
	buf[0].cs_change = 1;
	
	buf[1].tx_buf = (uint64_t)b2;
	buf[1].len = sizeof(b2);
	buf[1].speed_hz = LCD_SPI_SPEED;
	buf[1].bits_per_word = LCD_SPI_BITS_PER_WORD;
	buf[1].delay_usecs = SPI_DELAY_USECS_DATA;
	
	
	#if defined(_DEBUG_)
		std::cout << "SPI TRANSMIT(" << len << ") data=";
		for (int i=0;i<len;i++) printf("%02x ",*(data+i));
		std::cout << " - SPI_SPEED=" << spi_speed << " BITS_PER_WORD=" << ((int)spi_bits) << " ... ";
	#endif
	
	//buf[len-1].cs_change=1;
	// send it
	r = ioctl(*spih, SPI_IOC_MESSAGE(2), &buf);	
	
	if (r < 0) {
		#if defined(_DEBUG_)
			std::cout << "ERROR" << std::endl;
			fprintf(stderr, "   Return=%d errno=%d str=%s", r, errno, strerror(errno));
			std::cout << std::endl;
		#else 
			fprintf(stderr, "LCD.DATA.SPI.TRANSMIT ERRROR (%d,%d) : %s", r, errno, strerror(errno));
		#endif
		//return -2;
	}
	
	#if defined(_DEBUG_)
		std::cout << "OK";
		std::cout << std::endl;
	#endif
	
	//return r;	
}

#define SPI_DELAY_USECS_CMD SPI_DELAY_USECS
void lcd_cmd(int *spih, uint16_t cmd) {
	uint8_t b1[4], b2[4];
	struct spi_ioc_transfer buf[2];
	int r;
	
	if (*spih == 0) {
		fprintf(stderr, "LCD.CMD - SPI not opened.");
		return; // device not opened
	}
	
	memset(&b1,0,sizeof(b1));
	memset(&b2,0,sizeof(b2));
	memset(&buf,0,sizeof(buf));
	
	// setup buffers
	b2[1] = b1[1] =  cmd>>8;
	b2[2] = b1[2] = cmd&0x00ff;
	b1[3] = 0x11; // 0x11 - CMD_BE const from ili9341.c (BE is short form "before")
	b2[3] = 0x1B; // 0x1B - CMD_AF const from ili9341.c (AF is short form "after")
	
	
	buf[0].tx_buf = (uint64_t)b1;
	//buf.rx_buf = (*uint64_t)data;
	buf[0].len = sizeof(b1);
	buf[0].speed_hz = LCD_SPI_SPEED;
	buf[0].bits_per_word = LCD_SPI_BITS_PER_WORD;
	buf[0].delay_usecs = SPI_DELAY_USECS_CMD;
	buf[0].cs_change = 1;
	
	buf[1].tx_buf = (uint64_t)b2;
	//buf.rx_buf = (*uint64_t)data;
	buf[1].len = sizeof(b2);
	buf[1].speed_hz = LCD_SPI_SPEED;
	buf[1].bits_per_word = LCD_SPI_BITS_PER_WORD;
	buf[1].delay_usecs = SPI_DELAY_USECS_CMD;
	
	
	#if defined(_DEBUG_)
		std::cout << "SPI TRANSMIT(" << len << ") data=";
		for (int i=0;i<len;i++) printf("%02x ",*(data+i));
		std::cout << " - SPI_SPEED=" << LCD_SPI_SPEED << " BITS_PER_WORD=" << ((int)LCD_SPI_BITS_PER_WORD) << " ... ";
	#endif
	
	//buf[len-1].cs_change=1;
	// send it
	r = ioctl(*spih, SPI_IOC_MESSAGE(2), &buf);	
	
	if (r < 0) {
		#if defined(_DEBUG_)
			std::cout << "ERROR" << std::endl;
			fprintf(stderr, "   Return=%d errno=%d str=%s", r, errno, strerror(errno));
			std::cout << std::endl;
		#else 
			fprintf(stderr, "LCD.CMD.SPI.TRANSMIT ERRROR (%d,%d) : %s", r, errno, strerror(errno));
		#endif
		//return -2;
	}
	
	#if defined(_DEBUG_)
		std::cout << "OK";
		std::cout << std::endl;
	#endif
}

void lcd_setptr(int *spih) {
	lcd_cmd(spih, 0x002b);
	lcd_data(spih, 0x0000); 
	lcd_data(spih, 0x0000); // 0
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x003f); //319
	
	lcd_cmd(spih, 0x002a);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000); // 0
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x00df); // 479
	
	lcd_cmd(spih, 0x002c);
}
	
void lcd_setarea(int *spih, uint16_t x, uint16_t y) {
	lcd_cmd(spih, 0x002b);
	lcd_data(spih, y>>8);
	lcd_data(spih, 0x00ff&y);
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x003f);

	lcd_cmd(spih, 0x002a);
	lcd_data(spih, x>>8) ;
	lcd_data(spih, 0x00ff&x) ;
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x00df);
	lcd_cmd(spih, 0x002c);
}

void lcd_setarea2(int *spih, uint16_t sx, uint16_t sy, uint16_t x, uint16_t y) {
	
	if (sx>479)	sx=0;
	if (sy>319) sy=0;
	if (x>479) x=479;
	if (y>319) y=319;
	
	lcd_cmd(spih, 0x002b);
	lcd_data(spih, sy>>8) ;
	lcd_data(spih, 0x00ff&sy);
	lcd_data(spih, y>>8);
	lcd_data(spih, 0x00ff&y);

	lcd_cmd(spih, 0x002a);
	lcd_data(spih, sx>>8) ;
	lcd_data(spih, 0x00ff&sx) ;
	lcd_data(spih, x>>8);
	lcd_data(spih, 0x00ff&x);
	
	lcd_cmd(spih, 0x002c);
}

void lcd_fill(int *spih, uint16_t color565) {
	lcd_setptr(spih);
	for(int x=0; x<153601;x++) {
		lcd_data(spih, color565);
	}
}

void lcd_fill2(int *spih, uint16_t sx, uint16_t sy, uint16_t x, uint16_t y, uint16_t color565) {
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
	lcd_setarea2(spih, sx,sy,x,y);
	for(int t=0;t<cnt;t++) {
		lcd_data(spih, color565);
	}	
}

void lcd_init(int *spih) {
	lcd_reset(spih);
	delayms(100);
	lcd_cmd(spih, 0x0000);
	delayms(1);

	lcd_cmd(spih, 0x00B0);
	lcd_data(spih, 0x0000);
	lcd_cmd(spih, 0x0011);
	delayms(50); //mdelay(50);

	lcd_cmd(spih, 0x00B3);
	lcd_data(spih, 0x0002);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x00C0);
	lcd_data(spih, 0x0010);//13
	lcd_data(spih, 0x003B);//480
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0002);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x0000);//NW
	lcd_data(spih, 0x0043);

	lcd_cmd(spih, 0x00C1);
	lcd_data(spih, 0x0008);//w_data(0x0008);
	lcd_data(spih, 0x0016);//w_data(0x0016);//CLOCK
	lcd_data(spih, 0x0008);
	lcd_data(spih, 0x0008);

	lcd_cmd(spih, 0x00C4);
	lcd_data(spih, 0x0011);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0003);

	lcd_cmd(spih, 0x00C6);
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x00C8); //GAMMA
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0013);
	lcd_data(spih, 0x005C);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0014);
	lcd_data(spih, 0x0008);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0021);
	lcd_data(spih, 0x0008);
	lcd_data(spih, 0x0014);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0053);
	lcd_data(spih, 0x000C);
	lcd_data(spih, 0x0013);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0021);
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x0035);
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x0036);  
	lcd_data(spih, 0x0028);

	lcd_cmd(spih, 0x003A);
	lcd_data(spih, 0x0055); //55 lgh

	lcd_cmd(spih, 0x0044);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0001);

	lcd_cmd(spih, 0x00B6);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0002); //220 GS SS SM ISC[3:0]
	lcd_data(spih, 0x003B);

	lcd_cmd(spih, 0x00D0);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0007); //VCI1
	lcd_data(spih, 0x001D); //VRH

	lcd_cmd(spih, 0x00D1);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0003); //VCM
	lcd_data(spih, 0x0000); //VDV

	lcd_cmd(spih, 0x00D2);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0014);
	lcd_data(spih, 0x0004);



	lcd_cmd(spih, 0xE0);  
	lcd_data(spih, 0x1f);  
	lcd_data(spih, 0x2C);  
	lcd_data(spih, 0x2C);  
	lcd_data(spih, 0x0B);  
	lcd_data(spih, 0x0C);  
	lcd_data(spih, 0x04);  
	lcd_data(spih, 0x4C);  
	lcd_data(spih, 0x64);  
	lcd_data(spih, 0x36);  
	lcd_data(spih, 0x03);  
	lcd_data(spih, 0x0E);  
	lcd_data(spih, 0x01);  
	lcd_data(spih, 0x10);  
	lcd_data(spih, 0x01);  
	lcd_data(spih, 0x00);  

	lcd_cmd(spih, 0XE1);  
	lcd_data(spih, 0x1f);  
	lcd_data(spih, 0x3f);  
	lcd_data(spih, 0x3f);  
	lcd_data(spih, 0x0f);  
	lcd_data(spih, 0x1f);  
	lcd_data(spih, 0x0f);  
	lcd_data(spih, 0x7f);  
	lcd_data(spih, 0x32);  
	lcd_data(spih, 0x36);  
	lcd_data(spih, 0x04);  
	lcd_data(spih, 0x0B);  
	lcd_data(spih, 0x00);  
	lcd_data(spih, 0x19);  
	lcd_data(spih, 0x14);  
	lcd_data(spih, 0x0F);  

	lcd_cmd(spih, 0xE2);
	lcd_data(spih, 0x0f);
	lcd_data(spih, 0x0f);

	lcd_data(spih, 0x0f);

	lcd_cmd(spih, 0xE3);
	lcd_data(spih, 0x0f);
	lcd_data(spih, 0x0f);

	lcd_data(spih, 0x0f);

	lcd_cmd(spih, 0x13);

	lcd_cmd(spih, 0x0029);
	delayms(20); //mdelay(20);

	lcd_cmd(spih, 0x00B4);
	lcd_data(spih, 0x0000);
	delayms(20); //mdelay(20);
	lcd_cmd(spih, 0x002C);
	lcd_cmd(spih, 0x002A); 
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);

	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x000dF);

	lcd_cmd(spih, 0x002B);  
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x003f); 

	lcd_cmd(spih, 0x002c); 
}


uint64_t get_ticks(void) {
	struct timeval t;
	gettimeofday(&t, NULL);
	
	return (uint64_t)( (uint64_t)(t.tv_sec*1000000) + t.tv_usec);
}


int main(int argc, char **argv)
{
	std::string dev = LCD_SPI_DEVICE;
	int spi;
	int r;
	uint64_t td_b, td_e, td;
	uint64_t total_time_demo_start, total_time_demo_end;
	
	r = spi_open(&spi, dev, LCD_SPI_MODE, LCD_SPI_BITS_PER_WORD, LCD_SPI_SPEED);
	if (r < 0) {
		fprintf(stderr, "Unable to open SPI bus 0.1 , error (%d,%d) : %s", r, errno, strerror(errno));
		return 1;
	} else {
		std::cout << "SPI 0.1 open." << std::endl;
	}
	
	total_time_demo_start = get_ticks();
	
	fprintf(stdout,"LCD init. ");
	td_b = get_ticks();
	lcd_init(&spi);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	
	fprintf(stdout,"Fill black. ");
	td_b = get_ticks();
	lcd_fill(&spi, 0x0000);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout,"Fill red. ");
	td_b = get_ticks();
	lcd_fill(&spi, 0xF800);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout,"Fill green. ");
	td_b = get_ticks();
	lcd_fill(&spi, 0x07E0);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout,"Fill blue. ");
	td_b = get_ticks();
	lcd_fill(&spi, 0x001F);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	
	fprintf(stdout,"Fill white. ");
	td_b = get_ticks();
	lcd_fill(&spi, 0xffff);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	delayms(500);
	
	fprintf(stdout,"Fill black. ");
	td_b = get_ticks();
	lcd_fill(&spi, 0x0000);
	td_e = get_ticks();
	td = (uint64_t)(td_e - td_b);
	fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	
	total_time_demo_end = get_ticks();
	td = (uint64_t)(total_time_demo_end - total_time_demo_start);
	fprintf(stdout,"\nTotal LCD test time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	
	r = spi_close(&spi);
	std::cout << "SPI 0.1 closed. (" << ((int)r) << ")" << std::endl;
	return 0;
}