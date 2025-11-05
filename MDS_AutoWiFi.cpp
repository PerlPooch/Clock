#include "MDS_AutoWiFi.h"
#include "config.h"

#define AUX_SETTING_URI		"/mqtt_setting"
#define AUX_SAVE_URI		"/mqtt_save"
#define AUX_CLEAR_URI		"/mqtt_clear"

MDS_AutoWiFi::MDS_AutoWiFi(): Portal(Server), display(nullptr) {
}

MDS_AutoWiFi::~MDS_AutoWiFi() {
}

void MDS_AutoWiFi::setMAC()
{
	// Compute the systemID -- The unique ID for this device. This will be used as the root leaf
	// for the MQTT topic and as the WiFi configuration AP name
	String mac = WiFi.macAddress();
	mac.getBytes((byte *)systemID, sizeof(systemID));
}


void MDS_AutoWiFi::setAutoConnectConfig(AutoConnectConfig &acConfig) {
	if (strlen(systemID) == 0)
		setMAC();

	acConfig.apid = String(WIFI_CONFIG_NAME) + " " + String(systemID);
	acConfig.psk = "12345678";
	acConfig.ota = AC_OTA_BUILTIN;
	acConfig.title = String(systemID);
	acConfig.homeUri = "/";
	acConfig.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_DISCONNECT | AC_MENUITEM_RESET | AC_MENUITEM_UPDATE;

	Serial.println(F("AutoWiFi: ") + String(acConfig.apid));
}


void MDS_AutoWiFi::checkForReset(bool &shouldResetWiFI, AutoConnectConfig &acConfig) {
	Serial.println(F("AutoWiFi: checkForReset()"));

	// Add check for button state to force

	// Check for the credentials file. If it's missing, force configuration	
	if(! SPIFFS.exists("/ac_credt")) {
		Serial.println("AutoWiFi: Configuration missing, forcing portal");
		shouldResetWiFI = true;
	} else {
		Portal.restoreCredential("/ac_credt", SPIFFS);
    }

	if (shouldResetWiFI == true) {
		Serial.println(F("AutoWiFi: WiFi AP Configuration"));

		if(display != NULL) {
			display->D(F("WiFi AP Configuration"));
			display->D("");
			display->D(String(acConfig.apid));
			display->D("PW: " + String(acConfig.psk));
		}

		acConfig.immediateStart = true;
		acConfig.autoRise = true;
	}
}


void MDS_AutoWiFi::setup(bool shouldResetWiFI) {
	Serial.println("AutoWiFi: setup() " + String(shouldResetWiFI ? "ShouldReset" : "NoReset"));

	AutoConnectConfig acConfig;

	setAutoConnectConfig(acConfig);
	
	if(Portal.load(FPSTR(CUSTOM_SETTINGS))) {
		AutoConnectAux& mqtt_setting = *Portal.aux(AUX_SETTING_URI);

		AutoConnectInput& name = mqtt_setting.getElement<AutoConnectInput>("mqttname");
		name.value = getConfigMQTTName();

		AutoConnectInput& broker = mqtt_setting.getElement<AutoConnectInput>("mqttserver");
		broker.value = getConfigMQTTBroker();

 		AutoConnectInput& port = mqtt_setting.getElement<AutoConnectInput>("mqttport");
 		port.value = getConfigMQTTPort();

		Portal.on(AUX_SAVE_URI, std::bind(&MDS_AutoWiFi::saveMQTTParams, this,
                          std::placeholders::_1,
                          std::placeholders::_2));
    } else {
		Serial.println("AutoWiFi: Portal load error");
	}


	checkForReset(shouldResetWiFI, acConfig);

	Portal.config(acConfig);
	
	if (Portal.begin()) { 
		// If we reach here, the portal is configured and has successfully connected to an AP
		if(shouldResetWiFI) {
			Serial.println("AutoWiFi: Saving credentials");
			Portal.saveCredential("/ac_credt", SPIFFS);
		}
	}

	if(display != NULL) {
		display->D("IP: " + WiFi.localIP().toString());
	} else {
		Serial.println("AutoWiFi: IP: " + WiFi.localIP().toString());
	}
}


String MDS_AutoWiFi::saveMQTTParams(AutoConnectAux& aux, PageArgument& args) {
	AutoConnectAux* mqtt_setting = Portal.aux(Portal.where());
	AutoConnectInput& nameInput = mqtt_setting->getElement<AutoConnectInput>("mqttname");
	AutoConnectInput& serverInput = mqtt_setting->getElement<AutoConnectInput>("mqttserver");
	AutoConnectInput& portInput = mqtt_setting->getElement<AutoConnectInput>("mqttport");

	String nameValue = nameInput.value;
	String serverValue = serverInput.value;
	String portValue = portInput.value;

	setConfigMQTTName(nameValue);
	setConfigMQTTBroker(serverValue);
	setConfigMQTTPort(portValue);

	Serial.println(F("AutoWiFi: saveMQTTParams() Configuration updated."));

	saveConfig();
	
	AutoConnectText&  result = aux["parameters"].as<AutoConnectText>();
	result.value = "Device Name: " + nameValue + "<br>Broker: " + serverValue + ":" + portValue;

	return String("");
}


void MDS_AutoWiFi::saveConfig() {
	if(&saveCallback != NULL)
		saveCallback(getConfigMQTTName(), getConfigMQTTBroker(), getConfigMQTTPort());
}

void MDS_AutoWiFi::tick() {
	Portal.handleClient();
}

// ----------------

AutoConnect* MDS_AutoWiFi::getPortal() {
	return &Portal;
}

ESP8266WebServer* MDS_AutoWiFi::getWebServer() {
	return &Server;
}

char* MDS_AutoWiFi::getSystemID() {
	return systemID;
}

void MDS_AutoWiFi::setConfigMQTTName(String b) {
	config_MQTTName = b;
}

String MDS_AutoWiFi::getConfigMQTTName() {
	return config_MQTTName;
}

void MDS_AutoWiFi::setConfigMQTTBroker(String b) {
	config_MQTTBroker = b;
}

String MDS_AutoWiFi::getConfigMQTTBroker() {
	return config_MQTTBroker;
}

void MDS_AutoWiFi::setConfigMQTTPort(String p) {
	config_MQTTPort = p;
}

String MDS_AutoWiFi::getConfigMQTTPort() {
	return config_MQTTPort;
}

void MDS_AutoWiFi::setSaveConfigCallback(std::function<void(String, String, String)> callback) {
	saveCallback = callback;
}

void MDS_AutoWiFi::setDisplay(MDS_Display& d) {
	display = &d;
}



