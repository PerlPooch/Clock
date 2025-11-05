#ifndef MDS_APPCONFIG
#define MDS_APPCONFIG
 
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>

#define MDS_APPCONFIG_VERSION	"1.0.0"

#define CONFIG_DOC_SIZE			700


class MDS_AppConfig {
	public:
        MDS_AppConfig();
        ~MDS_AppConfig();
        void 					setup();
		void					load();
		void					save();
		void					print();
		void					setSetConfigCallback(std::function<void(StaticJsonDocument<CONFIG_DOC_SIZE>&)> callback);
		void					setGetConfigCallback(std::function<void(StaticJsonDocument<CONFIG_DOC_SIZE>&)> callback);

	private:
		bool					isSetup = false;
		std::function<void(StaticJsonDocument<CONFIG_DOC_SIZE>&)> setDataCallback;
		std::function<void(StaticJsonDocument<CONFIG_DOC_SIZE>&)> getDataCallback;
};

#endif
