#ifndef MDS_SDD1306_DISPLAY
#define MDS_SDD1306_DISPLAY
 
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <arduino-timer.h>

#define MDS_DISPLAY_VERSION		"1.0.0"

#define	MDS_DISPLAY_LINES		4		// Number of text lines that fit on the display
#define MDS_SCREEN_WIDTH		128		// OLED display width, in pixels
#define MDS_SCREEN_HEIGHT		32		// OLED display height, in pixels
#define	MDS_DISPLAY_TIMEOUT		5000	// Time(ms) between each display scroll
#define MDS_OLED_RESET 			-1		// Reset pin # (or -1 if sharing Arduino reset pin)

struct Screen {
	String		lines[MDS_DISPLAY_LINES];
};

class MDS_Display {
	public:
        MDS_Display();
        ~MDS_Display();
        void					setup();
        bool					clearMessage();
        void					clear();
		void 					_clear();
		void					at(String m, uint8_t line);
        void					D(String m);
		Adafruit_SSD1306*		getDevice();
		void					tick();
		void 					setAutoUpdate(bool au);
		bool					getAutoUpdate();

	private:
		Adafruit_SSD1306		display;
		bool					isSetup = false;
		bool					doTicks = true;
		Screen					screen;
		unsigned long			nextClearTime;
};

#endif