#include "MDS_Display.h"
#include <Adafruit_SSD1306.h>

MDS_Display::MDS_Display() {
//		setup();
}

MDS_Display::~MDS_Display() {
}

void MDS_Display::setup() {
	if(isSetup)
		return;

	if(&Serial != NULL) {
		Serial.print(F("MDS_Display V"));
		Serial.println(F(MDS_DISPLAY_VERSION));
	}
	
	display = Adafruit_SSD1306(MDS_SCREEN_WIDTH, MDS_SCREEN_HEIGHT, &Wire, MDS_OLED_RESET);

	if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
		if(&Serial != NULL)
			Serial.println(F("Display: Fatal: SSD1306 allocation failed."));

		for(;;); // Don't proceed, loop forever
	}

	clear();

	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);
	display.setTextWrap(false);
	display.setCursor(0,0);

	display.display();

	for(int i = 0; i < MDS_DISPLAY_LINES; i++)
		screen.lines[i] = String();

	nextClearTime = millis() + MDS_DISPLAY_TIMEOUT;
	
	isSetup = true;
}

bool MDS_Display::clearMessage() {
	D("");
			
	return true;
}

void MDS_Display::setAutoUpdate(bool au) {
	doTicks = au;
}

bool MDS_Display::getAutoUpdate() {
	return doTicks;
}

void MDS_Display::clear() {
	display.clearDisplay();
 	display.setCursor(0,0);
 	display.display();
	
	for(int i = 0; i < MDS_DISPLAY_LINES; i++)
		screen.lines[i] = String();
}

void MDS_Display::_clear() {
	display.clearDisplay();
}

void MDS_Display::at(String m, uint8_t line) {
 	display.setCursor(0, (int16_t)line);
	display.print(m);
 	display.display();
}

void MDS_Display::D(String m) {
	for(int i = 0; i < MDS_DISPLAY_LINES-1; i++) {
		screen.lines[i] = screen.lines[i + 1];
	}

// 	if(&Serial != NULL)
// 		Serial.println("Display: [" + m + "]");

	screen.lines[MDS_DISPLAY_LINES-1] = m;

	display.clearDisplay();
	display.setCursor(0,0);
	
	for(int i = 0; i < MDS_DISPLAY_LINES; i++) {
		display.println(screen.lines[i]);
	}

	if(m.length() != 0) {
		nextClearTime = millis() + MDS_DISPLAY_TIMEOUT;
	}

	display.display();
}

Adafruit_SSD1306* MDS_Display::getDevice() {
	return &display;
}

void MDS_Display::tick() {
	if(!doTicks) return;
	
	if(millis() >= nextClearTime) {
		clearMessage();
	
		nextClearTime = millis() + MDS_DISPLAY_TIMEOUT;
	}
}