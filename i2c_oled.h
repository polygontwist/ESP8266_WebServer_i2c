/*
 
*/

#ifndef i2c_OLED_H
#define i2c_OLED_H

// #include "Arduino.h"
// #include "Wire.h"
//#include <OneWire.h>
//#include <i2c.h>
//#include "gpio.h"
// #include "ets_sys.h"
//#include "osapi.h"

#define DISPLAYOFF           0xAE
#define SETCONTRAST          0x81
#define DISPLAYALLON_RESUME  0xA4
#define DISPLAYALLON         0xA5
#define NORMALDISPLAY        0xA6
#define INVERTDISPLAY        0xA7
#define DISPLAYON            0xAF
#define SETDISPLAYOFFSET     0xD3
#define SETCOMPINS           0xDA
#define SETVCOMDETECT        0xDB
#define SETDISPLAYCLOCKDIV   0xD5
#define SETPRECHARGE         0xD9
#define SETMULTIPLEX         0xA8
#define SETLOWCOLUMN         0x00
#define SETHIGHCOLUMN        0x10
#define SETSTARTLINE         0x40
#define MEMORYMODE           0x20
#define COLUMNADDR           0x21
#define PAGEADDR             0x22
#define COMSCANINC           0xC0
#define COMSCANDEC           0xC8
#define SEGREMAP             0xA0
#define CHARGEPUMP           0x8D
#define EXTERNALVCC          0x10
#define SWITCHCAPVCC         0x20
#define SETPAGEADDR          0xB0
#define SETCOLADDR_LOW       0x00
#define SETCOLADDR_HIGH      0x10
#define ACTIVATE_SCROLL                       0x2F
#define DEACTIVATE_SCROLL                     0x2E
#define SET_VERTICAL_SCROLL_AREA              0xA3
#define RIGHT_HORIZONTAL_SCROLL               0x26
#define LEFT_HORIZONTAL_SCROLL                0x27
#define VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL  0x29
#define VERTICAL_AND_LEFT_HORIZONTAL_SCROLL   0x2A

// I2C devices are accessed through a Device ID. This is a 7-bit
// value but is sometimes expressed left-shifted by 1 as an 8-bit value.
// A pin on SSD1306 allows it to respond to ID 0x3C or 0x3D. The board
// I bought from ebay used a 0-ohm resistor to select between "0x78"
// (0x3c << 1) or "0x7a" (0x3d << 1). The default was set to "0x78"
#define OLED_DEVID   0x3c

#define drawmode_WHITE 0
#define drawmode_BLACK 1
#define drawmode_INVERSE 2

// I2C communication here is either <DEVID> <CTL_CMD> <command byte>
// or <DEVID> <CTL_DAT> <display buffer bytes> <> <> <> <>...
// These two values encode the Co (Continuation) bit as b7 and the
// D/C// (Data/Command Selection) bit as b6.
/*#define CTL_CMD  0x80
#define CTL_DAT  0x40
*/
 
#define swap(a, b) { int16_t t = a; a = b; b = t; }
 
class i2c_oled 
{
	public:
		i2c_oled();
		void init(int i2c_devid);
		
		void getBuff(void);
		void drawPixel(int x, int y, int mode);
		void drawLine(int xa, int ya,int xe, int ye, int mode);
		void drawRect(int x, int y,int b, int h, int mode);

		void invert_display(bool inv);
		
		void reset_display(void);
		void displayOn(void);
		void displayOff(void);
		void clear_display(void);
		void SendChar(unsigned char data);
		void sendCharXY(unsigned char data, int zeile, int spalte);
		void sendcommand(unsigned char com);
		void setXY(unsigned char zeile,unsigned char spalte);
		void sendStr(unsigned char *string);
		void sendStrXY( char *string, int zeile, int spalte);
		

	private:
		int _devid;//=0x3c;		
		void init_OLED(void);
		void drawBuff(char data);
		int width=128;
		int height=64;
		
	
};




#endif
