// ---- Configuration -------------------------------------------------------------------
//
#define USE_LEDSTRIP

//
// --------------------------------------------------------------------------------------

#include "config.h"
#include "MDS_Display.h"
#include "MDS_AppConfig.h"
#include "MDS_AutoWiFi.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#ifdef USE_LEDSTRIP
#include <NeoPixelBusLg.h>
#include <NeoPixelAnimator.h>
#endif
#include <Arduino.h>
#include <FS.h>
#include <NTPClient.h>

#include <ESP8266HTTPClient.h>

// --------------------------------------------------------------------------------------

#define VERSION				"2.11"
#define PROJECT_VERSION		"Clock " VERSION


// curl -s 'http://192.168.0.29?dim=10&bright=50&dimNight=8&brightNight=20'

// curl -s 'http://192.168.0.29?dim=10&bright=50&dimNight=0&brightNight=0&temp=10&hue=0&nightHour=22&dayHour=9'
// curl -s 'http://192.168.0.29?latitude=42.49988&longitude=-71.27531&coldHue=45&warmHue=11'
// curl -s 'http://192.168.0.29?owAppKey=TOKEN'

char				systemID[32];		// System ID. Based on the WiFi MAC

bool				isConfigured = false;

MDS_Display			display;
MDS_AppConfig		appConfig;
MDS_AutoWiFi		autoWiFi;

#ifdef USE_LEDSTRIP
// Pin is _fixed to be GPIO3 (RX pin)
// NeoPixelBusLg<NeoGrbFeature, NeoWs2812xMethod, NeoGammaCieLabEquationMethod> strip(NUM_LEDSTRIP);
NeoPixelBusLg<NeoGrbFeature, NeoWs2812xMethod, NeoGammaNullMethod> strip(NUM_LEDSTRIP);
//NeoGamma<NeoGammaTableMethod> colorGamma;
NeoGamma<NeoGammaEquationMethod> colorGamma;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, (24-4) * 3600);	// TODO: Set Timezone from OpenWeather

#define GAMMA 1.8f
// #define RGB(r, g, b) (RgbColor((pow(r / 255.0f, GAMMA) * 255) + 0.5f, (pow(g / 255.0f, GAMMA) * 255) + 0.5f, (pow(b / 255.0f, GAMMA) * 255) + 0.5f))
#define RGB(r, g, b) (RgbColor(gammaTable[r], gammaTable[g], gammaTable[b]))

#endif

uint8_t gammaTable[256];  // Lookup table for gamma correction

unsigned long epoch;
float dimPct;
float brightPct;
float tempPct;
int timezone;
int temperature;
int httpResponseCode;
unsigned long lastEpoch;
unsigned long nextNTPUpdate;
unsigned long nextTemperatureUpdate;
unsigned long nextRebootUpdate;
unsigned long nextDisplayUpdate;
uint32_t displaySleepSeconds;

#define DISP_ST_NIGHT				1
#define DISP_ST_WAKING				2
#define DISP_ST_DAY 				3
#define DISP_ST_NODDING				4
#define DISP_ST_SLEEPING			5
#define DISP_ST_SLEEP				6

uint8_t displayState = DISP_ST_DAY;
uint8_t targetDisplayState = DISP_ST_DAY;
uint8_t displayStep;
uint8_t openWeatherTries = 0;

// ---- AppConfig

struct AppConfig {
	bool		isConfigured;
	char		MQTTName[32];
	char		MQTTBroker[64];
	uint16_t	MQTTPort;
	uint8_t		dimLEDPercentage;
	uint8_t		brightLEDPercentage;
	uint8_t		dimNightLEDPercentage;
	uint8_t		brightNightLEDPercentage;
	uint8_t		temperatureLEDPercentage;
	uint8_t		LEDHuePercentage;
	uint8_t		nightHour;
	uint8_t		dayHour;
	char		owAppKey[33];
	char		latitude[11];
	char		longitude[11];
	uint8_t		LEDColdHuePercentage;
	uint8_t		LEDWarmHuePercentage;
};
AppConfig appConfigData;

void setConfigurationCB(const JsonDocument &doc) {
// Serial.println("setConfigurationCB");
	appConfigData.isConfigured = doc[jc_IS_CONFIGURED];

	strlcpy(appConfigData.MQTTName, doc[jc_MQTTNAME], sizeof(appConfigData.MQTTName));
	strlcpy(appConfigData.MQTTBroker, doc[jc_MQTTBROKER], sizeof(appConfigData.MQTTBroker));
	appConfigData.MQTTPort = doc[jc_MQTTPORT];
	appConfigData.dimLEDPercentage = doc[jc_DIMLEDPERCENTAGE];
	appConfigData.brightLEDPercentage = doc[jc_BRIGHTLEDPERCENTAGE];
	appConfigData.dimNightLEDPercentage = doc[jc_DIMNIGHTLEDPERCENTAGE];
	appConfigData.brightNightLEDPercentage = doc[jc_BRIGHTNIGHTLEDPERCENTAGE];
	appConfigData.temperatureLEDPercentage = doc[jc_TEMPERATUREPERCENTAGE];
	appConfigData.LEDHuePercentage = doc[jc_LEDHUEPERCENTAGE];
	appConfigData.nightHour = doc[jc_NIGHTHOUR];
	appConfigData.dayHour = doc[jc_DAYHOUR];
	strlcpy(appConfigData.owAppKey, doc[jc_OWKEY], sizeof(appConfigData.owAppKey));
	strlcpy(appConfigData.latitude, doc[jc_OWLAT], sizeof(appConfigData.latitude));
	strlcpy(appConfigData.longitude, doc[jc_OWLON], sizeof(appConfigData.longitude));
	appConfigData.LEDColdHuePercentage = doc[jc_LEDCOLDHUEPERCENTAGE];
	appConfigData.LEDWarmHuePercentage = doc[jc_LEDWARMHUEPERCENTAGE];
	
	if(!appConfigData.isConfigured) {
		appConfigData.dimLEDPercentage = 15;
		appConfigData.brightLEDPercentage = 40;
		appConfigData.dimNightLEDPercentage = 5;
		appConfigData.brightNightLEDPercentage = 10;
		appConfigData.temperatureLEDPercentage = 10;
		appConfigData.LEDHuePercentage = 0;
		appConfigData.nightHour = 21;
		appConfigData.dayHour = 06;
		
		strlcpy(appConfigData.owAppKey, "", sizeof(appConfigData.owAppKey));
		strlcpy(appConfigData.latitude, "", sizeof(appConfigData.latitude));
		strlcpy(appConfigData.longitude, "", sizeof(appConfigData.longitude));

		appConfigData.LEDColdHuePercentage = 45;
		appConfigData.LEDWarmHuePercentage = 11;
		
		appConfigData.isConfigured = true;
	}
}

void getConfigurationCB(JsonDocument &doc) {
	doc[jc_IS_CONFIGURED]				= appConfigData.isConfigured;

	doc[jc_MQTTNAME]					= appConfigData.MQTTName;
	doc[jc_MQTTBROKER]					= appConfigData.MQTTBroker;
	doc[jc_MQTTPORT] 					= appConfigData.MQTTPort;
	doc[jc_DIMLEDPERCENTAGE]			= appConfigData.dimLEDPercentage;
	doc[jc_BRIGHTLEDPERCENTAGE]			= appConfigData.brightLEDPercentage;
	doc[jc_DIMNIGHTLEDPERCENTAGE]		= appConfigData.dimNightLEDPercentage;
	doc[jc_BRIGHTNIGHTLEDPERCENTAGE]	= appConfigData.brightNightLEDPercentage;
	doc[jc_TEMPERATUREPERCENTAGE]		= appConfigData.temperatureLEDPercentage;
	doc[jc_LEDHUEPERCENTAGE]			= appConfigData.LEDHuePercentage;
	doc[jc_NIGHTHOUR]					= appConfigData.nightHour;
	doc[jc_DAYHOUR]						= appConfigData.dayHour;
	doc[jc_OWKEY]						= appConfigData.owAppKey;
	doc[jc_OWLAT]						= appConfigData.latitude;
	doc[jc_OWLON]						= appConfigData.longitude;
	doc[jc_LEDCOLDHUEPERCENTAGE]		= appConfigData.LEDColdHuePercentage;
	doc[jc_LEDWARMHUEPERCENTAGE]		= appConfigData.LEDWarmHuePercentage;
}

// ---- autoWiFi

void savePortalConfigurationCB(String name, String broker, String port) {
	Serial.println(F("savePortalConfigurationCB()"));

	strlcpy(appConfigData.MQTTName, name.c_str(), name.length() + 1);
	strlcpy(appConfigData.MQTTBroker, broker.c_str(), broker.length() + 1);
	appConfigData.MQTTPort = (uint16_t)port.toInt();

	appConfig.save();
	appConfig.print();
}

// ---- 

void generateGammaTable() {
    for (int i = 0; i <= 255; i++) {
        float normalized = i / 255.0;
        gammaTable[i] = round(pow(i / 255.0, GAMMA) * 255.0); 
//		Serial.println(String(i) + " " + String(gammaTable[i]));
    }
}

void rootPage() {
	char	data[CONFIG_DOC_SIZE];
	
	if(autoWiFi.getWebServer()->args() > 0) {
		if(autoWiFi.getWebServer()->hasArg(F(jc_BRIGHTLEDPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_BRIGHTLEDPERCENTAGE)).toInt();

			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got bright: " + String(val));

			appConfigData.brightLEDPercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_DIMLEDPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_DIMLEDPERCENTAGE)).toInt();
			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got dim: " + String(val));

			appConfigData.dimLEDPercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_BRIGHTNIGHTLEDPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_BRIGHTNIGHTLEDPERCENTAGE)).toInt();
			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got bright night: " + String(val));

			appConfigData.brightNightLEDPercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_DIMNIGHTLEDPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_DIMNIGHTLEDPERCENTAGE)).toInt();
			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got dim night: " + String(val));

			appConfigData.dimNightLEDPercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_TEMPERATUREPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_TEMPERATUREPERCENTAGE)).toInt();
			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got temp brightness: " + String(val));

			appConfigData.temperatureLEDPercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_LEDHUEPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_LEDHUEPERCENTAGE)).toInt();

			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got hue: " + String(val));

			appConfigData.LEDHuePercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_NIGHTHOUR))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_NIGHTHOUR)).toInt();
			if(val > 23) val = 23;
			Serial.println("Got night hour: " + String(val));

			appConfigData.nightHour = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_DAYHOUR))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_DAYHOUR)).toInt();
			if(val > 23) val = 23;
			Serial.println("Got day hour: " + String(val));

			appConfigData.dayHour = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_MQTTNAME))) {
			String name = autoWiFi.getWebServer()->arg(F(jc_MQTTNAME));
			Serial.println("Got device Name: " + name);

			strlcpy(appConfigData.MQTTName, name.c_str(), sizeof(appConfigData.MQTTName));

			autoWiFi.getWebServer()->send(200, "application/json", name);

			appConfig.save();
			appConfig.print();
		}

		if(autoWiFi.getWebServer()->hasArg(F(jc_OWKEY))) {
			String name = autoWiFi.getWebServer()->arg(F(jc_OWKEY));
			Serial.println("Got OpenWeather Key: " + name);

			strlcpy(appConfigData.owAppKey, name.c_str(), sizeof(appConfigData.owAppKey));

			nextTemperatureUpdate = 0;
			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_OWLAT))) {
			String name = autoWiFi.getWebServer()->arg(F(jc_OWLAT));
			Serial.println("Got latitude: " + name);

			strlcpy(appConfigData.latitude, name.c_str(), sizeof(appConfigData.latitude));

			nextTemperatureUpdate = 0;
			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_OWLON))) {
			String name = autoWiFi.getWebServer()->arg(F(jc_OWLON));
			Serial.println("Got longitude: " + name);

			strlcpy(appConfigData.longitude, name.c_str(), sizeof(appConfigData.longitude));

			nextTemperatureUpdate = 0;
			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_LEDCOLDHUEPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_LEDCOLDHUEPERCENTAGE)).toInt();

			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got cold hue: " + String(val));

			appConfigData.LEDColdHuePercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_LEDWARMHUEPERCENTAGE))) {
			uint8_t val = autoWiFi.getWebServer()->arg(F(jc_LEDWARMHUEPERCENTAGE)).toInt();

			if(val > 100) val = 100;
			if(val < 0) val = 0;
			Serial.println("Got warm hue: " + String(val));

			appConfigData.LEDWarmHuePercentage = val;

			appConfig.save();
			appConfig.print();
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_DSLEEP))) {
			uint32_t val = autoWiFi.getWebServer()->arg(F(jc_DSLEEP)).toInt();

			if(val > 86400) val = 86400;
			if(val < 0) val = 0;
			Serial.println("Got display sleep: " + String(val));

			displaySleepSeconds = val;
		}
		if(autoWiFi.getWebServer()->hasArg(F(jc_REBOOT))) {
			displaySleepSeconds = 60; // Must be longer than the reboot ...
			nextRebootUpdate = millis()/1000 + 10;
		}
// 		if(autoWiFi.getWebServer()->hasArg(F("word"))) {
// 			String word = autoWiFi.getWebServer()->arg(F("word"));
// 			Serial.println("Got word: " + word);
// 
// 			strangeThing(word);
// 
// 			autoWiFi.getWebServer()->send(200, "application/json", word);
// 		}
		StaticJsonDocument<CONFIG_DOC_SIZE> doc;

		getConfigurationCB(doc);
		serializeJsonPretty(doc, data, sizeof(data));

		autoWiFi.getWebServer()->send(200, "application/json", data);
	} else {
//		autoWiFi.getWebServer()->send(200, "application/text", "Submit parameter ?word=");

		StaticJsonDocument<CONFIG_DOC_SIZE> doc;

		getConfigurationCB(doc);

		JsonObject nested = doc.createNestedObject("status");
 
		unsigned long now = millis()/1000;
	 	if(timeClient.isTimeSet()) {
			nested[st_TIMEZONE] = int(timezone / 3600);
			if(nextNTPUpdate > 0) {
				char buffer[30];

				nested[st_NEXTNTPUPDATE] = int(nextNTPUpdate - now);

				int h = timeClient.getHours();
				int m = timeClient.getMinutes();
				int	s = timeClient.getSeconds();

				snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", h, m, s);

				nested[st_TIME] = String(buffer);
			} else {
				nested[st_NEXTNTPUPDATE] = 0;
			}

			if(nextTemperatureUpdate > 0) {
				nested[st_TEMPERATURE] = int(temperature);
				nested[st_NEXTOWUPDATE] = int(nextTemperatureUpdate - now);
				nested[st_LASTOWRESPONSE] = httpResponseCode;
			} else {
				nested[st_NEXTOWUPDATE] = 0;
			}

			nested[st_DISPLAY] = String(displayState == DISP_ST_SLEEP ? "Sleep" : (displayState == DISP_ST_DAY ? "Day":"Night"));
		} else {
			nested[st_DISPLAY] = String("Preparing");
		}

		nested[st_DSLEEPREMAINING] = displaySleepSeconds;

		nested[st_VERSION] = String(PROJECT_VERSION);
		
		serializeJsonPretty(doc, data, sizeof(data));

		autoWiFi.getWebServer()->send(200, "application/json", data);
	}
}

void listDir(fs::FS &fs, const char *dirname){
	Serial.printf("  Directory %s:\n", dirname);

	String str = "";
	Dir dir = fs.openDir(dirname);
	while (dir.next()) {
		str += F("    ");
		str += dir.fileName();
		str += F(" (");
		str += dir.fileSize();
		str += F("b)\n");
	}
	Serial.print(str);
}

void test() {
	for(int h = 0; h < 36; h++) {
		for(int j = 0; j < 100; j++) {
			RgbColor color = colorGamma.Correct( RgbColor(HsbColor(1.0*((float)h/100), 1.0, 1.0*((float)j/100))) );
//			RgbColor color =<RgbColor(HsbColor(1.0*((float)h/100), 1.0, 1.0*((float)j/100)));
			for(int i = 0; i < 10; i++) {
				strip.SetPixelColor(i, color);
			}
			strip.Show();
			delay(20);
		}
		for(int j = 100; j > 0; j--) {
			RgbColor color = colorGamma.Correct( RgbColor(HsbColor(1.0*((float)h/100), 1.0, 1.0*((float)j/100))) );
//			RgbColor color = RgbColor(HsbColor(1.0*((float)h/100), 1.0, 1.0*((float)j/100)));
			for(int i = 0; i < 10; i++) {
				strip.SetPixelColor(i, color);
			}
			strip.Show();
			delay(20);
		}
	}
}



void setup() {
	delay(3000);

	// Set up Serial Port
	Serial.begin(115200);
	Serial.println();

	// Set up Display
	display.setup();
	display.setAutoUpdate(false);
	
	display._clear();
	display.at(F(PROJECT_VERSION), 0);

	// Set up AppConfig
	appConfig.setup();
	appConfig.setSetConfigCallback(setConfigurationCB);
	appConfig.setGetConfigCallback(getConfigurationCB);


	Serial.print(F("\n"));
	Serial.print(F(VERSION));
	Serial.println(F("\n\n"));

	Serial.println();

	// See if we are in WiFi configuration mode
	pinMode(PIN_RESET_WIFI, INPUT_PULLUP);
	bool shouldResetWiFI = ! digitalRead(PIN_RESET_WIFI);
	Serial.println("shouldResetWiFI?: " + String(shouldResetWiFI ? "true" : "false"));

	// Load saved configuration
	Serial.println(F("Loading configuration..."));
	appConfig.load();

	// tell AutoWiFi about the configuration
	autoWiFi.setConfigMQTTName(String(appConfigData.MQTTName));
	autoWiFi.setConfigMQTTBroker(String(appConfigData.MQTTBroker));
	autoWiFi.setConfigMQTTPort(String(appConfigData.MQTTPort));
	
	// Since we have custom portal configuration, we need to tell it how to save the data
	autoWiFi.setSaveConfigCallback(savePortalConfigurationCB);

	autoWiFi.getWebServer()->on("/", rootPage);

	autoWiFi.setup(shouldResetWiFI);

	// List the filesystem
	if(!SPIFFS.begin()) {
		Serial.println(F("SPIFFS: Mount Failed"));
		return;
	} else {
		Serial.println(F("SPIFFS: OK"));
		listDir(SPIFFS, "/");
	}
	
#ifdef USE_LEDSTRIP
	generateGammaTable();

	pinMode(PIN_ENABLESTRIP, OUTPUT);
	digitalWrite(PIN_ENABLESTRIP, LOW);
	
	strip.Begin();
    strip.Show();
    
   	nextDisplayUpdate = 0;
#endif
   
	for(int i = 0; i < NUM_LEDSTRIP; i++) {
		strip.SetPixelColor(i, RgbColor(0, 0, 0));
	}
	strip.Show();
	
	nextRebootUpdate = 0;
	
//  	timeClient.setUpdateInterval(5000);
	timeClient.begin();
	nextNTPUpdate = 0;
	timeClient.update();

	Serial.println("Daytime intensities: " + String(appConfigData.dimLEDPercentage) + "/" + String(appConfigData.brightLEDPercentage) );
	Serial.println("Nighttime intensities: " + String(appConfigData.dimNightLEDPercentage) + "/" + String(appConfigData.brightNightLEDPercentage) );
	Serial.println("Hue Shift: " + String(appConfigData.LEDHuePercentage));
//	test();
	
	nextTemperatureUpdate = 0;
	displaySleepSeconds = 0;
	
	Serial.println(F("Display test.\n"));
	for(int i = 0; i < 60; i++) {
		strip.SetPixelColor(i, RGB(64, 0, 0));
		strip.Show();
		delay(15);
		autoWiFi.tick();
	}	
		display.tick();
	for(int i = 0; i < 60; i++) {
		strip.SetPixelColor(i, RGB(0, 64, 0));
		strip.Show();
		delay(15);
		autoWiFi.tick();
	}	
	for(int i = 0; i < 60; i++) {
		strip.SetPixelColor(i, RGB(0, 0, 64));
		strip.Show();
		delay(15);
		autoWiFi.tick();
	}	
	for(int i = 0; i < 60; i++) {
		strip.SetPixelColor(i, RGB(64, 64, 64));
		strip.Show();
		delay(15);
		autoWiFi.tick();
	}	
	for(int i = 0; i < 60; i++) {
		strip.SetPixelColor(i, RGB(0, 0, 0));
		strip.Show();
		delay(15);
		autoWiFi.tick();
	}	
	display.tick();

	delay(2000);
	Serial.println(F("Ready.\n"));
	display._clear();
}


void loop() {
	char buffer[30];

	if(displayState == DISP_ST_NIGHT || displayState == DISP_ST_DAY || displayState == DISP_ST_SLEEP ) {
		display.tick();
		autoWiFi.tick();
	}
	
	unsigned long now = millis()/1000;
	
//  	Serial.println("now: " + String(now));
//  	Serial.println("nextNTPUpdate: " + String(nextNTPUpdate));

	if(nextRebootUpdate > 0 && now >= nextRebootUpdate) {
		Serial.println("Restarting ...");
		delay(3);
	    ESP.restart();
	}

	if(now >= nextNTPUpdate) {
		Serial.println("Time Update: " + String(now) + "/" + String(nextNTPUpdate) + " ");
		display.D("Updating Time...");
					
		if(timeClient.forceUpdate()) {
			Serial.println(F("Happy update."));
			nextNTPUpdate = now + 3600;
		} else {
			Serial.println(F("Sad update."));
		}
	}
	
	if(now >= nextTemperatureUpdate && openWeatherTries < OPENWEATHER_MAX_TRIES) {
		if(strlen(appConfigData.owAppKey) != 0) {
			Serial.println("Weather Update: " + String(now) + "/" + String(nextTemperatureUpdate) + " Attempt: " + openWeatherTries );
			display.D("Updating Weather...");
	
			String owURL = "http://api.openweathermap.org/data/2.5/weather?"
						   "lat=" + String(appConfigData.latitude) +
						   "&lon=" + String(appConfigData.longitude) +
						   "&exclude=hourly,minutely&units=imperial" +
						   "&appid=" + appConfigData.owAppKey;
					
			Serial.println("GET " + owURL);
	
			WiFiClient client;
			HTTPClient http;
	
			http.begin(client, owURL.c_str());
			httpResponseCode = http.GET();	// this is global so we can track the last call
	
			if(httpResponseCode > 0) {
				Serial.print("HTTP Response code: ");
				Serial.println(httpResponseCode);
				String payload = http.getString();
				Serial.println(payload);
			
				StaticJsonDocument<1000> doc;
				DeserializationError error = deserializeJson(doc, payload);
	
				if(error) {
					Serial.print(F("AppConfig: Unable to read configuration: "));
					Serial.println(error.c_str());
				} else if(httpResponseCode == 200) {
					timezone = doc["timezone"];
					Serial.print("timezone: ");
					Serial.println(timezone / 3600);

					JsonObject mainObj = doc["main"];
	
					if (mainObj.containsKey("temp")) {
						temperature = int(mainObj["temp"].as<float>() + 0.5f);
						Serial.print("Temperature: ");
						Serial.println(temperature);
						
						nextTemperatureUpdate = now + OPENWEATHER_UPDATE;
						
 						timeClient.setTimeOffset(timezone);
						
						openWeatherTries = 0;
					} else {
						Serial.println("Key 'temp' not found");
						nextTemperatureUpdate = now + OPENWEATHER_UPDATE;
						openWeatherTries ++;
					}
				} else {
					nextTemperatureUpdate = now + OPENWEATHER_UPDATE;
					openWeatherTries ++;
				}
			} else {
				Serial.print("Error code: ");
				Serial.println(httpResponseCode);
				nextTemperatureUpdate = now + OPENWEATHER_UPDATE;
				openWeatherTries ++;
			}
	
			http.end();
		} else {
			Serial.println("Weather Update skipped. No key.");
			nextTemperatureUpdate = now + OPENWEATHER_UPDATE;
		}
	}
	
// #define DEBUG_NTPClient 1

	RgbColor p;
 	if(timeClient.isTimeSet()) {

		if((displayStep > 0) && (millis() >= nextDisplayUpdate)) {
			if(displayState == DISP_ST_NODDING) {
				float pos = (float)displayStep / DISP_FADE_STEPS;
				
				dimPct = (float) (appConfigData.dimNightLEDPercentage + 
					(appConfigData.dimLEDPercentage - appConfigData.dimNightLEDPercentage) * pos)/100; 
				brightPct = (float) (appConfigData.brightNightLEDPercentage + 
					(appConfigData.brightLEDPercentage - appConfigData.brightNightLEDPercentage) * pos)/100; 

			}
			if(displayState == DISP_ST_WAKING) {
				float pos = 1.0 - (float)displayStep / DISP_FADE_STEPS;
				
				dimPct = (float) (appConfigData.dimNightLEDPercentage + 
					(appConfigData.dimLEDPercentage - appConfigData.dimNightLEDPercentage) * pos)/100; 
				brightPct = (float) (appConfigData.brightNightLEDPercentage + 
					(appConfigData.brightLEDPercentage - appConfigData.brightNightLEDPercentage) * pos)/100; 
			}
			if(displayState == DISP_ST_SLEEPING) {
				float pos = (float)displayStep / DISP_FADE_STEPS;
				
				dimPct = (float) (0 + 
					(appConfigData.dimLEDPercentage - 0) * pos)/100; 
				brightPct = (float) (0 + 
					(appConfigData.brightLEDPercentage - 0) * pos)/100;
			}

			displayStep --;

			if(displayState == DISP_ST_NODDING && displayStep == 0)		displayState = DISP_ST_NIGHT;
			if(displayState == DISP_ST_WAKING && displayStep == 0)		displayState = DISP_ST_DAY;
			if(displayState == DISP_ST_SLEEPING && displayStep == 0)	displayState = DISP_ST_SLEEP;

			nextDisplayUpdate = millis() + DISP_FADE_TIME;
		} else if(displayState == DISP_ST_DAY) {
			dimPct = (float) appConfigData.dimLEDPercentage/100;
			brightPct = (float) appConfigData.brightLEDPercentage/100;
		} else if(displayState == DISP_ST_NIGHT) {
			dimPct = (float) appConfigData.dimNightLEDPercentage/100;
			brightPct = (float) appConfigData.brightNightLEDPercentage/100;
		}

		tempPct = (float) appConfigData.temperatureLEDPercentage/100;

		epoch = timeClient.getEpochTime();

		if(
			((displayState == DISP_ST_NODDING || displayState == DISP_ST_WAKING || displayState == DISP_ST_SLEEPING) && millis() >= nextDisplayUpdate)
			|| lastEpoch < epoch) {

			int h = timeClient.getHours();
			int m = timeClient.getMinutes();
			int	s = timeClient.getSeconds();

			// If it's night and we think it should be day
			if((displayState == DISP_ST_NIGHT) && (h >= appConfigData.dayHour) && (h < appConfigData.nightHour)) {
// 			if((displayState == DISP_ST_NIGHT) && ! digitalRead(PIN_RESET_WIFI)) {
				displayState = DISP_ST_WAKING;
				displayStep = DISP_FADE_STEPS;
				nextDisplayUpdate = millis() + DISP_FADE_TIME;
			}

			// If it's day and we think it should be night
			if((displayState == DISP_ST_DAY) && !((h >= appConfigData.dayHour) && (h < appConfigData.nightHour))) {
// 			if((displayState == DISP_ST_DAY) && digitalRead(PIN_RESET_WIFI)) {
				displayState = DISP_ST_NODDING;
				displayStep = DISP_FADE_STEPS;
				nextDisplayUpdate = millis() + DISP_FADE_TIME;
			}

			if((displayState == DISP_ST_DAY || displayState == DISP_ST_NIGHT) && (displaySleepSeconds)) {
				displayState = DISP_ST_SLEEPING;
				displayStep = DISP_FADE_STEPS;
				nextDisplayUpdate = millis() + DISP_FADE_TIME;
			}
			
			if(displayState == DISP_ST_DAY || displayState == DISP_ST_NIGHT || displayState == DISP_ST_SLEEP) {
				snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", h, m, s);

				Serial.print(String(buffer) + " " + String(timezone / 3600) + " " + String(nextNTPUpdate - now) + 
					" T: " + String(temperature) + " " + String(nextTemperatureUpdate - now) +
					(displayState == DISP_ST_SLEEP ? " Sleep" : (displayState == DISP_ST_DAY ? " Day":" Night") ));
				Serial.println(" [" + String(dimPct) + ", " + String(brightPct) + "]");

				if(! digitalRead(PIN_RESET_WIFI)) {
					display._clear();
	
					display.at(String(buffer) + " " + String(timezone / 3600) + " " + String(nextNTPUpdate - now) +
						(displayState == DISP_ST_SLEEP ? " Sleep" : (displayState == DISP_ST_DAY ? " Day":" Night") ), 0);
// 					display.at(WiFi.localIP().toString(), 12);
					display.at("T: " + String(temperature) + " " + String(nextTemperatureUpdate - now) + 
						" I: " + String((uint8_t)(dimPct*100)) + "% " + String((uint8_t)(brightPct*100)) + "%", 24);
					display.at(String(now) + " " + String(nextTemperatureUpdate), 12);
				} else {
					display.clearMessage();
				}
			} 

			if(displayState == DISP_ST_SLEEP) {
				Serial.println(" Sleeping: " + String(displaySleepSeconds));
				
				if(displaySleepSeconds == 0) {
					displayState = DISP_ST_WAKING;
					displayStep = DISP_FADE_STEPS;
					nextDisplayUpdate = millis() + DISP_FADE_TIME;
				}
			}
			
			bool isP = h > 12;
			h = h % 12;
			h = h * 5;
		
			for(int i = 0; i < 60; i++)
				strip.SetPixelColor(i, RgbColor(0,0,0));

			for(int i = 0; i < s+1; i++)
				strip.SetPixelColor(i, RgbColor(0, 0, 255));

			for(int i = 0; i < m+1; i++) {
				p = strip.GetPixelColor(i);
				strip.SetPixelColor(i, RgbColor(p.R, 255, p.B));
			}

			for(int i = 0; i < h+1; i++) {
				p = strip.GetPixelColor(i);
				strip.SetPixelColor(i, RgbColor(255, p.G, p.B));
			}

			for(int i = 0; i < 60; i++) {
				if(appConfigData.LEDHuePercentage != 0) {
					p = strip.GetPixelColor(i);

					HsbColor C = HsbColor(p);

					C.H += (float)appConfigData.LEDHuePercentage/100;
					if(C.H > 1.0) C.H -= 1.0;

					strip.SetPixelColor(i, C);
				}

				if(i == temperature)
//					strip.SetPixelColor(i, RgbColor(0, 235, 145));
					strip.SetPixelColor(i, HsbColor((float)appConfigData.LEDColdHuePercentage/100, 1.0f, 1.0f));
				else if (i + 60 == temperature)
// 					strip.SetPixelColor(i, RgbColor(255, 138, 80));
					strip.SetPixelColor(i, HsbColor((float)appConfigData.LEDWarmHuePercentage/100, 1.0f, 1.0f));
				
				// Set the brightnesses for 5s and non-5s
				p = strip.GetPixelColor(i);
				
				if(((displayState == DISP_ST_SLEEP) || (displayState == DISP_ST_NIGHT)) && ((i == temperature) || (i + 60 == temperature))) {
					p.R *= tempPct; p.G *= tempPct; p.B *= tempPct;
				} else if(i % 5 == 0) {
					p.R *= brightPct; p.G *= brightPct; p.B *= brightPct;
				} else {
					p.R *= dimPct; p.G *= dimPct; p.B *= dimPct;
				}


				p = RGB(p.R, p.G, p.B);
				
				strip.SetPixelColor(i, p);
			}
			
		
			strip.Show();

			lastEpoch = epoch;

			if(nextTemperatureUpdate - now > OPENWEATHER_UPDATE) {
				nextTemperatureUpdate = now + OPENWEATHER_RETRY_UPDATE;
				openWeatherTries = 0;
			}	
				
			if((displayState == DISP_ST_SLEEP) && (displaySleepSeconds > 0))
				displaySleepSeconds --;
		}
	} else {
		Serial.println(F("Need Update ..."));

		display._clear();
		display.at(F("Fetching Time ..."), 0);

		for(int i = 0; i < 60; i++) {
			strip.SetPixelColor(i, RGB(64, 64, 64));
			strip.Show();
			delay(50);
			
			strip.SetPixelColor(i, RgbColor(0,0,0));
			strip.Show();

			display.tick();
			autoWiFi.tick();
		}
	}
}

float smap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}