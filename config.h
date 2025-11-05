#define WIFI_CONFIG_NAME	"CLOCK"

#define PIN_RESET_WIFI 		12
#define PIN_LED 			16
#define PIN_LED_1			2		// NodeMCU Secondary LED (shares pin with relay!)
#define	PIN_ENABLESTRIP		14

#ifdef USE_LEDSTRIP
#define PIN_LEDSTRIP		3		// Pin is _fixed to be GPIO3 (RX pin)
#define NUM_LEDSTRIP		60
#endif

#define	DISP_FADE_TIME				2
#define DISP_FADE_STEPS				255
#define OPENWEATHER_MAX_TRIES		2
#define OPENWEATHER_UPDATE			600
#define OPENWEATHER_RETRY_UPDATE	60

// ---- AppConfig

#define jc_IS_CONFIGURED				"isConfigured"
#define jc_MQTTNAME						"MQTTName"
#define jc_MQTTBROKER					"MQTTBroker"
#define jc_MQTTPORT						"MQTTPort"
#define jc_DIMLEDPERCENTAGE				"dim"				// 0 .. 100. Brightness of standard dots when in Day time
#define jc_BRIGHTLEDPERCENTAGE			"bright"			// 0 .. 100. Brightness of Tick dots when in Day time
#define jc_DIMNIGHTLEDPERCENTAGE		"dimNight"			// 0 .. 100. Brightness of standard dots when in Night time
#define jc_BRIGHTNIGHTLEDPERCENTAGE		"brightNight"		// 0 .. 100. Brightness of Tick dots when in Night time
#define jc_TEMPERATUREPERCENTAGE		"temp"				// 0 .. 100. Brightness of temperature dot when sleeping
#define jc_LEDHUEPERCENTAGE				"hue"				// 0 .. 100. Universal color shift to apply 
#define jc_LEDCOLDHUEPERCENTAGE			"coldHue"			// 0 .. 100. Color of temperature dot when temperature < 60°
#define jc_LEDWARMHUEPERCENTAGE			"warmHue"			// 0 .. 100. Color of temperature dot when temperature > 60°
#define jc_NIGHTHOUR					"nightHour"			// 0 .. 23. Hour to start Night time
#define jc_DAYHOUR						"dayHour"			// 0 .. 23. Hour to end Night time
#define jc_OWKEY						"owAppKey"			// <string>. OpenWeather API Key
#define jc_OWLAT						"latitude"			// <float>. Location Latitude
#define jc_OWLON						"longitude"			// <float>. Location Longitude
#define jc_DSLEEP						"displaySleep"		// <int>. Sleep the display for this many seconds.
#define jc_REBOOT						"reboot"			// <>. Reboot

#define st_DSLEEPREMAINING				"sleepRemaining"	// <int>. Remaining sleep time in seconds.
#define st_TIMEZONE						"timezone"			// <int>. Timezone from OW
#define st_NEXTNTPUPDATE				"nextNTPUpdate"		// <int>. Number of seconds until the next NTP update
#define st_NEXTOWUPDATE					"nextOWUpdate"		// <int>. Number of seconds until the next OpenWeather update
#define st_TEMPERATURE					"temperature"		// <int>. Current temperature in °F
#define st_LASTOWRESPONSE				"lastOWResponse"	// <int>. Last HTTP response from OpenWeather
#define st_TIME							"time"				// <string>. Current time
#define st_DISPLAY						"display"			// <string>. Display state [Day|Night|Sleep|Preparing]
#define st_VERSION						"version"			// <string>. Clock firmware version
