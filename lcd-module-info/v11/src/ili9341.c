/*
 * ili9341 Framebuffer
 *
 * ToDo: Fix this text vv
 *
 * Original: Copyright (c) 2009 Jean-Christian de Rivaz
 *
 * Console support, 320x240 instead of 240x320:
 * Bits and pieces borrowed from the fsl-ili9341.c:
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 * Author: kedei<310953417@qq.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * The Solomon Systech ili9341 chip drive TFT screen up to 240x320. 
 *
 * For direct I/O-mode:
 *
 * This driver expect the SSD1286 to be connected to a 16 bits local bus
 * and to be set in the 16 bits parallel interface mode. To use it you must
 * define in your board file a struct platform_device with a name set to
 * "ili9341" and a struct resource array with two IORESOURCE_MEM: the first
 * for the control register; the second for the data register.
 *
 *
 * LCDs in their own, native SPI mode aren't supported yet, mostly because I 
 * can't get my hands on a cheap one.
 */

//#define DEBUG

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <mach/platform.h>
#include <linux/input/mt.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <mach/irqs.h>
#include <linux/irq.h>



#define UPSIDEDOWN


static volatile bool lcd_bool = true;
static volatile bool tsc_bool = false;
static unsigned char value_j = 0;

struct ili9341_page {
	unsigned short x;
	unsigned short y;
	unsigned short *buffer;
	unsigned short *oldbuffer;
	unsigned short len;
	int must_update;
};

struct ili9341 {
	struct device *dev;
	struct fb_info *info;
	unsigned int pages_count;
	struct ili9341_page *pages;
	unsigned long pseudo_palette[17];
	int backlight;
};

struct ts_event {
	 int	x;
	 int	y;
	 long	z1, z2;
	 int 	flag;
};
static struct ts_event tsc;

static void __iomem *lcd_spi_base;


#define GPIOSET(no, ishigh)	{ if (ishigh) set|=(1<<no); else reset|=(1<<no); } while(0)




#define  TS_START	0X80
#define  TS_X		0XD3
#define  TS_Y		0X93
#define  TS_Z1		0XB3
#define  TS_Z2		0XC3

#define SPI_CS			0x00
#define SPI_FIFO		0x04
#define SPI_CLK			0x08
#define SPI_DLEN		0x0c
#define SPI_LTOH		0x10
#define SPI_DC			0x14


#define SPI_CS_LEN_LONG		0x02000000
#define SPI_CS_DMA_LEN		0x01000000
#define SPI_CS_CSPOL2		0x00800000
#define SPI_CS_CSPOL1		0x00400000
#define SPI_CS_CSPOL0		0x00200000
#define SPI_CS_RXF		0x00100000
#define SPI_CS_RXR		0x00080000
#define SPI_CS_TXD		0x00040000
#define SPI_CS_RXD		0x00020000
#define SPI_CS_DONE		0x00010000
#define SPI_CS_LEN		0x00002000
#define SPI_CS_REN		0x00001000
#define SPI_CS_ADCS		0x00000800
#define SPI_CS_INTR		0x00000400
#define SPI_CS_INTD		0x00000200
#define SPI_CS_DMAEN		0x00000100
#define SPI_CS_TA		0x00000080
#define SPI_CS_CSPOL		0x00000040
#define SPI_CS_CLEAR_RX		0x00000020
#define SPI_CS_CLEAR_TX		0x00000010
#define SPI_CS_CPOL		0x00000008
#define SPI_CS_CPHA		0x00000004
#define SPI_CS_CS_10		0x00000002
#define SPI_CS_CS_01		0x00000001

#define SPI_CS_TXTX		0X000000B0

#define RESET		0X00
#define CMD_BE		0X11
#define CMD_AF		0X1B

#define DATE_BE		0X15
#define DATE_AF		0X1F


#define TSC_IRQ		25
struct input_dev *input_dev;

static u32  cs ;
static u32  tsc_cs ;

static void spi_init_pinmode(void)
{
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

	int pin;
	u32 *gpio = ioremap(GPIO_BASE, SZ_16K);

	/* SPI is on GPIO 7..11 */
	for (pin = 7; pin <= 11; pin++) {
		INP_GPIO(pin);		/* set mode to GPIO input first */
		SET_GPIO_ALT(pin, 0);	/* set mode to ALT 0 */
	}

	iounmap(gpio);

#undef INP_GPIO
#undef SET_GPIO_ALT
}

static spi_sss(unsigned char _data)
{

	
	writel( readl(lcd_spi_base+SPI_CS)|SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX,lcd_spi_base+SPI_CS);

	writel( readl(lcd_spi_base+SPI_CS)|SPI_CS_TA,lcd_spi_base+SPI_CS);

	while (!(readl(lcd_spi_base+SPI_CS)&SPI_CS_TXD));

	writel(_data,lcd_spi_base+SPI_FIFO);


	while(!(readl(lcd_spi_base+SPI_CS)&SPI_CS_DONE));

	
}

static  void spi_transform(unsigned char *spi_buff)
{	
	while(tsc_bool);
	
	lcd_bool = true;
	if(tsc_bool)
	{
		while(tsc_bool);
	}
	
	writel(cs|SPI_CS_TXTX,lcd_spi_base+SPI_CS);

	while(!(readl(lcd_spi_base+SPI_CS)&SPI_CS_TXD));
	
	writel(0x00,lcd_spi_base+SPI_FIFO);
	writel((unsigned char)*(spi_buff),lcd_spi_base+SPI_FIFO);
	writel((unsigned char)*(spi_buff+1),lcd_spi_base+SPI_FIFO);
	writel((unsigned char)*(spi_buff+2),lcd_spi_base+SPI_FIFO);

	while(!(readl(lcd_spi_base+SPI_CS)&SPI_CS_DONE));
	
	lcd_bool = false;
	
}

static inline void tsc_transform(unsigned char *spi_buff)
{

	
	writel(tsc_cs|SPI_CS_TXTX,lcd_spi_base+SPI_CS);	

	for (;!(readl(lcd_spi_base+SPI_CS)&SPI_CS_TXD););

	writel(0x00,lcd_spi_base+SPI_FIFO);
	writel((unsigned char)*(spi_buff),lcd_spi_base+SPI_FIFO);
	writel((unsigned char)*(spi_buff+1),lcd_spi_base+SPI_FIFO);
	writel((unsigned char)*(spi_buff+2),lcd_spi_base+SPI_FIFO);

	for(;!(readl(lcd_spi_base+SPI_CS)&SPI_CS_DONE););
}


static unsigned char _cmd[3],_data[3],_tsc[3];


void LCD_RESET(void)
{
	_cmd[0] = 0x00;
	_cmd[1] = 0x00;
	_cmd[2] = 0x00;
	spi_transform(_cmd);
}

void LCD_NORESET(void)
{
	_cmd[0] = 0x00;
	_cmd[1] = 0x00;
	_cmd[2] = 0x01;
	spi_transform(_cmd);
}


static inline void  cmd(unsigned  short cmd)
{
	_cmd[0] = (unsigned char)(cmd>>8);
	_cmd[1] = (unsigned char)cmd;
	_cmd[2] = CMD_BE;
	spi_transform(_cmd);
	
	_cmd[2] = CMD_AF;
	spi_transform(_cmd);
}



static inline void w_data(unsigned  short data)
{


	_data[0] = (unsigned char)(data>>8);
	_data[1] = (unsigned char)data;
	_data[2] = DATE_BE;
	spi_transform(_data);

	_data[2] = DATE_AF;
	spi_transform(_data);

	
}


static inline void w_rgb(unsigned  short data)
{

	_data[0] = (unsigned char)(data>>8);
	_data[1] = (unsigned char)data;
	_data[2] = DATE_BE;
	spi_transform(_data);

	_data[2] = DATE_AF;
	spi_transform(_data);

}





void iinntt(void)
{
    
	LCD_NORESET();
	mdelay(50);
	LCD_RESET();
	msleep(100);
	
	LCD_NORESET();
	mdelay(50);
   	cmd(0x0000);
	mdelay(10);
	
	cmd(0x00B0);
	w_data(0x0000);
	cmd(0x0011);
	mdelay(50);

	cmd(0x00B3);
	w_data(0x0002);
	w_data(0x0000);
	w_data(0x0000);
	w_data(0x0000);

	cmd(0x00C0);
	w_data(0x0010);//13
	w_data(0x003B);//480
	w_data(0x0000);
	w_data(0x0002);
	w_data(0x0000);
	w_data(0x0001);
	w_data(0x0000);//NW
	w_data(0x0043);

	cmd(0x00C1);
	w_data(0x0008);//w_data(0x0008);
	w_data(0x0016);//w_data(0x0016);//CLOCK
	w_data(0x0008);
	w_data(0x0008);

	cmd(0x00C4);
	w_data(0x0011);
	w_data(0x0007);
	w_data(0x0003);
	w_data(0x0003);

	cmd(0x00C6);
	w_data(0x0000);

	cmd(0x00C8);//GAMMA
	w_data(0x0003);
	w_data(0x0003);
	w_data(0x0013);
	w_data(0x005C);
	w_data(0x0003);
	w_data(0x0007);
	w_data(0x0014);
	w_data(0x0008);
	w_data(0x0000);
	w_data(0x0021);
	w_data(0x0008);
	w_data(0x0014);
	w_data(0x0007);
	w_data(0x0053);
	w_data(0x000C);
	w_data(0x0013);
	w_data(0x0003);
	w_data(0x0003);
	w_data(0x0021);
	w_data(0x0000);

	cmd(0x0035);
	w_data(0x0000);

	cmd(0x0036);  
	w_data(0x0028);

	cmd(0x003A);
	w_data(0x0055);//55 lgh

	cmd(0x0044);
	w_data(0x0000);
	w_data(0x0001);

	cmd(0x00B6);
	w_data(0x0000);
	w_data(0x0002);//220 GS SS SM ISC[3:0];ÆäÖÐGS SS¿ØÖÆÏÔÊŸ·œÏò£¬Í¬Ê±ÐÞžÄR36
	w_data(0x003B);

	cmd(0x00D0);
	w_data(0x0007);
	w_data(0x0007);//VCI1
	w_data(0x001D);//VRH

	cmd(0x00D1);
	w_data(0x0000);
	w_data(0x0003);//VCM
	w_data(0x0000);//VDV

	cmd(0x00D2);
	w_data(0x0003);
	w_data(0x0014);
	w_data(0x0004);



	cmd(0xE0);  
	w_data(0x1f);  
	w_data(0x2C);  
	w_data(0x2C);  
	w_data(0x0B);  
	w_data(0x0C);  
	w_data(0x04);  
	w_data(0x4C);  
	w_data(0x64);  
	w_data(0x36);  
	w_data(0x03);  
	w_data(0x0E);  
	w_data(0x01);  
	w_data(0x10);  
	w_data(0x01);  
	w_data(0x00);  

	cmd(0XE1);  
	w_data(0x1f);  
	w_data(0x3f);  
	w_data(0x3f);  
	w_data(0x0f);  
	w_data(0x1f);  
	w_data(0x0f);  
	w_data(0x7f);  
	w_data(0x32);  
	w_data(0x36);  
	w_data(0x04);  
	w_data(0x0B);  
	w_data(0x00);  
	w_data(0x19);  
	w_data(0x14);  
	w_data(0x0F);  

	cmd(0xE2);
	w_data(0x0f);
	w_data(0x0f);

	w_data(0x0f);

	cmd(0xE3);
	w_data(0x0f);
	w_data(0x0f);

	w_data(0x0f);


	cmd(0x13);


	cmd(0x0029);
	mdelay(20);

	cmd(0x00B4);
	w_data(0x0000);
	mdelay(20);
	cmd(0x002C);
	cmd(0x002A); 
	w_data(0x0000);
	w_data(0x0000);

	w_data(0x0001);
	w_data(0x000dF);

	cmd(0x002B);  
	w_data(0x0000);
	w_data(0x0000);
	w_data(0x0001);
	w_data(0x003f); 

	cmd(0x002c); 


  
}

static int tscscreem_open(struct input_dev *input_dev)
{
	return 0;
}

static void tscscreem_close(struct input_dev *input_dev)
{
	
}

#define GPIO_ALT_OFFSET(g) ((((g)/10))*4)
#define GPIO_ALT_VAL(a, g) ((a)<<(((g)%10)*3))
static inline void tscscreem_set_input(int gpio) 
{
	unsigned int v;
	v=readl(__io_address(GPIO_BASE+GPIO_ALT_OFFSET(gpio)));
	v&=~GPIO_ALT_VAL(0x7, gpio);	
	writel(v, __io_address(GPIO_BASE+GPIO_ALT_OFFSET(gpio)));
}

unsigned  char ts_num;

static void touch_read(void)
{
	long  temp_max,temp_min,j,temp,temp_value,temp_x,temp_y,temp_z1,temp_z2;
	tsc_start:
	while(lcd_bool);
		tsc_bool = true;
	msleep(3);
	if(lcd_bool)
	{
		tsc_bool = false;
		goto tsc_start;
	}
	writel(32,lcd_spi_base+SPI_CLK);
	
	tsc.flag =0;

	_tsc[0] = TS_X;
	tsc_transform(_tsc);
	
	readl(lcd_spi_base+SPI_FIFO);
	readl(lcd_spi_base+SPI_FIFO);
	temp_max  = readl(lcd_spi_base+SPI_FIFO)<<4;
	temp_max |= readl(lcd_spi_base+SPI_FIFO)>>4;
	temp_min  = temp_max;
	temp_value= temp_max;
	for(j = 0;j < 6;j++)
	{
		tsc_transform(_tsc);
		
		if((readl(__io_address(GPIO_BASE+0x34))&(1<<TSC_IRQ)))   
		{
			
			writel(8,lcd_spi_base+SPI_CLK);
			msleep(1);
			tsc_bool = false;
			tsc.flag = 1;
			return;
		}
		readl(lcd_spi_base+SPI_FIFO);
		readl(lcd_spi_base+SPI_FIFO);
		temp  = readl(lcd_spi_base+SPI_FIFO)<<4;
		temp |= readl(lcd_spi_base+SPI_FIFO)>>4;
		if(temp_max<temp)	temp_max = temp;
		if(temp_min>temp)	temp_min = temp;
		temp_value += temp;
	}
	
	temp_y =2026-((temp_value-temp_max-temp_min)/5) ;

	_tsc[0] = TS_Y;
	tsc_transform(_tsc);
	
	readl(lcd_spi_base+SPI_FIFO);
	readl(lcd_spi_base+SPI_FIFO);
	temp_max  = readl(lcd_spi_base+SPI_FIFO)<<4;
	temp_max |= readl(lcd_spi_base+SPI_FIFO)>>4;
	temp_min  = temp_max;
	temp_value= temp_max;
	for(j = 0;j < 6;j++)
	{
		tsc_transform(_tsc);
		
		if((readl(__io_address(GPIO_BASE+0x34))&(1<<TSC_IRQ)))   
		{
			
			writel(8,lcd_spi_base+SPI_CLK);
			msleep(1);
			tsc_bool = false;
			tsc.flag = 1;
			return;
		}
		readl(lcd_spi_base+SPI_FIFO);
		readl(lcd_spi_base+SPI_FIFO);
		temp  = readl(lcd_spi_base+SPI_FIFO)<<4;
		temp |= readl(lcd_spi_base+SPI_FIFO)>>4;
		if(temp_max<temp)	temp_max = temp;
		if(temp_min>temp)	temp_min = temp;
		temp_value += temp;
	}
	
	temp_x =(temp_value-temp_max-temp_min)/5;


	_tsc[0] = TS_Z1;
	tsc_transform(_tsc);
	temp_value++;
	readl(lcd_spi_base+SPI_FIFO);
	readl(lcd_spi_base+SPI_FIFO);
	temp_max  = readl(lcd_spi_base+SPI_FIFO)<<4;
	temp_max |= readl(lcd_spi_base+SPI_FIFO)>>4;
	temp_min  = temp_max;
	temp_value= temp_max;
	for(j = 0;j < 5;j++)
	{
		tsc_transform(_tsc);
		
		if((readl(__io_address(GPIO_BASE+0x34))&(1<<TSC_IRQ)))   
		{
			
			writel(8,lcd_spi_base+SPI_CLK);
			msleep(1);
			tsc_bool = false;
			tsc.flag = 1;
			return;
		}
		readl(lcd_spi_base+SPI_FIFO);
		readl(lcd_spi_base+SPI_FIFO);
		temp  = readl(lcd_spi_base+SPI_FIFO)<<4;
		temp |= readl(lcd_spi_base+SPI_FIFO)>>4;
		if(temp_max<temp)	temp_max = temp;
		if(temp_min>temp)	temp_min = temp;
		temp_value += temp;
	}
	tsc.z1 = (temp_value-temp_max-temp_min)/4;
	_tsc[0] = TS_START;
	tsc_transform(_tsc);
	
	if(tsc.z1 < 10)
	{
		
		
		writel(8,lcd_spi_base+SPI_CLK);
		msleep(1);
 		tsc_bool = false;
		tsc.z1 = 0;
	
		return ;
	}
	tsc.x = temp_x;
	tsc.y = temp_y;

	writel(8,lcd_spi_base+SPI_CLK);
	tsc_bool = false;
}



static void tsc_do_tasklet(unsigned long param)
{
	if((readl(__io_address(GPIO_BASE+0x34))&(1<<TSC_IRQ))) return;
	tsc.z2 = 0;
	while(!(readl(__io_address(GPIO_BASE+0x34))&(1<<TSC_IRQ)))
	{	
		tsc.z1 = 0;
		touch_read();
		if(tsc.flag) 	continue;
		if(tsc.z1&&(tsc.z2 != tsc.z1))
		{
			
			tsc.z2 = tsc.z1;
			input_report_abs(input_dev, ABS_X, tsc.x);
			input_report_abs(input_dev, ABS_Y, tsc.y);
			input_report_key(input_dev, BTN_TOUCH, 1);  
			input_report_abs(input_dev, ABS_PRESSURE, 0);  
			input_sync(input_dev);
		}
		msleep(10);
		
	}
	
	{ 
		input_report_abs(input_dev, ABS_X, tsc.x);
		input_report_abs(input_dev, ABS_Y, tsc.y);
		input_report_key(input_dev, BTN_TOUCH, 0);  
		input_report_abs(input_dev, ABS_PRESSURE, 1);  
        	input_sync(input_dev);
	}
	return;
	//tsc_end:
	//input_report_key(input_dev, BTN_TOUCH, 0);
	//input_sync(input_dev);
	//return;
};




static int thread_process(void* param)
{
	while(1)
	{
		msleep(30);
		if(!(readl(__io_address(GPIO_BASE+0x34))&(1<<TSC_IRQ)))
		{
			
			 tsc_do_tasklet(0x00);
		}

	}
	return 0;
};

static struct task_struct* tsc_thread = NULL;

static tsc_init(void)
{
	int ret = 0;

	input_dev = input_allocate_device();
	if(!input_dev){
		ret = -ENOMEM;
		goto out;
	}
	_tsc[1] = 0;
	_tsc[2] = 0;
	input_dev->name = "KeDei";

	input_dev->open = tscscreem_open;
	input_dev->close = tscscreem_close;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 80,1984, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 111,1915, 0, 0);

	//input_set_abs_params(input_dev, ABS_X, 115,2440, 0, 0);
	//input_set_abs_params(input_dev, ABS_Y, 155,1900, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0,0,1, 0);

	ret = input_register_device(input_dev);
	if (ret)
	{ 
		goto out;
	}
	

	tsc_thread = kthread_create(thread_process, NULL, "tsc_thread");

	mdelay(400);
	wake_up_process(tsc_thread);	

	return 0;
	out:
		input_free_device(input_dev);
		return ret;
}

/**************************************/

static  void spi_init(void)
{
	spi_init_pinmode();

	tscscreem_set_input(TSC_IRQ);

	writel(0,lcd_spi_base+SPI_CS);

	writel(readl(lcd_spi_base+SPI_CS)|SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX,lcd_spi_base+SPI_CS);

	cs 	|= SPI_CS_CPOL;	//clok  high
	cs	|= SPI_CS_CPHA;	//data high  change
 	cs 	|= SPI_CS_LEN_LONG;
	tsc_cs	= cs;
	cs	|= SPI_CS_CS_01;
	
	writel(8,lcd_spi_base+SPI_CLK);

	writel(readl(lcd_spi_base+SPI_CS)|cs,lcd_spi_base+SPI_CS);
	
	iinntt();

	tsc_init();



}

static inline void ili9341_write_byte(unsigned char data, int rs)
{
}



static  void ili9341_set_area(int x, int y) {

	cmd(0x002b);    //Set Gamma 
	w_data(y>>8); 
	w_data(0x00ff&y);  
	w_data(0x0001); 
	w_data(0x003f);

	cmd(0x002a);    //Set Gamma 
	w_data(x>>8); 
	w_data(0x00ff&x); 
	w_data(0x0001); 
	w_data(0x00df);
	cmd(0x002c); 
}

static void ili9341_setptr(struct ili9341 *item, int x, int y) {

	cmd(0x002b);    //Set Gamma 
	w_data(0x0000); 
	w_data(0x0000); 
	w_data(0x0001); 
	w_data(0x003f);

	cmd(0x002a);    //Set Gamma 
	w_data(0x0000); 
	w_data(0x0000); 
	w_data(0x0001); 
	w_data(0x00df);
	cmd(0x002c); 
}

static void ili9341_copy(struct ili9341 *item, unsigned int index)
{
	unsigned short x;
	unsigned short y;
	unsigned short *buffer;
	unsigned short *oldbuffer;
	unsigned int len;
	unsigned int count;
	int sendNewPos=1;
	x = item->pages[index].x;
	y = item->pages[index].y;
	buffer = item->pages[index].buffer;
	oldbuffer = item->pages[index].oldbuffer;
	len = item->pages[index].len;


	for (count = 0; count < len; count++) {
	
		if (buffer[count]==oldbuffer[count]) {
			sendNewPos=1;
		} else {
			if (sendNewPos) {
				ili9341_set_area(x,y);
				sendNewPos=0;
			}
			w_rgb(buffer[count]);
			oldbuffer[count]=buffer[count];
		}
		x++;
		if (x >= item->info->var.xres) {
			y++;
			x=0;
			ili9341_set_area(x,y);
		}
		
	}

}

static void ili9341_copy_t(struct ili9341 *item, unsigned int index)
{
	unsigned short x;
	unsigned short y;
	unsigned short *buffer;
	unsigned short *oldbuffer;
	unsigned int len;
	unsigned int count;
	int sendNewPos=1;
	x = item->pages[index].x;
	y = item->pages[index].y;
	buffer = item->pages[index].buffer;
	oldbuffer = item->pages[index].oldbuffer;
	len = item->pages[index].len;

	
	for (count = 0; count < len; count++) 
	{
		w_rgb(buffer[count]);
		//w_data(buffer[count]);
		oldbuffer[count]=buffer[count];
	}

}


static void ili9341_update_all(struct ili9341 *item)
{
	unsigned short i;
	struct fb_deferred_io *fbdefio = item->info->fbdefio;
	for (i = 0; i < item->pages_count; i++) {
		item->pages[i].must_update=1;
	}
	schedule_delayed_work(&item->info->deferred_work, fbdefio->delay);
}

static void ili9341_update(struct fb_info *info, struct list_head *pagelist)
{
	struct ili9341 *item = (struct ili9341 *)info->par;
	struct page *page;
	int i;

	list_for_each_entry(page, pagelist, lru) {
		item->pages[page->index].must_update=1;
	}

	
	if(value_j > 5)
	{
		
		for (i=0; i<item->pages_count; i++) {
			if (item->pages[i].must_update) {
				item->pages[i].must_update=0;
				ili9341_copy(item, i);
			}
		}
	}
	else
	{
		
		ili9341_setptr(item,0,0);
		for (i=0; i<item->pages_count; i++) {
			//ToDo: Small race here between checking and setting must_update, 
			//maybe lock?
			item->pages[i].must_update=0;
			ili9341_copy_t(item, i);
			
		}
		value_j++;
	}


}


static void __init ili9341_setup(struct ili9341 *item)
{
	
	int x;
	unsigned short *buffer;
	unsigned short *oldbuffer;
	unsigned int len;

	
	

	int i,j,count;
	ili9341_setptr(item, 0, 0);
	for (x=0; x<320*480; x++) 
			w_rgb(item->pages[0].oldbuffer[x]);
		
}

//This routine will allocate the buffer for the complete framebuffer. This
//is one continuous chunk of 16-bit pixel values; userspace programs
//will write here.
static int __init ili9341_video_alloc(struct ili9341 *item)
{
	unsigned int frame_size;

	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	frame_size = item->info->fix.line_length * item->info->var.yres;
	dev_dbg(item->dev, "%s: item=0x%p frame_size=%u\n",
		__func__, (void *)item, frame_size);

	item->pages_count = frame_size / PAGE_SIZE;
	if ((item->pages_count * PAGE_SIZE) < frame_size) {
		item->pages_count++;
	}
	dev_dbg(item->dev, "%s: item=0x%p pages_count=%u\n",
		__func__, (void *)item, item->pages_count);

	item->info->fix.smem_len = item->pages_count * PAGE_SIZE;
	item->info->fix.smem_start =
	    (unsigned long)vmalloc(item->info->fix.smem_len);
	if (!item->info->fix.smem_start) {
		dev_err(item->dev, "%s: unable to vmalloc\n", __func__);
		return -ENOMEM;
	}
	memset((void *)item->info->fix.smem_start, 0, item->info->fix.smem_len);

	return 0;
}

static void ili9341_video_free(struct ili9341 *item)
{
	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	kfree((void *)item->info->fix.smem_start);
}

static int __init ili9341_pages_alloc(struct ili9341 *item)
{
	unsigned short pixels_per_page;
	unsigned short yoffset_per_page;
	unsigned short xoffset_per_page;
	unsigned int index;
	unsigned short x = 0;
	unsigned short y = 0;
	unsigned short *buffer;
	unsigned short *oldbuffer;
	unsigned int len;

	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	item->pages = kmalloc(item->pages_count * sizeof(struct ili9341_page),
			      GFP_KERNEL);
	if (!item->pages) {
		dev_err(item->dev, "%s: unable to kmalloc for ili9341_page\n",
			__func__);
		return -ENOMEM;
	}

	pixels_per_page = PAGE_SIZE / (item->info->var.bits_per_pixel / 8);
	yoffset_per_page = pixels_per_page / item->info->var.xres;
	xoffset_per_page = pixels_per_page -
	    (yoffset_per_page * item->info->var.xres);
	dev_dbg(item->dev, "%s: item=0x%p pixels_per_page=%hu "
		"yoffset_per_page=%hu xoffset_per_page=%hu\n",
		__func__, (void *)item, pixels_per_page,
		yoffset_per_page, xoffset_per_page);

	oldbuffer = kmalloc(item->pages_count * pixels_per_page * 2,
			      GFP_KERNEL);
	if (!oldbuffer) {
		dev_err(item->dev, "%s: unable to kmalloc for ili9341_page oldbuffer\n",
			__func__);
		return -ENOMEM;
	}

	buffer = (unsigned short *)item->info->fix.smem_start;
	for (index = 0; index < item->pages_count; index++) {
		len = (item->info->var.xres * item->info->var.yres) -
		    (index * pixels_per_page);
		if (len > pixels_per_page) {
			len = pixels_per_page;
		}
		dev_dbg(item->dev,
			"%s: page[%d]: x=%3hu y=%3hu buffer=0x%p len=%3hu\n",
			__func__, index, x, y, buffer, len);
		item->pages[index].x = x;
		item->pages[index].y = y;
		item->pages[index].buffer = buffer;
		item->pages[index].oldbuffer = oldbuffer;
		item->pages[index].len = len;

		x += xoffset_per_page;
		if (x >= item->info->var.xres) {
			y++;
			x -= item->info->var.xres;
		}
		y += yoffset_per_page;
		buffer += pixels_per_page;
		oldbuffer += pixels_per_page;
	}

	return 0;
}

static void ili9341_pages_free(struct ili9341 *item)
{
	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	kfree(item->pages);
}

static inline __u32 CNVT_TOHW(__u32 val, __u32 width)
{
	return ((val<<width) + 0x7FFF - val)>>16;
}

//This routine is needed because the console driver won't work without it.
static int ili9341_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	int ret = 1;

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 */
	if (info->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green +
				      7471 * blue) >> 16;
	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		if (regno < 16) {
			u32 *pal = info->pseudo_palette;
			u32 value;

			red = CNVT_TOHW(red, info->var.red.length);
			green = CNVT_TOHW(green, info->var.green.length);
			blue = CNVT_TOHW(blue, info->var.blue.length);
			transp = CNVT_TOHW(transp, info->var.transp.length);

			value = (red << info->var.red.offset) |
				(green << info->var.green.offset) |
				(blue << info->var.blue.offset) |
				(transp << info->var.transp.offset);

			pal[regno] = value;
			ret = 0;
		}
		break;
	case FB_VISUAL_STATIC_PSEUDOCOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		break;
	}
	return ret;
}

static int ili9341_blank(int blank_mode, struct fb_info *info)
{
	struct ili9341 *item = (struct ili9341 *)info->par;
	if (blank_mode == FB_BLANK_UNBLANK)
		item->backlight=1;
	else
		item->backlight=0;
	//Item->backlight won't take effect until the LCD is written to. Force that
	//by dirty'ing a page.
	item->pages[0].must_update=1;
	schedule_delayed_work(&info->deferred_work, 0);
	return 0;
}

static void ili9341_touch(struct fb_info *info, int x, int y, int w, int h) 
{
	struct fb_deferred_io *fbdefio = info->fbdefio;
	struct ili9341 *item = (struct ili9341 *)info->par;
	int i, ystart, yend;
	if (fbdefio) {
		//Touch the pages the y-range hits, so the deferred io will update them.
		for (i=0; i<item->pages_count; i++) {
			ystart=item->pages[i].y;
			yend=item->pages[i].y+(item->pages[i].len/info->fix.line_length)+1;
			if (!((y+h)<ystart || y>yend)) {
				item->pages[i].must_update=1;
			}
		}
		//Schedule the deferred IO to kick in after a delay.
		schedule_delayed_work(&info->deferred_work, fbdefio->delay);
	}
}

static void ili9341_fillrect(struct fb_info *p, const struct fb_fillrect *rect) 
{
	sys_fillrect(p, rect);
	ili9341_touch(p, rect->dx, rect->dy, rect->width, rect->height);
}

static void ili9341_imageblit(struct fb_info *p, const struct fb_image *image) 
{
	sys_imageblit(p, image);
	ili9341_touch(p, image->dx, image->dy, image->width, image->height);
}

static void ili9341_copyarea(struct fb_info *p, const struct fb_copyarea *area) 
{
	sys_copyarea(p, area);
	ili9341_touch(p, area->dx, area->dy, area->width, area->height);
}

static ssize_t ili9341_write(struct fb_info *p, const char __user *buf, 
				size_t count, loff_t *ppos) 
{
	ssize_t res;
	res = fb_sys_write(p, buf, count, ppos);
	ili9341_touch(p, 0, 0, p->var.xres, p->var.yres);
	return res;
}

static struct fb_ops ili9341_fbops = {
	.owner        = THIS_MODULE,
	.fb_read      = fb_sys_read,
	.fb_write     = ili9341_write,
	.fb_fillrect  = ili9341_fillrect,
	.fb_copyarea  = ili9341_copyarea,
	.fb_imageblit = ili9341_imageblit,
	.fb_setcolreg	= ili9341_setcolreg,
	.fb_blank	= ili9341_blank,
};

static struct fb_fix_screeninfo ili9341_fix __initdata = {
	.id          = "ili9341",
	.type        = FB_TYPE_PACKED_PIXELS,
	.visual      = FB_VISUAL_TRUECOLOR,
	.accel       = FB_ACCEL_NONE,
	.line_length = 480 * 2,
};

static struct fb_var_screeninfo ili9341_var __initdata = {
	.xres		= 480,
	.yres		= 320,
	.xres_virtual	= 480,
	.yres_virtual	= 320,
	.width		= 480,
	.height		= 320,
	.bits_per_pixel	= 16,
	.red		= {11, 5, 0},
	.green		= {5, 6, 0},
	.blue		= {0, 5, 0},
	.activate	= FB_ACTIVATE_NOW,
	.vmode		= FB_VMODE_NONINTERLACED,
};

static struct fb_deferred_io ili9341_defio = {
        .delay          = HZ /25,//25
        .deferred_io    = &ili9341_update,
};

static int __init ili9341_probe(struct platform_device *dev)
{
	int ret = 0;
	struct ili9341 *item;
	struct fb_info *info;

	lcd_spi_base = ioremap(SPI0_BASE, SZ_256 - 1);   //spi map
	spi_init();



	dev_dbg(&dev->dev, "%s\n", __func__);

	item = kzalloc(sizeof(struct ili9341), GFP_KERNEL);
	if (!item) {
		dev_err(&dev->dev,
			"%s: unable to kzalloc for ili9341\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	item->dev = &dev->dev;
	dev_set_drvdata(&dev->dev, item);
	item->backlight=1;


	info = framebuffer_alloc(sizeof(struct ili9341), &dev->dev);
	if (!info) {
		ret = -ENOMEM;
		dev_err(&dev->dev,
			"%s: unable to framebuffer_alloc\n", __func__);
		goto out_item;
	}
	info->pseudo_palette = &item->pseudo_palette;
	item->info = info;
	info->par = item;
	info->dev = &dev->dev;
	info->fbops = &ili9341_fbops;
	info->flags = FBINFO_FLAG_DEFAULT|FBINFO_VIRTFB;
	info->fix = ili9341_fix;
	info->var = ili9341_var;

	ret = ili9341_video_alloc(item);
	if (ret) {
		dev_err(&dev->dev,
			"%s: unable to ili9341_video_alloc\n", __func__);
		goto out_info;
	}
	info->screen_base = (char __iomem *)item->info->fix.smem_start;

	ret = ili9341_pages_alloc(item);
	if (ret < 0) {
		dev_err(&dev->dev,
			"%s: unable to ili9341_pages_init\n", __func__);
		goto out_video;
	}

	info->fbdefio = &ili9341_defio;
	fb_deferred_io_init(info);

	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&dev->dev,
			"%s: unable to register_frambuffer\n", __func__);
		goto out_pages;
	}
	
	ili9341_update_all(item);
	//msleep(1200);
	//ili9341_setup(item);

	return ret;

out_pages:
	ili9341_pages_free(item);
out_video:
	ili9341_video_free(item);
out_info:
	framebuffer_release(info);
out_item:
	kfree(item);
out:
	return ret;
}


static void __exit ili9341_remove(struct platform_device *dev)
{
	struct fb_info *info = dev_get_drvdata(&dev->dev);
	struct ili9341 *item = (struct ili9341 *)info->par;
	
	LCD_RESET();
	unregister_framebuffer(info);
	ili9341_pages_free(item);
	ili9341_video_free(item);
	framebuffer_release(info);
	kfree(item);
	//return 0;
}

#ifdef CONFIG_PM
static int ili9341_suspend(struct platform_device *dev, pm_message_t state)
{
//	struct fb_info *info = dev_get_drvdata(&spi->dev);
//	struct ili9341 *item = (struct ili9341 *)info->par;
	/* enter into sleep mode */
//	ili9341_reg_set(item, ili9341_REG_SLEEP_MODE, 0x0001);
	return 0;
}

static int ili9341_resume(struct platform_device *dev)
{
//	struct fb_info *info = dev_get_drvdata(&spi->dev);
//	struct ili9341 *item = (struct ili9341 *)info->par;
	/* leave sleep mode */
//	ili9341_reg_set(item, ili9341_REG_SLEEP_MODE, 0x0000);
	return 0;
}
#else
#define ili9341_suspend NULL
#define ili9341_resume NULL
#endif

static struct platform_driver ili9341_driver = {
	.probe = ili9341_probe,
	.driver = {
		   .name = "ili9341",
		   },
};

static int __init ili9341_init(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);

	ret = platform_driver_register(&ili9341_driver);
	if (ret) {
		pr_err("%s: unable to platform_driver_register\n", __func__);
	}

	return ret;
}

module_init(ili9341_init);
//module_remove(ili9341_remove);
module_exit(ili9341_remove);


MODULE_DESCRIPTION("ili9341 LCD Driver");
MODULE_AUTHOR("Jeroen Domburg <jeroen@spritesmods.com>");
MODULE_LICENSE("GPL");
