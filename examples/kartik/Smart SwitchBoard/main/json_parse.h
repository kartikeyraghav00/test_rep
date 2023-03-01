#ifndef JSON_PARSE_H
#define JSON_PARSE_H

#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"


#define COMMISION_READY 	"S_01"
#define CONFIG_RECIEVED 	"S_02"
#define WIFI_CONNECTED 		"S_03"
#define INTERNET_CONNECTED 	"S_04"
#define RECOMMISION_DATA 	"S_05"
#define MQTT_ONLINE 		"S_06"


#define CONFIG_PARSING_ERROR 		"E_01"
#define WIFI_SSID_PASSWORD_ERROR 	"E_02"
#define INTERNET_CONNECTED_FAILED 	"E_03"
#define MQTT_CONNECT_FAILED		 	"E_04"
#define MQTT_DISCONNECTED			"E_05"

#define UPDATE_STATUS 1
#define UPDATE_SWITCH_STATUS 2

typedef struct schedule_tpoint_s{
	int id;
	int time;//in mins
	int action;//0-ON,1-OFF,2-Scene,3-delete
	//char action_val;
	//char action_val[CURR_ACTION_VAL];
}schedule_tpoint_t;

typedef struct schedule_s{
	int s_time;// use for sorting in mins = hh*60+mm  from 00 to 23 midnight 24 hrs format00-59 mins
	//duplicate remove later
	int s_action;//0-ON,1-OFF,2-Scene,3-delete
	int e_action;//0-ON,1-OFF,2-Scene,3-delete
	int duration;//minutes
	int state;//0-disable,1-enable, 2 - delete
	//char s_action_val;//colors or scene json
	//char e_action_val;
	//char s_action_val[CURR_ACTION_VAL];//colors or scene json
	//char e_action_val[CURR_ACTION_VAL];
	//char uuid[37];
	char days[8];
}schedule_t;

typedef struct sidx_s{
	int id;
	char uuid[37];
	schedule_t schedules;
}sidx_t;

enum led_color_state{
	GREEN_LED_STEADY=0,
	GREEN_LED_BLINK,
	GREEN_LED_OFF,
	BLUE_LED_STEADY,
	BLUE_LED_BLINK,
	BLUE_GREEN_LED_BLINK,
	BLUE_LED_OFF
};

enum rest_call_type{
	ALEXA,
	INVENTORY,
	ACCESSTOKEN,
	LOCKINFO,
	UPDATEVERSION,
	UPDATEWIFI,
	OTACHECK,
	OTACHECK_DEPLOYMENT,
	OTA_FEEDBACK,
	GETSCHEDULES,
	GET_OTA_URL,
	GET_OTA_URLV2,
	UPDATE_METERING
};

enum light_option{
	COMMISSION=1001,
	SCENE,
	PLAN,
	SCHEDULE,
	AUDIO,
	BRIGHTNESS,
	FLASH_MODE,
	FLASH_SPEED,
	SETTINGS,
	TIMER,
	LIGHT_ON_OFF,
	COLORS,
	UPDATE_WIFI
};

extern enum led_color_state led_state;
extern 	char mac_str[18];
int getLEDColorState();

bool validateJson(char* jsonbuff);

char *getbleNotificationColor();

void setbleNotificationColor(char *status);

char* rest_call(enum rest_call_type rtype);

void parseMqttJson(char *data);

void saveSettings();

void setDndProperty(bool value);

void executeMqttpublishTask();

void setLastKnownStatus(char *value);

int getDndStatus();

char *getMostRecentColor();

void setTurnONState(bool value);

void factoryReset();

void setMostRecentColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white, uint16_t brightness);

void setDndStatus(int val);

void parseConfigJson(char *data);

void setTotalSchedule(int cnt);

int getTotalSchedule();

void parseScheduleJson(char *jsonbuff);

void parseRestScheduleJson(char *jsonbuff);


int parseSettings(char* jsonbuff);

int getLastKnownProperty();

void setLastKnownProperty(int val);

char *getCustomColor();

char* jsonFromHtmlResponse(char *response, enum rest_call_type rtype);

void setLockDeviceUUID(char *did);

char *getLockDeviceUUID();

char* getCurrentSWVersion();

char *getCurrentStatus();

void setCurrentStatus(char *status);

bool isinventoryUpdateReq();

void setinventoryUpdateReq(bool inventory);


void setCurrentSWVersion(char *version);

void setAvailableVersion(char *ver);
char * getAvailableVersion();

void setLEDColorState(enum led_color_state ledstate);

int getCommissionState();

void setCommisionState(int state);

char* getHUBMacID();

void setBuff(char *jBuff);

char* getBuffer();

void setJsonParseStatus(int s);

int getJsonParseStatus();

void setSpid(char *sid);

char *getSpid();

void setmqttURL(char *murl);

char *getmqttURL();

void sethttpURL(char *hurl);

char *gethttpURL();

long currentSec(void);

char* getDeploymentID();

void CheckOTAAvailable();

void setDeploymentID(char* depId);

char *getOtaTargetToken();

void setOtaTargetToken(char *ota_tok);

char *getDeploymentResource();

void setDeploymentResource(char *dep_res);

int isDeploymentBaseAvailable();

// char *getOTAResource();

// void setOTAResource(char *ota_res);

void setOtaFileResource(char *res);

char* getOtaFileResource();

void setOTA_URL(char *ourl);

uint32_t getOTAStatus();

uint32_t getHubResetStatus();

void setOTAStatus(uint32_t ot_status);

void setHubResetStatus(uint32_t rst_status);

void setAvailableOTAVersion(char* available_version);

char* getAvailableOTAVersion();

void setAccessToken(char *atok);

void setAccessTokenValidity(int valid);

char *getAccessToken();

void setRefreshToken(char *rtok);

char *getRefreshToken();

void setAction(int act);

int getAction();

void setSsid(char *sid);

char *getSsid();

char *getPwd();

char* getMqttPort();
char* getHttpPort();
void setHttpPort(char *tmp);
void setMqttPort(char *tmp);

char *getMurlWithPort();
char *getHurlWithPort();

void setPwd(char *pass);

void setDeviceUUID(char *did);

char *getDeviceUUID();

void setGWUUID(char *uid);

char *getGWUUID();

void setOtaToken(char *uid);

char *getOtaToken();

void setUnitUUID(char *uid);

char *getUnitUUID();

void setEntityUUID(char *eid);

char *getEntityUUID();

int getAccessTokenValidity();
void setAccessTokenValidity(int valid);

int parseConf(char* jsonbuff, int parse_access);

void executecommissionStatustask();

esp_mqtt_client_handle_t getMQTTClient();

void removeMQTTColorTask();

void executeMqttColorTask(char *mqttData);

void parseBleControlCommand(char *jsonbuff);

void parseColorMqttJson(char *jsonbuff);

void fadeColorMqtt(uint16_t *destination);

void connectWifi();
void connectWifiCommision(char *s, char *p);

void setTotalRecords(int value);

int getTotalRecords();

void setdownloadedRecords(int value);

int getdownloadedRecords();

int writeNVData(char *key, char *value);

char *getTimeZone();

void setTimeZone(char *tzTime);

void turnOnBulb(char * color);

void turnOFFBulb();

bool getTurnONState();

void setSecurityKey(char *key);

char *getSecurityKey();

void setRecommisionStatus(int status);

int getRecommisionStatus();

#endif /* JSON_PARSE_H */
