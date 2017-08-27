// Author: Conjur , 2016-08-22
// https://www.raspberrypi.org/forums/viewtopic.php?p=1027153#p1027153
// for ESP8266
#include <SPI.h>
#include <FS.h>
#define SPI_MOSI 13   //blue
#define SPI_SCLK 14   //grey
#define SPI_SS_L 4    //brown

uint8_t lcd_rotations[4] = {
  0b11101010, //  0
  0b01001010, // 90
  0b00101010, //180
  0b10001010  //270
};
volatile uint8_t lcd_rotation;
volatile uint16_t lcd_h;
volatile uint16_t lcd_w;

uint16_t colors[17] = {
  0b0000000000000000,        /* BLACK   000000 */
  0b0000000000010000,        /* NAVY    000080 */
  0b0000000000011111,        /* BLUE    0000ff */
  0b0000010011000000,        /* GREEN   009900 */
  0b0000010011010011,        /* TEAL    009999 */
  0b0000011111100000,        /* LIME    00ff00 */
  0b0000011111111111,        /* AQUA    00ffff */
  0b1000000000000000,        /* MAROON  800000 */
  0b1000000000010000,        /* PURPLE  800080 */
  0b1001110011000000,        /* OLIVE   999900 */
  0b1000010000010000,        /* GRAY    808080 */
  0b1100111001111001,        /* SILVER  cccccc */
  0b1111100000000000,        /* RED     ff0000 */
  0b1111100000011111,        /* FUCHSIA ff00ff */
  0b1111111111100000,        /* YELLOW  ffff00 */
  0b1111111111111111,        /* WHITE   ffffff */
  0b0000000000000000         /* BLACK   000000 */ //added to fix my poor coding (color+1)
};

uint8_t color;

uint8_t buf[54];
uint16_t p, c;
uint32_t isize, ioffset, iwidth, iheight, ibpp, fpos, rowbytes;


void lcd_rst(void) {
  GPOC = 1<<SPI_SS_L; SPI.write(0); GPOS = 1<<SPI_SS_L; delay(25);
  GPOC = 1<<SPI_SS_L; SPI.write(1); GPOS = 1<<SPI_SS_L; delay(25);
}
void lcd_cmd(uint8_t cmd) {
  GPOC = 1<<SPI_SS_L; SPI.write(cmd>>1); SPI.write(0x11+((cmd&1)<<5)); GPOS = 1<<SPI_SS_L;
  GPOC = 1<<SPI_SS_L; SPI.write(cmd>>1); SPI.write(0x1B+((cmd&1)<<5)); GPOS = 1<<SPI_SS_L;
}
void lcd_dat(uint8_t dat) {
  GPOC = 1<<SPI_SS_L; SPI.write(dat>>1); SPI.write(0x15+((dat&1)<<5)); GPOS = 1<<SPI_SS_L;
  GPOC = 1<<SPI_SS_L; SPI.write(dat>>1); SPI.write(0x1F+((dat&1)<<5)); GPOS = 1<<SPI_SS_L;
}
void lcd_color(uint16_t col) {
  GPOC = 1<<SPI_SS_L; SPI.write16(col); SPI.write(0x75); GPOS = 1<<SPI_SS_L;
  GPOC = 1<<SPI_SS_L; SPI.write16(col); SPI.write(0x7F); GPOS = 1<<SPI_SS_L;
  yield();
}
void lcd_init(void) {
  //reset display
  lcd_rst();
  lcd_cmd(0x00);
  lcd_cmd(0x11);delay(1); //Sleep Out
  lcd_cmd(0xEE); lcd_dat(0x02); lcd_dat(0x01); lcd_dat(0x02); lcd_dat(0x01);
  lcd_cmd(0xED); lcd_dat(0x00); lcd_dat(0x00); lcd_dat(0x9A); lcd_dat(0x9A); lcd_dat(0x9B); lcd_dat(0x9B); lcd_dat(0x00); lcd_dat(0x00); lcd_dat(0x00); lcd_dat(0x00); lcd_dat(0xAE); lcd_dat(0xAE); lcd_dat(0x01); lcd_dat(0xA2); lcd_dat(0x00);
  lcd_cmd(0xB4); lcd_dat(0x00);
  lcd_cmd(0xC0); lcd_dat(0x10); lcd_dat(0x3B); lcd_dat(0x00); lcd_dat(0x02); lcd_dat(0x11);
  lcd_cmd(0xC1); lcd_dat(0x10);
  lcd_cmd(0xC8); lcd_dat(0x00); lcd_dat(0x46); lcd_dat(0x12); lcd_dat(0x20); lcd_dat(0x0C); lcd_dat(0x00); lcd_dat(0x56); lcd_dat(0x12); lcd_dat(0x67); lcd_dat(0x02); lcd_dat(0x00); lcd_dat(0x0C);
  lcd_cmd(0xD0); lcd_dat(0x44); lcd_dat(0x42); lcd_dat(0x06);
  lcd_cmd(0xD1); lcd_dat(0x43); lcd_dat(0x16);
  lcd_cmd(0xD2); lcd_dat(0x04); lcd_dat(0x22);
  lcd_cmd(0xD3); lcd_dat(0x04); lcd_dat(0x12);
  lcd_cmd(0xD4); lcd_dat(0x07); lcd_dat(0x12);
  lcd_cmd(0xE9); lcd_dat(0x00);
  lcd_cmd(0xC5); lcd_dat(0x08);
  
  lcd_setrotation(0);
  lcd_cmd(0x29);delay(2); // Display On
  lcd_cmd(0x00);   // NOP
  lcd_cmd(0x11);delay(1); // Sleep Out
}

//lcd_fillframe
//fills an area of the screen with a single color.
void lcd_fillframe(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t col) {
  int span=h*w;
  lcd_setframe(x,y,w,h);
  for(int q=0;q<span;q++) { lcd_color(col); }
}

void lcd_setframe(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  lcd_cmd(0x2A);
  lcd_dat(x>>8); lcd_dat(x&0xFF);
  lcd_dat(((w+x)-1)>>8); lcd_dat(((w+x)-1)&0xFF);
  lcd_cmd(0x2B);
  lcd_dat(y>>8); lcd_dat(y&0xFF);
  lcd_dat(((h+y)-1)>>8); lcd_dat(((h+y)-1)&0xFF);
  lcd_cmd(0x2C);
}
void lcd_img(char *fname, uint16_t x, uint16_t y) {
  if (SPIFFS.exists(fname)) {
    File f = SPIFFS.open(fname, "r");
    f.seek(0,SeekSet);
    f.read(buf,30);
    isize =   buf[2] + (buf[3]<<8) + (buf[4]<<16) + (buf[5]<<24);
    ioffset = buf[10] + (buf[11]<<8) + (buf[12]<<16) + (buf[13]<<24);
    iwidth =  buf[18] + (buf[19]<<8) + (buf[20]<<16) + (buf[21]<<24);
    iheight = buf[22] + (buf[23]<<8) + (buf[24]<<16) + (buf[25]<<24);
    ibpp =    buf[28] + (buf[29]<<8);

    Serial.printf("\n\nFile Size: %u\nOffset: %u\nWidth: %u\nHeight: %u\nBPP: %u\n\n",isize,ioffset,iwidth,iheight,ibpp);
    lcd_setframe(x,y,iwidth,iheight); //set the active frame...
    rowbytes=(iwidth*3) + 4-((iwidth*3)%4);
    for(p=iheight-1;p>0;p--) {
      fpos=ioffset+(p*rowbytes);
      f.seek(fpos,SeekSet);
      // p = relative page address (y)
      for(c=0;c<iwidth;c++) {
        // c = relative column address (x)
        f.read(buf,3);
        lcd_color(((buf[2]&248)<<8) + ((buf[1]&252)<<3) + (buf[0]>>3));
        
      }
    }
  
  }



  
}




void lcd_setrotation(uint8_t m) {
  lcd_cmd(0x36); lcd_dat(lcd_rotations[m]);
  if (m&1) {
    lcd_h = 480;
    lcd_w = 320;
  } else {
    lcd_h = 320;
    lcd_w = 480;
  }
}
void setup() {
  SPIFFS.begin();
  pinMode(SPI_SS_L,OUTPUT);
  pinMode(SPI_SCLK,SPECIAL);
  pinMode(SPI_MOSI,SPECIAL);
  GPOS = 1<<SPI_SS_L;
  color=0;
  lcd_rotation=0;
  SPI.begin();
  SPI.setClockDivider(0x00081001);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  Serial.begin(250000);
  lcd_init();
  lcd_fillframe(0,0,480,320,0); //black out the screen.
  lcd_img("/boot.bmp", 50, 5);
}

void loop() {
  /*
  //Update rotation
  lcd_setrotation(lcd_rotation);

  //Fill entire screen with new color
  lcd_fillframe(0,0,lcd_w,lcd_h,colors[color]);

  //Make a color+1 box, 5 pixels from the top-left corner, 20 pixels high, 95 (100-5) pixels from right border.
  lcd_fillframe(5,5,lcd_w-100,20,colors[color+1]);

  //increment color
  color++;
  //if color is overflowed, reset to 0
  if (color==16) {color=0;}

  //increment rotation
  lcd_rotation++;
  //if rotation is overflowed, reset to 0
  if (lcd_rotation==4) lcd_rotation=0;
  */
  delay(500);
}