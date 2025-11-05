#include "MDS_AppConfig.h"
#include <functional>
#include <FS.h>

#define CONFIG_FILE			"/config.json"
#define CONFIG_DOC_SIZE			700


MDS_AppConfig::MDS_AppConfig() {
}

MDS_AppConfig::~MDS_AppConfig() {
}

void MDS_AppConfig::setup() {
	if(isSetup)
		return;

	if(&Serial != NULL) {
		Serial.print(F("MDS_AppConfig V"));
		Serial.println(F(MDS_APPCONFIG_VERSION));
	}

	if(&SPIFFS == NULL) {
		if(&Serial != NULL)
			Serial.println(F("Fatal: SPIFFS allocation failed."));

		for(;;); // Don't proceed, loop forever
	}
	SPIFFS.begin();
	
	isSetup = true;
}

void MDS_AppConfig::load() {
//	Serial.println(F("MDS_AppConfig::load"));
	File file = SPIFFS.open(CONFIG_FILE, "r");

	if(file) {
		StaticJsonDocument<CONFIG_DOC_SIZE> doc;

		DeserializationError error = deserializeJson(doc, file);

		if(error) {
			Serial.print(F("AppConfig: Unable to read configuration: "));
			Serial.println(error.c_str());
		} else {
			setDataCallback(doc);
		} 

		file.close();
	} else {
		Serial.print(F("AppConfig: Unable to read configuration '"));
		Serial.print(CONFIG_FILE);
		Serial.println(F("': not found."));
	}
}


void MDS_AppConfig::save() {
//	Serial.println(F("MDS_AppConfig::save()"));

	SPIFFS.remove(CONFIG_FILE);

	File file = SPIFFS.open(CONFIG_FILE, "w");
	if (!file) {
		Serial.println(F("AppConfig: Unable to create configuration file"));
		return;
	}

	StaticJsonDocument<CONFIG_DOC_SIZE> doc;

	getDataCallback(doc);

	if (serializeJson(doc, file) == 0) {
		Serial.println(F("AppConfig: Unable to write configuration."));
	}

	file.close();
}


void MDS_AppConfig::print() {
//	Serial.println(F("MDS_AppConfig::printFile()"));

	File file = SPIFFS.open(CONFIG_FILE, "r");
	if (!file) {
		Serial.println(F("AppConfig: Unable to read file"));
		return;
	}

	while (file.available()) {
		Serial.print((char)file.read());
	}
	Serial.println();

	file.close();
}


void MDS_AppConfig::setSetConfigCallback(std::function<void(StaticJsonDocument<CONFIG_DOC_SIZE>&)> callback) {
//	Serial.println(F("MDS_AppConfig::setSetConfigCallback"));
	setDataCallback = callback;
}


void MDS_AppConfig::setGetConfigCallback(std::function<void(StaticJsonDocument<CONFIG_DOC_SIZE>&)> callback) {
//	Serial.println(F("MDS_AppConfig::setGetConfigCallback"));
	getDataCallback = callback;
}





