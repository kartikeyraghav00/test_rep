#include "json_parse.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <esp_spiffs.h>
#include "esp_system.h"
#include "esp_timer.h"
#include <sys/stat.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/dirent.h>
#include "example_common_private.h"

#include "driver/gpio.h"

static const char *TAG = "JsonParsing";
void turnOnPlug(int viaMqtt, uint8_t numb);
void turnOffPlug(int viaMqtt, uint8_t numb);

#define ENABLE_OTA

#if 1
int action = -1;
char mport[5];
char hport[5];
int is_dep_base_available = -1;
int commission_state=-1;
int access_valid = 1;
uint32_t ota_status=0;
uint32_t reset_status=0;
char *jsonBuff = NULL;
int publishCounter = 3;
// char bleNotificationColor[80];
//char charrgb[80] = {};
//char colorWarmthAttribute[80];
//char colorRGBAttribute[80];
//char rebootColor[80];
//char customColor[80];
uint8_t lastKnownProperty = 1;
uint8_t dndEnable = 0;
char currentSWVersion[32];
char plainTex[1500] = {};

char ssid[64];
char pwd[64];
char deviceUUID[37];
//char ota_token[37];
char GWUUID[37];
char entityUUID[37];
char unitUUID[37];
char spid[33];
char mqttURL[64];
char httpURL[64];
//char ota_resource[128];
//char dep_resource[12];
char available_version[32];
char accessToken[400];
char refreshToken[350];
char secKey[20];
char timeZone[8];
bool isOn = false;
bool inventoryDBUpdate = false;
volatile int json_parse_status = 0;
TaskHandle_t mqttParse_task_hdl = NULL;
TaskHandle_t mqttColor_task_hdl = NULL;
char currentStatus[10];
int rcStatus = 0;
int tRecords = 0;
int downloadedRecords = 0;

void AESECB128_NOPADDING(char *data, char * plainTex, char * key);

void startOTA(char *otaUrl);
void notify_over_control_chr(int type);
void startRestCallTask();

void executeTimerTask(char *sceneData);
void removeTimerTask();

void setTotalRecords(int value)
{
	tRecords = value;
}

int getTotalRecords()
{
	return tRecords;
}

void setdownloadedRecords(int value)
{
	downloadedRecords = value;
}

int getdownloadedRecords()
{
	return downloadedRecords;
}

// char *getbleNotificationColor()
// {
// 	return bleNotificationColor;
// }

// void setbleNotificationColor(char *status)
// {
// 	ESP_LOGI(TAG,"Current Status ----- [%s]",status);
// 	memset(bleNotificationColor,0,sizeof(bleNotificationColor));
// 	strcpy(bleNotificationColor,status);
// 	notify_over_control_chr(UPDATE_SWITCH_STATUS);
// }

char *getCurrentStatus()
{
	return currentStatus;
}

void setCurrentStatus(char *status)
{
	ESP_LOGI(TAG,"Current Status ----- [%s]",status);
	memset(currentStatus,0,sizeof(currentStatus));
	strcpy(currentStatus,status);
	notify_over_control_chr(UPDATE_STATUS);
}


bool isinventoryUpdateReq()
{
	return inventoryDBUpdate;
}

void setinventoryUpdateReq(bool inventory)
{
	inventoryDBUpdate = inventory;
}

char *getTimeZone()
{	
	return timeZone;
}

void setTimeZone(char *tzTime)
{
	memset(timeZone,0,sizeof(timeZone));
	strcpy(timeZone,tzTime);
}


void setSecurityKey(char *key)
{
	memset(secKey,0,sizeof(secKey));
	strcpy(secKey,key);
}

char *getSecurityKey()
{
	return secKey;
}

void setRecommisionStatus(int key)
{
	rcStatus = key;
}

int getRecommisionStatus()
{
	return rcStatus;
}


char *getPwd(){
	return pwd;
}

int getDndStatus()
{
	return dndEnable;
}
void setDndStatus(int val)
{
	dndEnable = val;
}

void setLastKnownProperty(int val)
{
	lastKnownProperty = val;
}

int getLastKnownProperty()
{
	return lastKnownProperty;
}

// char *getCustomColor()
// {
// 	return customColor;
// }

// char *getMostRecentColor()
// {
// 	return rebootColor;
// }
// char* getCurrentSWVersion()
// {
// 	printf("\n############### => getCurrentSWVersion => %s\n",currentSWVersion);
// 	return currentSWVersion;
// }

void setCurrentSWVersion(char *version)
{
	memset(currentSWVersion,0,sizeof(currentSWVersion));
	strcpy(currentSWVersion,version);
	printf("\n############### => setCurrentSWVersion => %s\n",currentSWVersion);
}

void setCommissionState(int state){
	commission_state = state;
}

void setTurnONState(bool value)
{
	isOn = value;
}

bool getTurnONState()
{
	return isOn;
}

int getCommissionState(){
	ESP_LOGI(TAG, "<< commission_state - %d >>",commission_state);
	return commission_state;
}

void setJsonParseStatus(int s){
	json_parse_status = s;
}

int getJsonParseStatus(){
	return json_parse_status;
}

void setSpid(char *sid){
	memset(spid,0,sizeof(spid));
	strcpy(spid,sid);
}

char *getSpid(){
	return spid;
}

void setMqttPort(char *tmp){
	memset(mport,0,sizeof(mport));
	strcpy(mport,tmp);
}

char* getMqttPort(){
	return mport;
}

char* getMurlWithPort(){
	return strcat(strcat(getmqttURL(),":"),getMqttPort());
}

char* getHurlWithPort(){
	return strcat(strcat(gethttpURL(),":"),getHttpPort());
}

void setmqttURL(char *murl){
	memset(mqttURL,0,sizeof(mqttURL));
	strcpy(mqttURL,murl);
}

char *getmqttURL(){
	return mqttURL;
}

void setHttpPort(char *tmp){
	memset(hport,0,sizeof(hport));
	strcpy(hport,tmp);
}

char* getHttpPort(){
	return "443";//hport;
}

void sethttpURL(char *hurl){
	memset(httpURL,0,sizeof(httpURL));
	int n = strlen(hurl);
	char *resource = strstr(hurl,".com");
    resource = resource + 5;// skip .com:
    int pl = strlen(resource);
    strncpy(httpURL,hurl,n-pl-1);
	ESP_LOGI(TAG, "http-port:%s", resource);
	setHttpPort(resource);
}

char *getCurrentTime_ms(){
	//TBD - 
	return NULL;
}

char *gethttpURL(){
	return httpURL;
}

void setAccessToken(char *atok){
	//updateAccessToken(atok);
	struct stat st;
    	size_t bytCount=0;
	memset(accessToken,0,sizeof(accessToken));
	if(atok == NULL){
		FILE *fin = fopen("/spiffs/atok.txt", "r");
        if (fin == NULL) {
            ESP_LOGE(TAG, "Failed to open atok.txt file for reading");
            return;
        }
        fscanf(fin, "%s", accessToken);
        ESP_LOGI(TAG, "Read from atok.txt file : %s\n",accessToken);
        // while (! feof (fin)) {
        //     bytCount = fread (accessToken, 1, sizeof(accessToken), fin);//(void *ptr, size_t size, size_t nmemb, FILE *stream);
        //     ESP_LOGI(TAG, "Read from atok.txt file: bytes : %d\n",bytCount);
        // }
	} else {
		if(stat("/spiffs/atok.txt", &st) == 0 && st.st_size > 50){
			unlink("/spiffs/atok.txt");
		}
		FILE* f = fopen("/spiffs/atok.txt", "w");
	    if (f == NULL) {
	        ESP_LOGE(TAG, "setAccessToken Failed to open config file for writing");
	        return;
	    }
	    bytCount = fwrite (atok, 1, 400, f);
	    fclose(f);
	    ESP_LOGI(TAG, "setAccessToken written bytes : %d\n", bytCount);
	    strcpy(accessToken,atok);
	}
}

char *getAccessToken(){
	return accessToken;
}

void setRefreshToken(char *rtok){
	memset(refreshToken,0,sizeof(refreshToken));
	strcpy(refreshToken,rtok);
}

char *getRefreshToken(){
	return refreshToken;
}

void setAction(int act){
	action = act;
}

int getAction(){
	return action;
}

void setSsid(char *sid){
	memset(ssid,0,sizeof(ssid));
	strcpy(ssid,sid);
}

char *getSsid(){
	return ssid;
}
void setPwd(char *pass){
	memset(pwd,0,sizeof(pwd));
	strcpy(pwd,pass);
}


void setDeviceUUID(char *did){
	memset(deviceUUID,0,sizeof(deviceUUID));
	strcpy(deviceUUID,did);
}

char *getDeviceUUID(){
	return deviceUUID;
}

void setGWUUID(char *uid){
	memset(GWUUID,0,sizeof(GWUUID));
	strcpy(GWUUID,uid);
}

char *getGWUUID(){
	return GWUUID;
}

void setUnitUUID(char *uid){
	memset(unitUUID,0,sizeof(unitUUID));
	strcpy(unitUUID,uid);
}

char *getUnitUUID(){
	return unitUUID;
}

void setEntityUUID(char *eid){
	memset(entityUUID,0,sizeof(entityUUID));
	strcpy(entityUUID,eid);
}

char *getEntityUUID(){
	return entityUUID;
}

void setAccessTokenValidity(int valid){
	access_valid = valid;
}

int getAccessTokenValidity(){
	return access_valid;
}

// void setMostRecentColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white)
// {
// 		char* strUpdate="%d,%d,%d,%d,%d";
//         memset(rebootColor,0,sizeof(rebootColor));
//         sprintf(rebootColor, strUpdate,red, green, blue, warm_white,  cool_white);
//         printf("#############most Recent Color Set    %s",rebootColor);


// }

int parseConf(char* jsonbuff, int parse_access){

	cJSON_Hooks memoryHook;

	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);

 	ESP_LOGI(TAG, "parseConf Entry");
    cJSON *root = cJSON_Parse(jsonbuff);
    if(root != NULL){

	    if(cJSON_GetObjectItem(root, "a") != NULL){//if key not present then NULL is returned
	    	int tmp = cJSON_GetObjectItem(root, "a")->valueint;
	    	setAction(tmp);
		    ESP_LOGI(TAG, "action:%d", getAction());
	    	if(tmp == 2) {goto update_wifi;}//for wifi update
	    	if(tmp != 1) {goto hubconfexit;}//for reset or invalid

		}
//if(getAction() == 1001)
if(getAction() == 1)
{
		// if(cJSON_GetObjectItem(root, "lmac") != NULL){//if key not present then NULL is returned
	 //    	char *tmp = cJSON_GetObjectItem(root, "lmac")->valuestring;
	 //    	setlockMacID(tmp);
		//     printf("\nlmac:%s\n", getlockMacID());
		// }
		if(cJSON_GetObjectItem(root, "duid") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "duid")->valuestring;
	    	setDeviceUUID(tmp);
		    ESP_LOGI(TAG, "duid:%s", getDeviceUUID());
		}
		if(cJSON_GetObjectItem(root, "guid") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "guid")->valuestring;
	    	setGWUUID(tmp);
		    ESP_LOGI(TAG, "guid:%s", getGWUUID());
		}		
		if(cJSON_GetObjectItem(root, "euid") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "euid")->valuestring;
	    	setEntityUUID(tmp);
		    ESP_LOGI(TAG, "euid:%s", getEntityUUID());
		}
		if(cJSON_GetObjectItem(root, "spid") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "spid")->valuestring;
	    	setSpid(tmp);
		    ESP_LOGI(TAG, "spid:%s", getSpid());
		}
		if(cJSON_GetObjectItem(root, "uuid") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "uuid")->valuestring;
	    	setUnitUUID(tmp);
		    ESP_LOGI(TAG, "uuid:%s", getUnitUUID());
		}
		if(cJSON_GetObjectItem(root, "murl") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "murl")->valuestring;
	    	setmqttURL(tmp);
		    ESP_LOGI(TAG, "murl:%s", getmqttURL());
		}
		if(cJSON_GetObjectItem(root, "hurl") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "hurl")->valuestring;
	    	sethttpURL(tmp);
		    ESP_LOGI(TAG, "hurl:%s", gethttpURL());
		}
		if(parse_access && cJSON_GetObjectItem(root, "atok") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "atok")->valuestring;
	    	setAccessToken(tmp);
		    ESP_LOGI(TAG, "atok:%s", getAccessToken());
		}
		if(cJSON_GetObjectItem(root, "rtok") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "rtok")->valuestring;
	    	setRefreshToken(tmp);
		    ESP_LOGI(TAG, "rtok:%s", getRefreshToken());
		}
update_wifi:
		if(cJSON_GetObjectItem(root, "s") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "s")->valuestring;
		    setSsid(tmp);
		    ESP_LOGI(TAG, "s:%s", getSsid());
		}
		if(cJSON_GetObjectItem(root, "p") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "p")->valuestring;
	    	setPwd(tmp);
		    ESP_LOGI(TAG, "p:%s", getPwd());
		}		
		if(cJSON_GetObjectItem(root, "sk") != NULL){//if key not present then NULL is returned
	    	char *tmp = cJSON_GetObjectItem(root, "sk")->valuestring;
	    	AESECB128_NOPADDING(tmp,plainTex,"NdRgUkXp2s5vByAD");
	    	setSecurityKey(plainTex);	    	
		    ESP_LOGI(TAG, "Security key:%s", getSecurityKey());
		}
		if(cJSON_GetObjectItem(root, "rc") != NULL){//if key not present then NULL is returned
	    	int tmp = cJSON_GetObjectItem(root, "rc")->valueint;
	    	ESP_LOGI(TAG, "rc:%d", tmp);	    	
	    	setRecommisionStatus(tmp);	    	
		    ESP_LOGI(TAG, "rc:%d", getRecommisionStatus());
		}
		
}//commission_action_end		
hubconfexit:
	    // free the memory!
	    cJSON_Delete(root);
	    setJsonParseStatus(1);
	} else {
 		ESP_LOGI(TAG, "json root is NULL");
 		setJsonParseStatus(2);//json parse fails
	}

 	ESP_LOGI(TAG, "parseConf Exit");
 	return getAction();
}
bool validateJson(char* jsonbuff){

	cJSON_Hooks memoryHook;

	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);
	bool status = false;
 	ESP_LOGI(TAG, " Validate Json  %s",jsonbuff);
    cJSON *root = cJSON_Parse(jsonbuff);
    if(root != NULL){
    	cJSON_Delete(root);
    	status = true;
	} else {		
 		status = false;
	}

 	ESP_LOGI(TAG, "validateJson[%d]",status);
 	return status;
}

// int parseSettings(char* jsonbuff){

// 	cJSON_Hooks memoryHook;

// 	memoryHook.malloc_fn = malloc;
// 	memoryHook.free_fn = free;
// 	cJSON_InitHooks(&memoryHook);

//  	printf("\n parseSettings Entry\n");
//     cJSON *root = cJSON_Parse(jsonbuff);
//     if(root != NULL){

// 	    if(cJSON_GetObjectItem(root, "lastKnowStatus") != NULL){//if key not present then NULL is returned
// 	    	int tmp = 0;
// 	    	tmp = cJSON_GetObjectItem(root, "lastKnowStatus")->valueint;
// 	    	setLastKnownProperty(tmp);
// 		    printf("\n setLastKnownStatus: %d\n", getLastKnownProperty());
// 			}
// 			if(cJSON_GetObjectItem(root, "customColor") != NULL){//if key not present then NULL is returned
// 	    	char *color = cJSON_GetObjectItem(root, "customColor")->valuestring;
// 	    	memset(customColor,0,sizeof(customColor));
// 	    	strcpy(customColor, color);
// 		    printf("\n setLastKnownStatus: %s\n", customColor);
// 			}

// 	    cJSON_Delete(root);
// 	    setJsonParseStatus(1);
// 	} else {
//  		printf("\njson root is NULL\n");
//  		setJsonParseStatus(2);//json parse fails
// 	}

//  	printf("\nparseConf Exit\n");
//  	return 0;
// }

void parseResponseJson(char* jsonbuff, enum rest_call_type rtype){

	cJSON_Hooks memoryHook;

	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);

 	ESP_LOGI(TAG, "parseResponseJson Entry");
    cJSON *root = cJSON_Parse(jsonbuff);

    if(root != NULL){
    	if(rtype == ACCESSTOKEN){
			if(cJSON_GetObjectItem(root, "accessToken") != NULL){//if key not present then NULL is returned
		    	char *tmp = cJSON_GetObjectItem(root, "atok")->valuestring;
		    	setAccessToken(tmp);
		    	setAccessTokenValidity(1);
			    ESP_LOGI(TAG, " accessToken:%s", getAccessToken());
			}
		}
		else if(rtype == INVENTORY){
			if(cJSON_GetObjectItem(root, "state") != NULL){//if key not present then NULL is returned
		    	int tmp = cJSON_GetObjectItem(root, "state")->valueint;
		    	if(tmp == 3) {// not commissioned state == 3
        			ESP_LOGI(TAG,"\r\n Not commissioned state = 3\r\n");
        			//factory Reset Here format partition
      				 factoryReset();
    					}
		    	setCommissionState(tmp);
			    ESP_LOGI(TAG, "commission state:%d", getCommissionState());
			}
			if(cJSON_GetObjectItem(root, "swVersion") != NULL){//if key not present then NULL is returned
		    	char *tmp = cJSON_GetObjectItem(root, "swVersion")->valuestring;
				vTaskDelay(1000 / portTICK_PERIOD_MS);
		    	if(strcmp(tmp,getCurrentSWVersion()) != 0){
		    		setinventoryUpdateReq(true);
		    	}

			    ESP_LOGI(TAG, "swVersion:%s", tmp);
			}
		}else if(rtype == GETSCHEDULES){
			ESP_LOGI(TAG, "Call Parsing Rest Json");
			parseRestScheduleJson(jsonbuff);

		}
		else if(rtype == GET_OTA_URL){
			if(cJSON_GetObjectItem(root, "otaUrl") != NULL && !cJSON_IsNull(cJSON_GetObjectItem(root, "otaUrl"))){//if key not present then NULL is returned
		    	char * otaUrl = cJSON_GetObjectItem(root, "otaUrl")->valuestring;
			    ESP_LOGI(TAG, "OTA URL: %s", otaUrl);
			    // esp_mqtt_client_stop(getMQTTClient());// If ram not available then stop mqtt
    			// esp_mqtt_client_destroy(getMQTTClient());
#ifdef ENABLE_OTA
			    startOTA(otaUrl);
#endif
			}

		}
		else if(rtype == GET_OTA_URLV2){
			ESP_LOGI(TAG, "Parsing OTA URL");
			if(cJSON_GetObjectItem(root, "otaUrl") != NULL && !cJSON_IsNull(cJSON_GetObjectItem(root, "otaUrl")))
				{
		    	char * otaUrl = cJSON_GetObjectItem(root, "otaUrl")->valuestring;
			    ESP_LOGI(TAG, "OTA URL: %s", otaUrl);
			    // esp_mqtt_client_stop(getMQTTClient());
    			// esp_mqtt_client_destroy(getMQTTClient());
#ifdef ENABLE_OTA
			    startOTA(otaUrl);
#endif
			}

		}
	    // free the memory!
	    cJSON_Delete(root);
	    setJsonParseStatus(1);
	} else {
 		ESP_LOGI(TAG, "parseResponseJson root is NULL");
 		setJsonParseStatus(2);//json parse fails
	}

 	ESP_LOGI(TAG, "parseResponseJson Exit");

}

char json_buf_html[1024];
char response_code[4] = {0};
char *getRestResponseCode(){
	return response_code;
}
char* jsonFromHtmlResponse(char *response, enum rest_call_type rtype){

	char *code = strchr(response,' ');//1st space is just before response code
	if(!code) return NULL;
	code++;//skipping space
	response_code[0] = code[0];
	response_code[1] = code[1];
	response_code[2] = code[2];
	response_code[3] = '\0';

	ESP_LOGI(TAG, "response :: %s | rest type -[%d]",response,rtype);// 200
	ESP_LOGI(TAG, "response code :: %s",response_code);// 200
	if(response_code[0] == '2' && (strcmp(response_code,"200")==0) && rtype == UPDATEVERSION){
		ESP_LOGI(TAG, "########################################UPDATEVERSION is successfully Set:");
		writeNVData("svVersion",getCurrentSWVersion());
	}

	char *start = strchr(response,'{');
	char *end  = strrchr(response,'}');
	if(start == NULL || end == NULL){
		start = strchr(response,'[');
		end  = strrchr(response,']');
		return NULL;
	}
	if(start == NULL || end == NULL){
		return NULL;
	}
	//ESP_LOGI(TAG, "%s",start);
	memset (json_buf_html, 0, sizeof(json_buf_html));

	int json_len = end - start;

	for(int i=0;i<=json_len;i++){
		json_buf_html[i] = start[i];
	}
	if(response_code[0] == '2' && (strcmp(response_code,"200")==0)){//ok valid response code 2XX series
		ESP_LOGI(TAG, "[ %s ]",json_buf_html);
		parseResponseJson(json_buf_html, rtype);
	}else{
		ESP_LOGI(TAG, "[ Invalid response with code - %s]",response_code);
		ESP_LOGI(TAG, "[ %s ]",json_buf_html);
		//{"code":"ERR_AUTH_002","message":["User Token expired, Access denied"]}
		// if token expired
		if(strstr(json_buf_html,"ERR_AUTH_002")){
			ESP_LOGI(TAG, "[ ERR_AUTH_002 .. user token expired... ]");//fetch access token
			setAccessTokenValidity(0);
		}
		if(strstr(json_buf_html,"ERR_AUTH_001")){// decommission
			ESP_LOGI(TAG, "[ ERR_AUTH_001 .. not commissioned invalid user ... ]");
			// erase_data_from_flash();
			// vTaskDelay(1000);
			// sys_reset();
			setAccessTokenValidity(0);
		}
		if(strstr(json_buf_html,"ERR_SM_005")){// reset user not exist
			ESP_LOGI(TAG, "[ ERR_AUTH_005 .. not commissioned invalid user Factory Reset ... ]");
			//erase_data_from_flash();
		}

		//{"code":"ERR_SM_005","message":["Invalid Refresh Token"]}//Not valid case as refresh token valid untill decommissioned
	}
	//free if allocated - *response
	return response_code;
}

//void fadeLedColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white);
//void set_led_color(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white);
//void setFastMode(bool state);

void MQTTPublishMsg(char *servName, char *attrName,char *state);

static void commissionStatustask(void *pvParameters)
{
	ESP_LOGI(TAG, "Executing Task commissionStatus_task\r");
	esp_mqtt_client_stop(getMQTTClient());
    esp_mqtt_client_destroy(getMQTTClient());
	vTaskDelay(2000/portTICK_PERIOD_MS);


     rest_call(INVENTORY);
                ESP_LOGI(TAG,"INVENTORY Rest Executed");
                if(!getAccessTokenValidity()){// if access token invalid ...
                    rest_call(ACCESSTOKEN);//fetch access always and try
                    vTaskDelay(5000 /portTICK_PERIOD_MS);
                    rest_call(INVENTORY);
                    }
    vTaskDelete(NULL);

}

void executecommissionStatustask()
{
    xTaskCreate(commissionStatustask, "commissionStatustask", 8192, NULL, 1, NULL);
}

static void mqttParse_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Executing Task mqttParse_task\r");
    ESP_LOGI(TAG, " JSON C MQTT Parse Task ------------%s",(char*)pvParameters);
 	parseMqttJson((char*)pvParameters);
    ESP_LOGI(TAG, " mqttParse_task Exit");
   vTaskDelete(mqttParse_task_hdl);
   mqttParse_task_hdl = NULL;
}

void executeMqttParseTask(char *mqttData)
{
    xTaskCreate(mqttParse_task, "mqttParse_task", 4096/*2048*/, (void*)mqttData, 1, mqttParse_task_hdl);
}

void removeMqttParseTask()
{
    if(mqttParse_task_hdl != NULL)
    {
        ESP_LOGI(TAG, "Deleting Task");
   vTaskDelete(mqttParse_task_hdl);
   mqttParse_task_hdl = NULL;
    }
    else
    {
        ESP_LOGI(TAG, "Not Deleting mqttParse_task_hdl coz its null");
    }

}

// static void mqttpublish_task(void *pvParameters)
// {
//     printf("\r\n Executing Task FOr MqTT Publish Task\r\n");
//  	while(1)
//  	{
//  		vTaskDelay(500 / portTICK_PERIOD_MS);
//  		if(publishCounter == 0)
//  		{
//  			if(strlen(colorRGBAttribute) != 0)
//  			{
//  				char *serviceName = "colorRGBControl";
//  				char *color = "color";
//  				printf("\r\n colorRGBControl MqTT Publish Task  -- %s \r\n",colorRGBAttribute);
//  				MQTTPublishMsg(serviceName, color,colorRGBAttribute);
//  				vTaskDelay(500 / portTICK_PERIOD_MS);
//  				memset(colorRGBAttribute, 0, sizeof(colorRGBAttribute));
//  			}
//  			if(strlen(colorWarmthAttribute) != 0)
//  			{
//  				 printf("\r\n colorRGBControl MqTT Publish Task -- %s \r\n",colorWarmthAttribute);
//  				MQTTPublishMsg("colorWarmthControl", "color",colorWarmthAttribute);
//  				vTaskDelay(500 / portTICK_PERIOD_MS);
//  				memset(colorWarmthAttribute, 0, sizeof(colorWarmthAttribute));
//  			}
//  			if(strlen(rebootColor) !=0)
//  			{
//  				if(getLastKnownProperty() == 1)
//  				{
//  					char *temp = "%d,%d,%s";
//  					char nvData[128]={0};
//     				memset(nvData,0,sizeof(nvData));
//     				sprintf(nvData, temp,getDndStatus(),getLastKnownProperty(),rebootColor);
//     				printf("Setting NV Data for reboot Color JSOnParse");
//     				setLastKnownStatus(nvData);
//  				}
//  			}
//  		}
//  		publishCounter--;
//  	}

//    vTaskDelete(NULL);
//    mqttParse_task_hdl = NULL;
// }

// void executeMqttpublishTask()
// {
//     xTaskCreate(mqttpublish_task, "mqttpublish_task", 2048, NULL, 5, NULL);
// }
void startOffAfterTask(int time);
void parseMqttJson(char *jsonbuff){
	ESP_LOGI(TAG, "-----------------------Free heap size: %d", esp_get_free_heap_size());
	// reboot or CheckOTAAvailable() after receiving upgradeAvailable in mqtt topic
	// check reset in mqtt topic and do factory reset

	cJSON_Hooks memoryHook;
	//ESP_LOGI(TAG, "parseMqttJson:: %s",jsonbuff);
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);
 	ESP_LOGI(TAG, "parseMqttJson Entry");
    cJSON *root = cJSON_Parse(jsonbuff);
    if(root != NULL){
    	cJSON *command = NULL;
    	cJSON *devices = NULL;
    	cJSON *services = NULL;
    	cJSON *serviceName = NULL;
    	cJSON *colorserviceName = NULL;
    	cJSON *colorwarmName = NULL;
    	cJSON *sceneControl = NULL;
    	cJSON *lastKnownStatusService = NULL;
    	cJSON *timeZoneSetting = NULL;
    	cJSON *otaURL = NULL;
    	cJSON *autoOff = NULL;
    	cJSON *attributeName = NULL;
	cJSON *action = NULL;
    	cJSON *parameters = NULL;
    	command = cJSON_GetObjectItem(root, "command");
    	if(command != NULL){
    		devices = cJSON_GetObjectItem(command, "devices");
    	}
    	if(devices != NULL){
    		//lock deviceUUID get and use to fetch mpin pass1 macid
    		if(cJSON_GetObjectItem(devices, "deviceUUID") != NULL){
    			char *tmp = cJSON_GetObjectItem(devices,"deviceUUID")->valuestring;
    			ESP_LOGI(TAG, "lduid : %s",tmp);
    		}
    		services = cJSON_GetObjectItem(devices, "services");
    	}else{
    		ESP_LOGI(TAG, "==>>devices is NULL:");
    	}
    	if(services != NULL){
    		serviceName = cJSON_GetObjectItem(services, "lcSwitchControl");
    		// colorserviceName = cJSON_GetObjectItem(services, "colorRGBControl");
    		// colorwarmName = cJSON_GetObjectItem(services, "colorWarmthControl");
    		// sceneControl  = cJSON_GetObjectItem(services, "sceneControl");
    		lastKnownStatusService  = cJSON_GetObjectItem(services, "lastKnownStatus");
    		timeZoneSetting  = cJSON_GetObjectItem(services, "timeZoneSetting");
    		otaURL = cJSON_GetObjectItem(services, "lcOtaService");
    		autoOff = cJSON_GetObjectItem(services, "autoOffAfter");

    		ESP_LOGI(TAG, "==>>services parsed:");
    	}
    	else{
    		ESP_LOGI(TAG, "==>>services is NULL:");
    	}
    	if(serviceName != NULL){
    		attributeName = cJSON_GetObjectItem(serviceName, "attributes");
    		ESP_LOGI(TAG, "==>>services is lcSwitchControl:");
    	}
    	// else if(colorserviceName != NULL)
    	// {
    	// 	attributeName = cJSON_GetObjectItem(colorserviceName, "attributes");
    	// 	printf("\n\n==>>services is colorRGBControl:\n\n");
    	// }
    	// else if(colorwarmName != NULL)
    	// {
    	// 	attributeName = cJSON_GetObjectItem(colorwarmName, "attributes");
    	// 	printf("\n\n==>>services is colorWarmthControl:\n\n");
    	// }
    	// else if(sceneControl != NULL)
    	// {
    	// 	attributeName = cJSON_GetObjectItem(sceneControl, "commands");
    	// 	printf("\n\n==>>services is sceneControl:\n\n");

    	// }
    	else if(lastKnownStatusService != NULL)
    	{
    		attributeName = cJSON_GetObjectItem(lastKnownStatusService, "attributes");
    		ESP_LOGI(TAG, "==>>services is lastKnownStatus:");

    	}
    	else if(timeZoneSetting != NULL)
    	{
    		attributeName = cJSON_GetObjectItem(timeZoneSetting, "attributes");
    		ESP_LOGI(TAG, "==>>services is timeZoneSetting:");

    	}
    	else if(otaURL != NULL)
    	{
    		attributeName = cJSON_GetObjectItem(otaURL, "attributes");
    		ESP_LOGI(TAG, "==>>services is LC OTA SERVICE otaURL:");

    	}
    	else if(autoOff != NULL)
    	{
    		attributeName = cJSON_GetObjectItem(autoOff, "attributes");
    		ESP_LOGI(TAG, "==>>services is autoOff");

    	}
    	else{
    		ESP_LOGI(TAG, "==>>lockControl is NULL:");
    	}
		if(attributeName != NULL && cJSON_GetObjectItem(attributeName, "power") != NULL){//if key not present then NULL is returned
	    	char *name = cJSON_GetObjectItem(attributeName, "power")->valuestring;
		    printf("\n\n==>>control:%s\n\n", name);
		    if(strcmp(name,"on") == 0){
				//turnOnPlug(1);
		    }else if(strcmp(name,"off") == 0){
				//turnOffPlug(1);
		    }else{
    			printf("\n\n==>>s cmp fails\n\n");
    		}
		}

		else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "offSet") != NULL)
		{
			char *name = cJSON_GetObjectItem(attributeName, "offSet")->valuestring;
			ESP_LOGI(TAG, " TimeZone offSet ...%s ",name);
			setTimeZone(name);
			writeNVData("tzVal",name);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			MQTTPublishMsg("timeZoneSetting", "offSet",name);
			ESP_LOGI(TAG, "###########Message Published");
		}
		else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "offAfter") != NULL)
		{

			char *name = cJSON_GetObjectItem(attributeName, "offAfter")->valuestring;
			ESP_LOGI(TAG, " #### OffAfter time ...%s ",name);
			if(strlen(name)	 > 0 ){
				writeNVData("offAfter",name);
				startOffAfterTask(atoi(name));//
			}
		}
		// else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "color") != NULL)
		// 	{
		// 		char *name = cJSON_GetObjectItem(attributeName, "color")->valuestring;
		// 		printf("\n\n Color RGB Control ...%s \n\n",name);
		// 		char charrgb[30] = {};
		// 		memset(charrgb, 0 ,sizeof(charrgb));
		// 		strcpy(charrgb, name);
		// 		uint16_t grays[5] = {0,0,0,0,0};
  //   			char *token = strtok(name, ",");
  //   			int i = 0;

  //  				while( token != NULL ) {
  //     				printf( " %s\n", token );
  //     				grays[i] = atoi(token);
  //     				token = strtok(NULL, ",");
  //     				i++;
  //  					}

  //  				if(grays[3] > 0 || grays[4] > 0)
  //  				{
  //  					fadeLedColor(0,0,0,grays[3],grays[4]);
  //  					strcpy(colorWarmthAttribute,charrgb);
  //  					printf("Keep Value for Next Boot Also -- %s -- %s",colorWarmthAttribute, charrgb);
  //  				}
  //  				else
  //  				{
  //  					fadeLedColor(grays[2],grays[1],grays[0],0,0);
  //  					strcpy(colorRGBAttribute, charrgb);
  //  					printf("Keep Value for Next Boot Also -- %s -- %s",colorRGBAttribute, charrgb);
  //  				}
  //  				publishCounter = 5;
  //  				printf("Keep Value for Next Boot Also");
  //  				memset(rebootColor, 0 ,sizeof(rebootColor));
  //  				strcpy(rebootColor,charrgb);
  //  				//MQTTPublishMsg("colorWarmthControl", "colorWarmth",charrgb);
		//     	printf("\n\n Color RGB Control After ...   %s \n\n",charrgb);


		// 	}
		// 	else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "colorWarmth") != NULL)
		// 	{
		// 		char *name = cJSON_GetObjectItem(attributeName, "colorWarmth")->valuestring;
		// 		printf("\n\n Color CW Control ...%s \n\n",name);
		// 		char charrgb[24] = {};
		// 		memset(charrgb, 0 ,sizeof(charrgb));
		// 		strcpy(charrgb, name);
		// 		uint16_t grays[5] = {0,0,0,0,0};
  //   			char *token = strtok(name, ",");
  //   			int i = 0;

  //  				while( token != NULL ) {
  //     				printf( " %s\n", token );
  //     				grays[i] = atoi(token);
  //     				token = strtok(NULL, ",");
  //     				i++;
  //  					}

		//     	fadeLedColor(grays[2],grays[1],grays[0],grays[3],grays[4]);
		//     	//vTaskDelay(300 / portTICK_PERIOD_MS);
		//     	//MQTTPublishMsg("colorWarmthControl", "colorWarmth",charrgb);
		// 	}
			// else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "mode") != NULL)
			// {
			// 	char *name = cJSON_GetObjectItem(attributeName, "mode")->valuestring;
			// 	printf("\n\n Last Known Mode ...%s \n\n",name);
			// 	char charrgb[24] = {};
			// 	memset(charrgb, 0 ,sizeof(charrgb));
			// 	strcpy(charrgb, name);
			// 	if(strcmp(name, "default") == 0)
			// 	{
			// 		setLastKnownProperty(0);
			// 	}
			// 	if(strcmp(name, "mostRecent") == 0)
			// 	{
			// 		setLastKnownProperty(1);
			// 	}
			// 	if(strcmp(name, "custom") == 0)
			// 	{
			// 		setLastKnownProperty(2);
			// 	}
			// 	//saveSettings();
			// 	vTaskDelay(1000 / portTICK_PERIOD_MS);
		    // 	MQTTPublishMsg("lastKnownStatus", "mode",name);

			// }
			// else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "dndSetting") != NULL)
			// {
			// 	char *name = cJSON_GetObjectItem(attributeName, "dndSetting")->valuestring;
			// 	printf("\n\n Last Known DnDSettings ...%s \n\n",name);
			// 	if(strcmp(name, "true") == 0)
			// 	{
			// 		setDndProperty(true);
			// 	}
			// 	else
			// 	{
			// 		setDndProperty(false);
			// 	}
			// 	vTaskDelay(1000 / portTICK_PERIOD_MS);
			// 	MQTTPublishMsg("lastKnownStatus", "dndSetting",name);
			// }
			// else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "customParams") != NULL)
			// {
			// 	char *name = cJSON_GetObjectItem(attributeName, "customParams")->valuestring;
			// 	printf("\n\n Last Known customParams ...%s \n\n",name);
			// 	memset(customColor,0,sizeof(customColor));
	  //   		strcpy(customColor, name);
	  //   		saveSettings();
	  //   		vTaskDelay(1000 / portTICK_PERIOD_MS);
			// 	MQTTPublishMsg("lastKnownStatus", "customParams",name);
			// }
    		else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "otaURL") != NULL)
			{
				char *name = cJSON_GetObjectItem(attributeName, "otaURL")->valuestring;
				ESP_LOGI(TAG, " OTA URL For Processing ...%s ",name);
				// printf("\no t a Processing state -> %s\n",name);
	    		vTaskDelay(1000 / portTICK_PERIOD_MS);
	    		MQTTPublishMsg("upgradeStatus","state","upgrading");
	    		// esp_mqtt_client_stop(getMQTTClient());
    			// esp_mqtt_client_destroy(getMQTTClient());
#ifdef ENABLE_OTA
			    startOTA(name);
#endif
				//cJSON_Delete(root);
			}
			else if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "state") != NULL)
			{
				char *name = cJSON_GetObjectItem(attributeName, "state")->valuestring;
				if(strcmp(name,"forced") == 0)
				{
					ESP_LOGI(TAG, " OTA URL For Processing ...%s ",name);
					// printf("\nstate forced OTA URL For Processing state -> %s\n",name);
		    		vTaskDelay(1000 / portTICK_PERIOD_MS);
				    startRestCallTask();
				}
				//cJSON_Delete(root);
			}
			// else
			// {
			// 	cJSON_Delete(root);
			// }

		// free the memory!
	    cJSON_Delete(root);
	    setJsonParseStatus(1);
	} else {
 		ESP_LOGI(TAG, "parseResponseJson root is NULL");
 		setJsonParseStatus(2);//json parse fails
	}
	ESP_LOGI(TAG, "----------------------Free heap size: %d", esp_get_free_heap_size());
 	ESP_LOGI(TAG, "parseMqttJson Exit");
}

// void setDndProperty(bool value){
//     FILE* f;
//     struct stat st;

//     	if(stat("/spiffs/dndProperty.txt", &st) == 0){
//     		if(!value)
//     		{
//     			printf("\nRemoving DND File from Service\n");
//     			unlink("/spiffs/dndProperty.txt");
//     		}
//     		else
//     		{
//     		printf("\nDND File Exists Do noting\n");
//     		}
//     	}
//     	else
//     	{
//     		if(!value)
//     		{
//     			printf("\nDND File doesnt Exists Do noting--------- \n");
//     		}
//     		else
//     		{
//     			f = fopen("/spiffs/dndProperty.txt", "w");
//     			if (f == NULL) {
//         		printf("Failed to open reset file for writing");
//     			} else
//     			{
//         			printf("\n dndProperty.txt open success \n");
//         			fclose(f);
//         			f = NULL;
//     			}
//     		}
//     	}
// }
void meteringMqttUpdate(int viaBle);
void parseBleControlCommand(char *jsonbuff){
	printf("\nparseBleControlCommand ... \n");
	cJSON_Hooks memoryHook;
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);
 	ESP_LOGI(TAG, "parseBleControlCommand Entry  -  %s",jsonbuff);
    cJSON *root = cJSON_Parse(jsonbuff);
    if(root != NULL){
    	ESP_LOGI(TAG, "Root is not Null ");
    	if(cJSON_GetObjectItem(root, "data") != NULL)
    	{
	    	char * item = cJSON_GetObjectItem(root, "data")->valuestring;
	  		ESP_LOGI(TAG, "Json Parsed from BLE JSON");
			AESECB128_NOPADDING(item,plainTex,getSecurityKey());
			if(strstr(plainTex,"timerControl"))
	    	{
	        	removeTimerTask();
	        	ESP_LOGI(TAG,"\r\n Timer Control Task Create ... \r\n");
	        	executeTimerTask(plainTex);
	    	}
	    	else if(strstr(plainTex,"reboot"))
	    	{
	        	ESP_LOGI(TAG,"\r\n Rebooting Device ... \r\n");
	        	esp_restart();
			}
	    	else if(strstr(plainTex,"factoryReset"))
	    	{           	        
	        	ESP_LOGI(TAG,"\r\n factoryReset Device ... \r\n");            
	        	factoryReset();
			}
			else if(strstr(plainTex,"meteringRefresh")){
	            ESP_LOGI(TAG,"\r\n updating metering data ... \r\n");
	            printf("\r\n u... m data ... \r\n");
	            meteringMqttUpdate(1);// via ble - 1
	        }
	    	else
	    	{
				parseMqttJson(plainTex);
			}
	  	}
		else if(cJSON_GetObjectItem(root, "a") != NULL)
		{
			int tmp = cJSON_GetObjectItem(root, "a")->valueint;
			// ESP_LOGI(TAG, "Action Value  -  %d",tmp);
			printf("\nAction Value  -  %d\n",tmp);
			if(tmp == 2)
			{
				char * ssid = cJSON_GetObjectItem(root, "s")->valuestring;
				char * password = cJSON_GetObjectItem(root, "p")->valuestring;
				cJSON *duidT = cJSON_GetObjectItem(root, "duid");
				if(duidT != NULL)
				{
					if(strcmp(cJSON_GetObjectItem(root, "duid")->valuestring,getDeviceUUID()) == 0)
					{
						// ESP_LOGI(TAG, " SSID on BLE JSON ###########   %s",ssid);
						// ESP_LOGI(TAG, " PASS on BLE JSON ###########   %s",password);
						// printf("\n SSID on BLE JSON ###########   %s",ssid);
						// printf("\n PASS on BLE JSON ###########   %s",password);
						example_wifi_sta_do_disconnect();
						vTaskDelay(500/portTICK_PERIOD_MS);
						connectWifiCommision(ssid,password);

						if(strcmp(getCurrentStatus(),INTERNET_CONNECTED) != 0)
						{

							//Error need to reset the Previous Wifi
							// writeNVData("ssid", "");
							// writeNVData("pass", "");
							example_wifi_sta_do_disconnect();
							vTaskDelay(500/portTICK_PERIOD_MS);
							connectWifi();
						}
						if(strcmp(getCurrentStatus(),INTERNET_CONNECTED) == 0)
						{

							writeNVData("ssid", ssid);
							writeNVData("pass", password);
							esp_restart();
						}
					}
				}
				else
				{
					// ESP_LOGI(TAG, " Device Doesnt Match So not updating Wifi   %s",ssid);
					printf("\nDevice Doesnt Match So not updating Wifi   %s\n",ssid);
				}
			}
		}

	    cJSON_Delete(root);
	    setJsonParseStatus(1);
	}
	else {
		ESP_LOGI(TAG, "parseResponseJson root is NULL");
		setCurrentStatus(CONFIG_PARSING_ERROR);
		setJsonParseStatus(2);//json parse fails
	}
 	ESP_LOGI(TAG, "parseBleControlCommand Exit");
}

void parseConfigJson(char *jsonbuff){

	cJSON_Hooks memoryHook;
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);
 	ESP_LOGI(TAG, "parseConfigJson Entry");
    cJSON *root = cJSON_Parse(jsonbuff);
    if(root != NULL){
    	ESP_LOGI(TAG, "Root is not Null ");
    	cJSON *item = cJSON_GetObjectItem(root, "message");
    	ESP_LOGI(TAG, "item values %d", cJSON_GetArraySize(item));

  		for (int i = 0 ; i < cJSON_GetArraySize(item) ; i++)
  			{
     			cJSON * subitem = cJSON_GetArrayItem(item, i);
     			int actionVal = cJSON_GetObjectItem(subitem, "action")->valueint;
     			ESP_LOGI(TAG, "--------action Val ---   %d", actionVal);
			    if(actionVal == 115)
     				{
     				ESP_LOGI(TAG, "--------action Val matched ---   %d", actionVal);
     				cJSON *metadata = cJSON_GetObjectItem(subitem, "metadata");
     				char *deviceUUID = cJSON_GetObjectItem(metadata, "deviceUUID")->valuestring;
     				char *gatewayUUID = cJSON_GetObjectItem(metadata, "device_handleName")->valuestring;
     				char *unitUUID = cJSON_GetObjectItem(metadata, "unitUUID")->valuestring;

     				ESP_LOGI(TAG, " *************device  Uuid %s  --  %s :",deviceUUID,getDeviceUUID());
     				ESP_LOGI(TAG, " *************gateway Uuid %s  --  %s :",gatewayUUID,getGWUUID());
     				ESP_LOGI(TAG, " *************unit    Uuid %s  --  %s :",unitUUID,getUnitUUID());

			     	if(strcmp(deviceUUID,getDeviceUUID()) == 0 && strcmp(gatewayUUID,getGWUUID()) == 0 && strcmp(unitUUID,getUnitUUID()) == 0)
     					{
     						ESP_LOGI(TAG, " *************Put Device DeCommision Code Here :");
     						factoryReset();
     					}

     				}

			}
		// free the memory!
	    cJSON_Delete(root);
	    setJsonParseStatus(1);
				}
				else {
 					ESP_LOGI(TAG, "parseResponseJson root is NULL");
 					setJsonParseStatus(2);//json parse fails
					}
 	ESP_LOGI(TAG, "parseConfigJson Exit");
}


#define MAX_SCHEDULES 10//20
//#define CURR_ACTION_VAL 256
//duplicate remove later;
// typedef struct schedule_tpoint_s{
// 	int id;
// 	int time;//in mins
// 	int action;//0-ON,1-OFF,2-Scene,3-delete
// 	char action_val;
// 	//char action_val[CURR_ACTION_VAL];
// }schedule_tpoint_t;

// typedef struct schedule_s{
// 	int s_time;// use for sorting in mins = hh*60+mm  from 00 to 23 midnight 24 hrs format00-59 mins
// 	//duplicate remove later
// 	int s_action;//0-ON,1-OFF,2-Scene,3-delete
// 	int e_action;//0-ON,1-OFF,2-Scene,3-delete
// 	int duration;//minutes
// 	int state;//0-disable,1-enable, 2 - delete
// 	char s_action_val;//colors or scene json
// 	char e_action_val;
// 	//char s_action_val[CURR_ACTION_VAL];//colors or scene json
// 	//char e_action_val[CURR_ACTION_VAL];
// 	//char uuid[37];
// 	char days[8];
// }schedule_t;

// typedef struct sidx_s{
// 	int id;
// 	char uuid[37];
// 	schedule_t schedules;
// }sidx_t;
//max 20 as of now
//65kb/scene - 1300 - 1.3Kb

//schedule_t schedules[MAX_SCHEDULES]={0};
sidx_t idx[MAX_SCHEDULES] = {0};
schedule_tpoint_t idx_action[2*MAX_SCHEDULES] = {0};



int available_schedule = 0;
char sch_file_name[1024];
//bubble sort as only max 20 schedules are there
void sortSchedule_s()
{
    int i, j;
    sidx_t temp;

    for (i = 0; i < MAX_SCHEDULES; i++)
    {
        for (j = 0; j < (MAX_SCHEDULES - 1 - i); j++)
        {
            if (idx[j].schedules.s_time > idx[j+1].schedules.s_time)
            {
                temp = idx[j];
                idx[j] = idx[j+1];
                idx[j + 1] = temp;
            }
        }
    }
}

void sortSchedule()
{
    int i, j;
    schedule_tpoint_t temp;

    for (i = 0; i < 2*MAX_SCHEDULES; i++)
    {
        for (j = 0; j < (2*MAX_SCHEDULES - 1 - i); j++)
        {
            if (idx_action[j].time > idx_action[j+1].time)
            {
                temp = idx_action[j];
                idx_action[j] = idx_action[j+1];
                idx_action[j + 1] = temp;
            }
        }
    }
}

volatile int curr_id = -1;
volatile int curr_action = -1;
//char curr_act_val[CURR_ACTION_VAL] = {0};
// void setClosestActionVal(char *val){
// 	memset(curr_act_val, 0, sizeof(curr_act_val));
// 	strcpy(curr_act_val, val);
// }
void setClosestAction(int act){
	curr_action = act;
}

void setClosestActionID(int id){
	curr_id = id;
}
//sachin
void printActionSchedule();
int getClosestActionTime(int curr_min){// greater than current time
	int i = 0;
	for(; i < 2* MAX_SCHEDULES; i++){
		if( idx_action[i].id != -1 && idx_action[i].time >= curr_min ){
			break;
		}
	}
	// printf("\n1111 getClosestTime idx : %d\n",i);
	if (i >= 2*MAX_SCHEDULES){
		//no schedule before night 12:00 PM
		//so return 1st schedule of next day
		//printf("\n2222 getClosestTime idx : %d\n",i);

		i = 0;
		for(; i < 2* MAX_SCHEDULES; i++){
			if( idx_action[i].id != -1 && idx_action[i].time >= 0 ){
				break;
			}
		}
		//printf("\n3333 getClosestTime idx : %d\n",i);
		//printActionSchedule();
		if (i >= 2*MAX_SCHEDULES){
			return -1;			
		}
	}
	printf("\ngetClst : %d\n",idx_action[i].time);
	setClosestAction(idx_action[i].action);
	//setClosestActionVal(idx_action[i].action_val);
	setClosestActionID(idx_action[i].id);

	return idx_action[i].time;
}

schedule_t getCurrentSchedule(int id){
	int i = 1;
	for(;i <= MAX_SCHEDULES; i++){
		if(idx[i-1].id == id){
			break;
		}
	}
	//printf("\ncurr Sched idx : %d\n",i);
	return idx[i-1].schedules;
}

int getClosestActionID(){

	return curr_id;
}
int getClosestAction(){

	return curr_action;
}

int isScheduleValidToday(int id, int day_index, bool isEndTime,int curr_time){
	int isValid = -1;
	printf("\n id - %d , wk_day - %d isEndTm - %d.\n",id,day_index,isEndTime);
	if(id == -1) return isValid;
	for(int i = 1;i <= MAX_SCHEDULES; i++){
		if(idx[i-1].id == id){
			//printf("\n=> weekday - %s\n", idx[i-1].schedules.days);
			//printf("\n=> day - %c\n", idx[i-1].schedules.days[day_index]);
			isValid = idx[i-1].schedules.days[day_index] - '0';

			if(isEndTime){
				// check prev day in case of schedule start time was valid.
				int end_time = idx[i-1].schedules.s_time + idx[i-1].schedules.duration;
				if(end_time >=1440 )
					end_time = end_time - 1440;
				if( idx[i-1].schedules.s_time > end_time){
					if(curr_time < idx[i-1].schedules.s_time ){//on or after 12 AM check previous day for validity else same day
						if(day_index == 0){
							day_index = 6;//0 - 1 = 6
						}else{
							day_index = day_index - 1;
						}
					}
					isValid = idx[i-1].schedules.days[day_index] - '0';
					printf("\ndys %s,d_idx %d, isVld - %d\n",idx[i-1].schedules.days,day_index,isValid);
					// return 1;//return true as start time was on valid day
				}
			}

			break;
		}
	}
	printf("\nis Vld D for sch - %d\n",isValid);
	return isValid;
}

int getClosestTime(){
	int i = 0;
	for(; i < MAX_SCHEDULES; i++){
		if(strcmp(idx[i].uuid,"free") != 0 ){
			break;
		}
	}
	printf("\ngtClstTm : %d\n",idx[i].schedules.s_time);
	if (i >= MAX_SCHEDULES){
		return -1;
	}
	return idx[i].schedules.s_time;
}

// int getClosestStartAction(){
// 	printf("\n getClosestStartAction : %d\n",idx[0].schedules.s_action);
// 	return idx[0].schedules.s_action;
// }

// int getClosestEndAction(){
// 	printf("\ngetClosestEndAction : %d\n",idx[0].schedules.e_action);
// 	return idx[0].schedules.e_action;
// }

// char *getClosestActionVal(){
// 	printf("\ngetClosestActionVal : %s\n",curr_act_val);
// 	if(strcmp(curr_act_val,"start") == 0 || strcmp(curr_act_val,"end") == 0){
// 		return NULL;
// 	}
// 	return curr_act_val; // handle later for color and scene
// }

void printSchedule(){
	printf("\nid\tST_time\tDuration\tstate\tUUID\n");
    printf("-----------------------------------\n");
	for(int si = 0;si < MAX_SCHEDULES; si++){
		printf("%d\t",idx[si].id);
        printf("%d\t",idx[si].schedules.s_time);
        printf("%d\t\t",idx[si].schedules.duration);
        printf("%d\t",idx[si].schedules.state );
		printf("%s\t",idx[si].uuid);
        //printf("\n%c",idx[si].schedules.s_action_val );
        //printf("\n%c",idx[si].schedules.e_action_val );
    	printf("\n");
	}
    printf("\n--------------------------------\n");
}


void printActionSchedule(){
	printf("\nidx\tid\ttime\tAction\n");
    printf("-----------------------------------\n");
	for(int si = 0;si < 2*MAX_SCHEDULES; si++){
		printf("%d\t",si);
		printf("%d\t",idx_action[si].id);
        printf("%d\t",idx_action[si].time);
		printf("%d\t",idx_action[si].action);
		//printf("%c\t",idx_action[si].action_val);
    	printf("\n");
	}
    printf("\n--------------------------------\n");
}

void clearScheduleFromSpiffs(int p_idx);
void persistSchedule(char *data, int p_idx);
void init_schedule_idx();


void init_schedule_idx(){
	for(int i = 1;i <= MAX_SCHEDULES; i++){
		idx[i-1].id = i;
		idx[i-1].schedules.s_time = -1;
		idx[i-1].schedules.s_action = -1;
		idx[i-1].schedules.e_action = -1;
		//idx[i-1].schedules.s_action_val = 's';
		//idx[i-1].schedules.e_action_val = 'e';
		memset(idx[i-1].uuid, 0, sizeof(idx[i-1].uuid));
		strcpy(idx[i-1].uuid,"free");
	}
}

void init_action_schedule_idx(){
	for(int i = 0;i < 2*MAX_SCHEDULES; i++){
		idx_action[i].id = -1;
		idx_action[i].time = -1;
		idx_action[i].action = -1;
		//idx_action[i].action_val = '0';
		// memset(idx_action[i].action_val, 0, sizeof(idx_action[i].action_val));
		// strcpy(idx_action[i].action_val,"end");
	}
}

int getFreeActionIndex(){
	int i=0;
	int isFree = 0;
	for(; i < 2*MAX_SCHEDULES; i++){
		if(idx_action[i].id == -1){
			isFree = 1;
			break;
		}
	}
	if(!isFree){
		return -1;
	}
	printf("\nFr Idx : %d\n",i);
	return i;
}

int getScheduleIndex(char *uid){
	int i=1;
	int found = 0;
	for(; i <= MAX_SCHEDULES; i++){
		if(strcmp(uid, idx[i-1].uuid) == 0){
			found = 1;
			break;
		}
	}
	if(!found){
		return -1;
	}
	printf("\nExisting Index : %d\n",idx[i-1].id);
	return idx[i-1].id; 
}

int getFreeScheduleIndex(){
	int i=1;
	int isFree = 0;
	for(; i <= MAX_SCHEDULES; i++){
		if(strcmp(idx[i-1].uuid, "free") == 0){
			isFree = 1;
			break;
		}
	}
	if(!isFree){
		return -1;
	}
	printf("\nFree Index : %d\n",idx[i-1].id);
	return idx[i-1].id;
}

void deleteSchedule(int id){
	if(id == -1){
		return;//nothing to delete
	}
	clearScheduleFromSpiffs(id);
	int i;
	for(i = 0; i < MAX_SCHEDULES;i++){
		// printf("\n\n==>>deleteSchedule id = %d -> idx[%d].id = %d",id,i,idx[i].id);

		if(idx[i].id == id){
			idx[i].schedules.s_time = 0;
			idx[i].schedules.s_action = 0;
			idx[i].schedules.e_action = 0;
			//idx[i].schedules.s_action_val = 's';
			//idx[i].schedules.e_action_val = 'e';
			idx[i].schedules.duration = 0;
			idx[i].schedules.state = 0;
			memset(idx[i].schedules.days,0,sizeof(idx[i].schedules.days));
			// memset(idx[i].schedules.s_action_val,0,sizeof(idx[i].schedules.s_action_val));
			// memset(idx[i].schedules.e_action_val,0,sizeof(idx[i].schedules.e_action_val));
			memset(idx[i].uuid,0,sizeof(idx[i].uuid));
			strcpy(idx[i].uuid,"free");
			break;
		}
	}

	for(i = 0; i < 2*MAX_SCHEDULES;i++){
		// printf("\n\n==>>deleteSchedule id = %d -> idx[%d].id = %d",id,i,idx_action[i].id);
		if(idx_action[i].id == id){//clean 2 indexes
			idx_action[i].id = -1;//empty index
			idx_action[i].time = -1;
			idx_action[i].action = -1;
			//idx_action[i].action_val = '0';
			// memset(idx_action[i].action_val,0,sizeof(idx_action[i].action_val));
		}

	}

	available_schedule--;
	if(available_schedule >1 ){
		sortSchedule();//sot idx_action ...
	}

}

void addSchedule(schedule_t s, char * uuid){

	//memset(schedules[available_schedule], 0, sizeof(schedule_t));// for fresh schedule but if we need to update
	//char *schFileName = "%s_%d_%d_%s_%d_%d_%d";
	char *schFileName = "%s_%d_%d_%s_%d_%d_%d";//added start action value and end action value
	deleteSchedule(getScheduleIndex(uuid));
	memset(sch_file_name,0,sizeof(sch_file_name));

	int i = getFreeScheduleIndex();
	if(i < 0){
		//printf("\n\n==>>addSchedule full not adding ... \n\n");
		return;
	}
	// printf("\n\n==>>addSchedule going to fill schedules i=%d  \n\n",i);

	idx[i-1].schedules.s_time = s.s_time;
	idx[i-1].schedules.s_action = s.s_action;
	idx[i-1].schedules.e_action = s.e_action;
	idx[i-1].schedules.duration = s.duration;
	idx[i-1].schedules.state = s.state;
	memset(idx[i-1].uuid,0,sizeof(idx[i-1].uuid));
	memset(idx[i-1].schedules.days,0,sizeof(idx[i-1].schedules.days));
	// memset(idx[i-1].schedules.s_action_val,0,sizeof(idx[i-1].schedules.s_action_val));
	// memset(idx[i-1].schedules.e_action_val,0,sizeof(idx[i-1].schedules.e_action_val));
	strcpy(idx[i-1].uuid,uuid);
	strcpy(idx[i-1].schedules.days,s.days);

	// if(strlen(s.s_action_val) > 5) // start-some value color or scene
	// 	strcpy(idx[i-1].schedules.s_action_val,s.s_action_val);
	// else
	// 	strcpy(idx[i-1].schedules.s_action_val,"start");

	// if(strlen(s.e_action_val) > 5) // end -
	// 	strcpy(idx[i-1].schedules.e_action_val,s.e_action_val);
	// else
	// 	strcpy(idx[i-1].schedules.e_action_val,"end");
	// sprintf(sch_file_name,schFileName,uuid,s.s_time,s.duration,s.days,s.s_action,s.e_action,s.state);
	sprintf(sch_file_name,schFileName,uuid,s.s_time,s.duration,s.days,s.s_action,s.e_action,s.state);
//--------- add two time point ------------
	int j = getFreeActionIndex();
	idx_action[j].id = idx[i-1].id;
	idx_action[j].time = idx[i-1].schedules.s_time;
	idx_action[j].action = idx[i-1].schedules.s_action;
	//idx_action[j].action_val = idx[i-1].schedules.s_action_val;
	// memset(idx_action[j].action_val, 0, sizeof(idx_action[j].action_val));
	// strcpy(idx_action[j].action_val, idx[i-1].schedules.s_action_val);

	j = getFreeActionIndex();

	idx_action[j].id = idx[i-1].id;
	if(s.duration == 0){
		idx_action[j].time = -1;//idx[i-1].schedules.s_time + idx[i-1].schedules.duration;
	}
	else{
		if(idx[i-1].schedules.s_time + idx[i-1].schedules.duration < 1440)
			idx_action[j].time = idx[i-1].schedules.s_time + idx[i-1].schedules.duration;
		else
			idx_action[j].time = idx[i-1].schedules.s_time + idx[i-1].schedules.duration - 1440;
	}

	idx_action[j].action = idx[i-1].schedules.e_action;
	//idx_action[j].action_val = idx[i-1].schedules.e_action_val;

	// memset(idx_action[j].action_val, 0, sizeof(idx_action[j].action_val));
	// strcpy(idx_action[j].action_val, idx[i-1].schedules.e_action_val);
//-------------------------

	persistSchedule(sch_file_name,i);//store at spiffs

	//printf("\n\n==>>addSchedule going to sortSchedule file ... \n\n");
//	if(available_schedule > 1)
	sortSchedule();//sort action struct
	//printSchedule();
    //printActionSchedule();

	available_schedule++;
}
//IMP
//Brownout detection is a hardware feature that shuts down the processor if the system voltage is below a threshold


//schedule file name
//uuid_stime_duration_weekdays_sactiontype_eactiontype_state.txt

//566578d8-12b7-4a37-944c-aec851811f15_1380_3600_1111111_0_1_1

int getTotalSchedule(){
	return available_schedule;
}

void setTotalSchedule(int cnt){
	available_schedule = cnt;
}



void parseScheduleJson(char *jsonbuff){

	cJSON_Hooks memoryHook;
	//printf("\nparseScheduleJson :: %s\n",jsonbuff);
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);
 	// printf("\nparseScheduleJson Entry\n");
    cJSON *root = cJSON_Parse(jsonbuff);
    long currentTimestamp = 0;
    if(root != NULL){
    	cJSON *command = NULL;
    	cJSON *devices = NULL;
    	cJSON *services = NULL;
    	cJSON *serviceName = NULL;
    	cJSON *commands = NULL;
    	cJSON *attributeName = NULL;
    	cJSON *actions = NULL;
    	cJSON *s_action = NULL;
    	cJSON *e_action = NULL;
    	cJSON *rule = NULL;
    	cJSON *params = NULL;
    	command = cJSON_GetObjectItem(root, "command");
    	if(command != NULL){
    		devices = cJSON_GetObjectItem(command, "devices");
    	}
    	if(devices != NULL){
    		//lock deviceUUID get and use to fetch mpin pass1 macid
    		if(cJSON_GetObjectItem(devices, "deviceUUID") != NULL){
    			char *tmp = cJSON_GetObjectItem(devices,"deviceUUID")->valuestring;
    			printf("\nlduid : %s \r\n",tmp);

    		}
    		services = cJSON_GetObjectItem(devices, "services");
    	}else{
    		printf("\n\n==>>devices is NULL:\n\n");
    	}
    	if(services != NULL){
    		serviceName = cJSON_GetObjectItem(services, "ruleService");
    	}    	
    	else{
    		printf("\n\n==>>services is NULL:\n\n");
    	}

    	if(serviceName != NULL){
    		commands = cJSON_GetObjectItem(serviceName, "commands");
    	}
    	else{
    		printf("\n\n==>>ruleService is NULL:\n\n");
    	}

		if(commands != NULL){
    		rule = cJSON_GetObjectItem(commands, "rule");
    	}
    	else{
    		printf("\n\n==>>commands is NULL:\n\n");
    	}

    	if(rule != NULL){
    		params = cJSON_GetObjectItem(rule, "parameters");
    	}

    	else{
    		printf("\n\n==>>rule is NULL:\n\n");
    	}
    	if(params != NULL){
    		ESP_LOGI(TAG,"------------Json Parsed");

    		cJSON *stateJson = cJSON_GetObjectItem(params, "state");
            int state = -1;
            if(cJSON_IsNumber(stateJson))
            {
            	state = cJSON_GetObjectItem(params, "state")->valueint;
                printf("\n Its a state => %d \n", state);
            }
            else
            {
               state = atoi(cJSON_GetObjectItem(params, "state")->valuestring);
               printf("\n Its not state => %d \n", state);
            }

    		char *sch_id = cJSON_GetObjectItem(params, "uuid")->valuestring;

    		if(state == 2 || state == 0 ){
				deleteSchedule(getScheduleIndex(sch_id));
				goto EXIT_SCHEDULE_PARSE;
			}
			cJSON *dur = cJSON_GetObjectItem(params, "duration");
            int duration = 0;
            if(cJSON_IsNumber(dur))
            {
            	duration = cJSON_GetObjectItem(params, "duration")->valueint;
                printf("\n Its a numberDuration => %d \n", duration);
            }
            else
            {
               duration = atoi(cJSON_GetObjectItem(params, "duration")->valuestring);
               printf("\n Its not numberDuration => %d \n", duration);
            }
    		char *name = cJSON_GetObjectItem(params, "name")->valuestring;
    		////0 for deviceUUID, 1 :- for entity UUID, 2:- for unit UUID
    		cJSON *sch = cJSON_GetObjectItem(params, "schType");
            int schType = 0;
            if(cJSON_IsNumber(sch))
            {
            	schType = cJSON_GetObjectItem(params, "schType")->valueint;
                printf("\n Its a schType => %d \n", schType);
            }
            else
            {
               schType = atoi(cJSON_GetObjectItem(params, "schType")->valuestring);
               printf("\n Its not schType => %d \n", schType);
            }
    		//int schType = cJSON_GetObjectItem(params, "schType")->valueint;
    		char *start_time = cJSON_GetObjectItem(params, "startTime")->valuestring;
    		char *weekdays = cJSON_GetObjectItem(params, "weekdays")->valuestring;

    		// printf("\nduration : %d", duration);
    		// printf("\nname : %s",name);
    		// printf("\nschType : %d",schType);
    		// printf("\nst_time : %s",start_time);
    		// printf("\nstate : %d",state);
    		// printf("\nsch_id : %s",sch_id);
    		// printf("\nweekdays : %s\n",weekdays);


    		actions = cJSON_GetObjectItem(params, "actions");
    		int s_act_type = 5;//def-Non Negative
    		int e_act_type = 5;
    		char *s_act_val = NULL;
    		char *e_act_val = NULL;

    		//start - end Action
    		if(actions != NULL){
				s_action = cJSON_GetObjectItem(actions, "startAction");
				if(s_action != NULL){
					//0-ON,1-OFF,2-Scene
					if(cJSON_GetObjectItem(s_action, "actionType")){
						s_act_type = atoi(cJSON_GetObjectItem(s_action, "actionType")->valuestring);
						printf("\n s_act_type : %d\n",s_act_type);
					}
					// if(cJSON_GetObjectItem(s_action, "actionValue")){
					// 	s_act_val = cJSON_GetObjectItem(s_action, "actionValue")->valuestring;
					// }
				}
				e_action = cJSON_GetObjectItem(actions, "endAction");
				if(e_action != NULL){
					if(cJSON_GetObjectItem(e_action, "actionType")){
						e_act_type = atoi(cJSON_GetObjectItem(e_action, "actionType")->valuestring);
					}
					// if(cJSON_GetObjectItem(e_action, "actionValue")){
					// 	e_act_val = cJSON_GetObjectItem(e_action, "actionValue")->valuestring;
					// }
				}
				int time_in_min = atoi(strtok(start_time,":"))*60 ;
				time_in_min = time_in_min + atoi(strtok(NULL,":"));
				//printf("\n\n==>>Fill schedule structure\n\n");
				schedule_t s;
				s.s_time = time_in_min;
				s.s_action = s_act_type;
				s.e_action = e_act_type;
				s.duration = duration;
				s.state = state;
				memset(s.days, 0,sizeof(s.days));
				// memset(s.s_action_val, 0,sizeof(s.s_action_val));
				// memset(s.e_action_val, 0,sizeof(s.e_action_val));
				strcpy(s.days, weekdays);
				// if(s_act_val)
				// 	strcpy(s.s_action_val, s_act_val);
				// if(e_act_val)
				// 	strcpy(s.e_action_val, e_act_val);
				strcpy(s.days, weekdays);
				// printf("\n\n==>>Going to addSchedule ... \n\n");
				addSchedule(s, sch_id);//0 index based or count from loadSchedule

				// publish feedbak to ensure schedule mqtt received and saved by device
    		}

    	}
    	else{
    		printf("\n\n==>>params is NULL:\n\n");
    	}

EXIT_SCHEDULE_PARSE:
		// free the memory!
	    cJSON_Delete(root);
	    setJsonParseStatus(1);
	} else {
 		// printf("\nparseScheduleJson root is NULL\n");
 		setJsonParseStatus(2);//json parse fails
	}
 	// printf("\nparseScheduleJson Exit\n");
}

void removeChar(char * str){
    int i, j;
    int len = strlen(str);
    for(i=0; i<len; i++)
    {
        if(str[i] == '/')
        {
            for(j=i; j<len; j++)
            {
                str[j] = str[j+1];
            }
            len--;
            i--;
        }
    }

}


void parseRestScheduleJson(char *jsonbuff){// sachin modify as per smartPlug Case	
	cJSON_Hooks memoryHook;
	removeChar(jsonbuff);

	printf("\n parseRestScheduleJson :: %s\n",jsonbuff);
	downloadedRecords++;
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);
 	printf("\n parseRestScheduleJson Entry\n");
    cJSON *root = cJSON_Parse(jsonbuff);
    long currentTimestamp = 0;
       	cJSON *command = NULL;
    	cJSON *devices = NULL;
    	cJSON *services = NULL;
    	cJSON *serviceName = NULL;
    	cJSON *commands = NULL;
    	cJSON *attributeName = NULL;
    	cJSON *actions = NULL;
    	cJSON *s_action = NULL;
    	cJSON *e_action = NULL;
    	cJSON *rule = NULL;
    	cJSON *params = NULL;
    if(root != NULL){
    	cJSON *command = NULL;
    	int totalRecords = cJSON_GetObjectItem(root, "totalRecords")->valueint;
    	ESP_LOGI(TAG,"Total Records [%d]",totalRecords);
    	setTotalRecords(totalRecords);
    	command = cJSON_GetObjectItem(root,"bulbRule");
    	int ruleCount = cJSON_GetArraySize(command);
    	ESP_LOGI(TAG,"Total Rules in Current Payload [%d]",ruleCount);
    	for (int i = 0; i < ruleCount; i++)
    	{
        // printf("scene Array :\n");
        cJSON *params = cJSON_GetArrayItem(command, i);
        if(params != NULL){
    		ESP_LOGI(TAG,"------------Json Parsed");

    		cJSON *stateJson = cJSON_GetObjectItem(params, "state");
            int state = -1;
            if(cJSON_IsNumber(stateJson))
            {
            	state = cJSON_GetObjectItem(params, "state")->valueint;
                printf("\n Its a state => %d \n", state);
            }
            else
            {
               state = atoi(cJSON_GetObjectItem(params, "state")->valuestring);
               printf("\n Its not state => %d \n", state);
            }

    		char *sch_id = cJSON_GetObjectItem(params, "uuid")->valuestring;

    		if(state == 2 || state == 0 ){
				deleteSchedule(getScheduleIndex(sch_id));
				goto EXIT_REST_SCHEDULE_PARSE;
			}
			cJSON *dur = cJSON_GetObjectItem(params, "duration");
            int duration = 0;
            if(cJSON_IsNumber(dur))
            {
            	duration = cJSON_GetObjectItem(params, "duration")->valueint;
                printf("\n Its a numberDuration => %d \n", duration);
            }
            else
            {
               duration = atoi(cJSON_GetObjectItem(params, "duration")->valuestring);
               printf("\n Its not numberDuration => %d \n", duration);
            }
    		char *name = cJSON_GetObjectItem(params, "name")->valuestring;
    		////0 for deviceUUID, 1 :- for entity UUID, 2:- for unit UUID
    		cJSON *sch = cJSON_GetObjectItem(params, "schType");
            int schType = 0;
            if(cJSON_IsNumber(sch))
            {
            	schType = cJSON_GetObjectItem(params, "schType")->valueint;
                printf("\n Its a schType => %d \n", schType);
            }
            else
            {
               schType = atoi(cJSON_GetObjectItem(params, "schType")->valuestring);
               printf("\n Its not schType => %d \n", schType);
            }
    		//int schType = cJSON_GetObjectItem(params, "schType")->valueint;
    		char *start_time = cJSON_GetObjectItem(params, "startTime")->valuestring;
    		char *weekdays = cJSON_GetObjectItem(params, "weekdays")->valuestring;

    		// printf("\nduration : %d", duration);
    		// printf("\nname : %s",name);
    		// printf("\nschType : %d",schType);
    		// printf("\nst_time : %s",start_time);
    		// printf("\nstate : %d",state);
    		// printf("\nsch_id : %s",sch_id);
    		// printf("\nweekdays : %s\n",weekdays);


    		actions = cJSON_GetObjectItem(params, "actions");
    		int s_act_type = 5;//def-Non Negative
    		int e_act_type = 5;
    		char *s_act_val = NULL;
    		char *e_act_val = NULL;

    		// char *s_act_sid = NULL;
    		// char *e_act_sid = NULL;

    		char *s_act_sname = NULL;
    		char *e_act_sname = NULL;

    		//start - end Action
    		if(actions != NULL){
				s_action = cJSON_GetObjectItem(actions, "startAction");
				if(s_action != NULL){
					//0-ON,1-OFF,2-Scene
					if(cJSON_GetObjectItem(s_action, "actionType") && !cJSON_IsNull(cJSON_GetObjectItem(s_action, "actionType"))){
						s_act_type = atoi(cJSON_GetObjectItem(s_action, "actionType")->valuestring);
						printf("\n s_act_type : %d\n",s_act_type);
					}
					if((s_act_type == 0 || s_act_type == 2) && !cJSON_IsNull(cJSON_GetObjectItem(s_action, "actionValue"))){
						s_act_val = cJSON_GetObjectItem(s_action, "actionValue")->valuestring;
						printf("\n s_act_val : %s\n",s_act_val);
						if(strcmp(s_act_val,"null") == 0){
							s_act_val = NULL;
						}
					}
					// if(!cJSON_IsNull(cJSON_GetObjectItem(e_action, "actionSceneUID"))){
					// 	s_act_sid = cJSON_GetObjectItem(s_action, "actionSceneUID")->valuestring;
					// 	printf("\n s_act_sid : %s\n",s_act_sid);
					// }
					// if(s_act_type == 2 && !cJSON_IsNull(cJSON_GetObjectItem(e_action, "actionSceneName"))){
					// 	s_act_sname = cJSON_GetObjectItem(s_action, "actionSceneName")->valuestring;
					// 	printf("\n s_act_sname : %s\n",s_act_sname);
					// }
				}
				e_action = cJSON_GetObjectItem(actions, "endAction");
				if(e_action != NULL){
					if(cJSON_GetObjectItem(e_action, "actionType") && !cJSON_IsNull(cJSON_GetObjectItem(e_action, "actionType"))){
						e_act_type = atoi(cJSON_GetObjectItem(e_action, "actionType")->valuestring);
						printf("\n e_act_type : %d\n",e_act_type);

					}
					if((e_act_type == 0 || e_act_type == 2) && !cJSON_IsNull(cJSON_GetObjectItem(e_action, "actionValue"))){
						e_act_val = cJSON_GetObjectItem(e_action, "actionValue")->valuestring;
						printf("\n e_act_val : %s\n",e_act_val);
						if(strcmp(e_act_val,"null") == 0){
							e_act_val = NULL;
						}

					}
					// if(!cJSON_IsNull(cJSON_GetObjectItem(e_action, "actionSceneUID"))){
					// 	e_act_sid = cJSON_GetObjectItem(e_action, "actionSceneUID")->valuestring;
					// 	printf("\n e_act_sid : %s\n",e_act_sid);

					// }
					// if(e_act_type == 2 && !cJSON_IsNull(cJSON_GetObjectItem(e_action, "actionSceneName"))){
					// 	e_act_sname = cJSON_GetObjectItem(e_action, "actionSceneName")->valuestring;
					// 	printf("\n e_act_sname : %s\n",e_act_sname);

					// }
				}
				int time_in_min = atoi(strtok(start_time,":"))*60 ;
				time_in_min = time_in_min + atoi(strtok(NULL,":"));
				//printf("\n\n==>>Fill schedule structure\n\n");
				schedule_t s;
				s.s_time = time_in_min;
				s.s_action = s_act_type;
				s.e_action = e_act_type;
				s.duration = duration;
				s.state = state;
				memset(s.days, 0,sizeof(s.days));
				
				//store in file colors/scenes ???? 
				//s_action_val / e_action_val store in file
				
				// memset(s.s_action_val, 0,sizeof(s.s_action_val));
				// memset(s.e_action_val, 0,sizeof(s.e_action_val));
				strcpy(s.days, weekdays);


				// if(s_act_val){
				// 	//strcpy(s.s_action_val, s_act_val);
				// 	s.s_action_val = 's';
				// }else{
				// 	s.s_action_val = '0';
				// }
				// if(e_act_val){
				// 	// strcpy(s.e_action_val, e_act_val);
				// 	s.e_action_val = 'e';
				// }else{
				// 	s.e_action_val = '0';
				// }
				strcpy(s.days, weekdays);
				printf("\n\n==>>Going to addSchedule ... \n\n");
				addSchedule(s, sch_id);//0 index based or count from loadSchedule			
    		} 
    		}
    		}  	            		
		// free the memory!
EXIT_REST_SCHEDULE_PARSE:    		
	    cJSON_Delete(root);
	    setJsonParseStatus(1);
	} else {
 		printf("\n parseRestScheduleJson root is NULL\n");
 		setJsonParseStatus(2);//json parse fails
	}
 	printf("\n parseRestScheduleJson Exit\n");
}

int si = 0;//for loadSchedule 
int ai = 0;//for loadSchedule 
void loadScheduleFile(char *path) {// list files from /spiffs/sched/

    DIR *dir = NULL;
    struct dirent *ent;
    char size[9];
    char tpath[128];
    char tbuffer[80];
    //char *lpath = NULL;
    //struct stat sb;

    // printf("\nList of Directory [%s]\n", path);
    // printf("-----------------------------------\n");
    // Open directory
    dir = opendir(path);
    if (!dir) {
        printf("Error opening directory\n");
        return;
    }

    // Read directory entries

    // printf("T  Size      Date/Time         Name\n");
    // printf("-----------------------------------\n");
    while ((ent = readdir(dir)) != NULL) {
        sprintf(tpath, path);
        if (path[strlen(path)-1] != '/') strcat(tpath,"/");
        strcat(tpath,ent->d_name);
        tbuffer[0] = '\0';
        //printf("\ntpath : %s\n",tpath);
        if(strstr(tpath,"sch")){

        	int index=0;
        	sscanf(tpath,"/spiffs/sch%d.cfg",&index);
        	// printf("\nschedule index - [%d]\n",index);

            FILE *f = fopen(tpath, "r");
            if (f == NULL) {
                printf("Failed to open schedule for writing");        
            } else {
                printf("%s open success\n",tpath);
            }
            char data[1024];
            memset(data, 0, sizeof(data));
            fscanf(f, "%s",data);
            //printf("data => :: %s",data);
            //1106e92c-059f-486e-af61-17783dd0a3fc_520_1240_1111111_0_1_1
            //
            char *uuid = strtok(data, "_");
            int start_time = atoi(strtok(NULL, "_"));
            int duration = atoi(strtok(NULL, "_"));
            char *days = strtok(NULL, "_");
            int s_act = atoi(strtok(NULL, "_"));
            int e_act = atoi(strtok(NULL, "_"));
            int state = atoi(strtok(NULL, "_"));
            // char *st_actVal = strtok(NULL, "_");
            // char *en_actVal=NULL;
            // if(st_actVal != NULL)
            // 	en_actVal = strtok(NULL, "_");

            // if(en_actVal == NULL)
            // {
            // 	en_actVal = "end";
            // }
             //printf("\nuuid : %s",uuid);
             //printf("\nst_time : %d",start_time);
             //printf("\nduration : %d", duration);
             //printf("\nweekdays : %s\n",days);
             //printf("\ns_act : %d",s_act);
             //printf("\ne_act : %d",e_act);
             //printf("\nstate : %d",state);


            //printf("\n st_actVal : %s",st_actVal);
             //printf("\n en_actVal : %s",en_actVal);

            fclose(f);
//----- for debuging
            idx[si].id = index;
			idx[si].schedules.s_time = start_time;
			idx[si].schedules.s_action = s_act;
			idx[si].schedules.e_action = e_act;
			idx[si].schedules.duration = duration;
			idx[si].schedules.state = state;
			memset(idx[si].uuid,0,sizeof(idx[si].uuid));
			memset(idx[si].schedules.days,0,sizeof(idx[si].schedules.days));
			strcpy(idx[si].uuid,uuid);
			strcpy(idx[si].schedules.days,days);
			// memset(idx[si].schedules.s_action_val,0,sizeof(idx[si].schedules.s_action_val));
			// if(st_actVal != NULL){
			// 	strcpy(idx[si].schedules.s_action_val,st_actVal);
			// } else
			// 	strcpy(idx[si].schedules.s_action_val,"start");
			// memset(idx[si].schedules.e_action_val,0,sizeof(idx[si].schedules.e_action_val));
			// if(en_actVal != NULL){
			// 	strcpy(idx[si].schedules.e_action_val,en_actVal);
			// } else
			// 	strcpy(idx[si].schedules.e_action_val,"end");
//----- for debuging
			idx_action[ai].id = index;
			idx_action[ai].time = start_time;
			idx_action[ai].action = s_act;
			//idx_action[ai].action_val = 's';
			// memset(idx_action[ai].action_val,0,sizeof(idx_action[ai].action_val));
			// if(st_actVal != NULL){
			// 	strcpy(idx_action[ai].action_val,st_actVal);
			// }else
			// 	strcpy(idx_action[si].action_val,"end");
			ai++;
			idx_action[ai].id = index;
			// idx_action[ai].time = (start_time + duration)%1440;
			if(duration == 0){
				idx_action[ai].time = -1;//(start_time + duration);
			}
			else{
				if(start_time + duration >= 1440)
					idx_action[ai].time = (start_time + duration - 1440);
				else
					idx_action[ai].time = (start_time + duration);
			}
			idx_action[ai].action = e_act;
			//idx_action[ai].action_val = 'e';
			// memset(idx_action[ai].action_val,0,sizeof(idx_action[ai].action_val));
			// if(en_actVal != NULL){
			// 	strcpy(idx_action[ai].action_val,en_actVal);
			// }

			ai++;
            si++;
            //addSchedule(s,uuid);
        }
    }
    setTotalSchedule(si);
    sortSchedule();
    //printSchedule();
    //printActionSchedule();
    // printf("\n-----------------------------------\n");

    closedir(dir);

    //free(lpath);

    uint32_t tot=0, used=0;
    esp_spiffs_info(NULL, &tot, &used);
    printf("SPIFFS: free %d KB of %d KB\n", (tot-used) / 1024, tot / 1024);
    // printf("-----------------------------------\n\n");
}

static unsigned int count = 0;
void incCount(){
	count++;
}
int getCount(){
	return count;
}

void turnOnPlug(int viaMqtt, uint8_t numb){
	setTurnONState(true);
	notify_over_control_chr(UPDATE_SWITCH_STATUS);
	printf("\n turnOn - viaMqtt[%d]\n",viaMqtt);
	gpio_set_level(4,1);
	gpio_set_level(numb,1);			//Set high the relay switch Number corresponding to switch number
	char *swver = getCurrentSWVersion();
    if( strstr(swver,"SYSTEM-16A") == NULL ){
		gpio_set_level(6/*LED_GPIO*/,0);
	}
	setLastKnownStatus("on");
	//added factory case
	if(viaMqtt == 2){ // just on/off in callibration mode no need to save or mqtt
		count = 0;
		return;
	}
	MQTTPublishMsg("lcSwitchControl", "power","on");
	// if(viaMqtt == 1)
		incCount();

}

void turnOffPlug(int viaMqtt, uint8_t numb){
	setTurnONState(false);
	notify_over_control_chr(UPDATE_SWITCH_STATUS);
	printf("\n turnOff - viaMqtt[%d]\n",viaMqtt);
	gpio_set_level(4,0);
	gpio_set_level(numb,0);			//Set low the relay switch Number corresponding to switch number
	char *swver = getCurrentSWVersion();
	
    if( strstr(swver,"SYSTEM-16A") == NULL){
		gpio_set_level(6/*LED_GPIO*/,1);
	}
	setLastKnownStatus("off");
	//added factory case
	if(viaMqtt == 2){ // just on/off in callibration mode no need to save or mqtt
		count = 1;
		return;
	}

	MQTTPublishMsg("lcSwitchControl", "power","off");

	// if(viaMqtt == 1)
		incCount();
}

#endif
