/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "json_parse.h"
#include <esp_spiffs.h>

static const char *TAG = "SMARTPLUG_MQTTS";



#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

char sub_topic1[128]={0};
char config_topic[128]={0};
char pub_topic[128]={0};
char mqtt_buff[2000];
char lwtpayload[512];
char clientID[100];
char lwtTopic[100];
char mqtts_url[50];
char payload[512];
esp_mqtt_client_handle_t client;
uint32_t lasttimestamp = 0;

//void executeScene(char *sceneData);
//void removeSceneTask();

void executeTimerTask(char *sceneData);
void removeTimerTask();
char *getIPAddress();
int getSignalStrength();
char * readNVData(char *key);
int writeNVData(char *key, char *value);

void removeMqttParseTask();
void executeMqttParseTask(char *mqttData);
//void fadeLedColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white);
void publishWifiStatus();
long onlineTime = 0;

esp_mqtt_client_handle_t getMQTTClient()
{
    return client;
}


void setConfigOnDisconnet(esp_mqtt_client_handle_t activeclient)
{
    memset(clientID,0,sizeof(clientID));
    char *tmp = "CLIENT_HSP_%s_MainMqttClient_%s";
    sprintf(clientID,tmp,getHUBMacID(),getUnitUUID());   
    
    memset(lwtTopic,0,sizeof(lwtTopic));
    tmp = "/monitor/%s/%s/heartbeat";
    sprintf(lwtTopic,tmp,getUnitUUID(),getDeviceUUID());
    onlineTime = currentSec();
    char* will_msg="{\"devices\" : {\"action\" : 140,\"deviceUUID\" : \"%s\",\"entityUUID\" : \"%s\",\"msgSequenceId\" : %d001,\"operationState\" : \"offline\",\"serviceProviderId\" : \"%s\",\"srcDeviceId\" : \"HSP_%s\",\"timestamp\" : %d001,\"unitUUID\" : \"%s\",\"userUUID\" : \"%s\"}}";
    memset(lwtpayload,0,sizeof(lwtpayload));
    sprintf(lwtpayload, will_msg,getDeviceUUID(),getEntityUUID(),onlineTime,getSpid(),getHUBMacID(),onlineTime,getUnitUUID(),/*getGWUUID()*/getDeviceUUID());

    
    memset(mqtts_url,0,sizeof(mqtts_url));
    sprintf(mqtts_url,"mqtts://%s",getmqttURL());
    ESP_LOGI(TAG, "[APP] mqtts_url: %s ",getmqttURL());
    ESP_LOGI(TAG, "[APP] mqtts_url: %s ",mqtts_url );
    ESP_LOGI(TAG, "[APP] clientID: %s ",clientID );
    ESP_LOGI(TAG, "[APP] getDeviceUUID: %s ",getDeviceUUID());
    ESP_LOGI(TAG, "[APP] getGWUUID: %s ",getGWUUID());
    ESP_LOGI(TAG, "[APP] getAccessToken: %s ",getAccessToken());
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = mqtts_url,//CONFIG_BROKER_URI,
            .verification.skip_cert_common_name_check = 1,
            //.verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start
        },
        .credentials = {
            .client_id = clientID,
            .username = getGWUUID(),
            .authentication.password = getAccessToken(),
        },
        .session = {
            .last_will.topic = lwtTopic,
            .last_will.msg = lwtpayload,
            .last_will.retain = 1,
            .last_will.qos = 0,
            .keepalive = 10,
        },
        .network = {
            .reconnect_timeout_ms = 5000,
        },
    };
    ESP_LOGW(TAG, "LWT Topic   --  %s ",lwtTopic);
    ESP_LOGW(TAG, "LWT Message --  %s ", lwtpayload);
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_set_config(activeclient,&mqtt_cfg);
    
}

void meteringMqttUpdate(int viaBle);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{

    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    char* sub_topic_c = "/control/%s/%s/#";    
    char* pub_topic_hb = "/monitor/%s/%s/heartbeat";
    char* config_topic_c = "/config/%s";
    
    memset(sub_topic1,0,sizeof(sub_topic1));    
    memset(pub_topic,0,sizeof(pub_topic));
    memset(config_topic,0,sizeof(config_topic));

    sprintf(sub_topic1,sub_topic_c,getUnitUUID(),getDeviceUUID());
    sprintf(pub_topic,pub_topic_hb,getUnitUUID(),getDeviceUUID());
    
    sprintf(config_topic,config_topic_c,getUnitUUID());
    
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        char* online_msg="{\"devices\" : {\"action\" : 140,\"deviceUUID\" : \"%s\",\"entityUUID\" : \"%s\",\"msgSequenceId\" : %d000,\"operationState\" : \"online\",\"serviceProviderId\" : \"%s\",\"srcDeviceId\" : \"HSP_%s\",\"timestamp\" : %d000,\"unitUUID\" : \"%s\",\"userUUID\" : \"%s\"}}";
        memset(payload,0,sizeof(payload));        
        sprintf(payload, online_msg,getDeviceUUID(),getEntityUUID(),onlineTime,getSpid(),getHUBMacID(),onlineTime,getUnitUUID(),/*getGWUUID()*/getDeviceUUID());

        msg_id = esp_mqtt_client_publish(client, pub_topic, payload, 0, 0, 1 /*retain*/);//HB Online
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, sub_topic1, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, config_topic, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP_LOGE(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        publishWifiStatus();
        setCurrentStatus(MQTT_ONLINE);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGE(TAG, "MQTT_EVENT_DISCONNECTED"); 
        setConfigOnDisconnet(client);
        setCurrentStatus(MQTT_DISCONNECTED);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGE(TAG,"TOPIC=%.*s\r", event->topic_len, event->topic);
        ESP_LOGI(TAG,"DATA=%.*s\r", event->data_len, event->data);
        printf("\nTOPIC=%.*s\r", event->topic_len, event->topic);
        // printf("\nDATA=%.*s\r", event->data_len, event->data);

        if(strstr(event->topic,"config")){
            memset(mqtt_buff,'\0',sizeof(mqtt_buff));
            strncpy(mqtt_buff, (char*)event->data,event->data_len);
            parseConfigJson(mqtt_buff);
            break;
        }
        else if(strstr(event->topic,"control"))
        {
        if(strstr(event->topic,"factoryReset")){
            //erase_data_from_flash();//will reboot also
            factoryReset();
            break;
        }
        else if(strstr(event->topic,"meteringRefresh")){
            ESP_LOGI(TAG,"\r\n updating metering data ... \r\n");
            printf("\r\n updating met data ... \r\n");
            meteringMqttUpdate(0);
            break;
        }
        else if(strstr(event->topic,"autoOffAfter")){
            ESP_LOGI(TAG,"\r\n updating metering data ... \r\n");
            executeMqttParseTask(mqtt_buff);// to get time value
            break;
        }
        else if(strstr(event->topic,"upgradeAvailable")){
            //check comes once or on every reboot
            ESP_LOGI(TAG,"\r\n App Upgrade Available ... \r\n");
            //sys_reset();//reboot and try OTA
            break;
        }
        else if(strstr(event->topic,"deviceReboot")){
            //check comes once or on every reboot
            ESP_LOGI(TAG,"\r\n Rebooting ... \r\n");
            //sys_reset();//reboot
            esp_restart();
            break;
        }
        // else if(strstr(event->topic,"sceneControl")){              
        //     removeSceneTask();            
        //     ESP_LOGI(TAG,"\r\n Scene Task Create ... \r\n");
        //     memset(mqtt_buff,'\0',sizeof(mqtt_buff));
        //     strncpy(mqtt_buff, (char*)event->data,event->data_len);
        //     executeScene(mqtt_buff);
        //     //vTaskDelay(300 / portTICK_PERIOD_MS);
        //     break;
        // }
        else if(strstr(event->topic,"timerControl")){              
            
            removeTimerTask();                        
            ESP_LOGI(TAG,"\r\n Timer Control Task Create ... \r\n");
            memset(mqtt_buff,'\0',sizeof(mqtt_buff));
            strncpy(mqtt_buff, (char*)event->data,event->data_len);
            executeTimerTask(mqtt_buff);
            //vTaskDelay(300 / portTICK_PERIOD_MS);
            break;
        }
        else if(strstr(event->topic,"lcSwitchControl"))
        {            
            vTaskDelay(10 /portTICK_PERIOD_MS);
            removeMqttParseTask();
            vTaskDelay(10 /portTICK_PERIOD_MS);
            memset(mqtt_buff,'\0',sizeof(mqtt_buff));
            strncpy(mqtt_buff, (char*)event->data,event->data_len);
            executeMqttParseTask(mqtt_buff);
            //vTaskDelay(300 / portTICK_PERIOD_MS);
            ESP_LOGI(TAG,"\r\n parsing Data ... \r\n");
             break;
        }
       else if (strstr(event->topic,"lastKnownStatus") || strstr(event->topic,"timeZoneSetting"))
        {
            removeMqttParseTask();
            vTaskDelay(10 /portTICK_PERIOD_MS);           
            memset(mqtt_buff,'\0',sizeof(mqtt_buff));
            strncpy(mqtt_buff, (char*)event->data,event->data_len);
            executeMqttParseTask(mqtt_buff);
            break;
        }
        else if(strstr(event->topic,"ruleService")){
            memset(mqtt_buff,'\0',sizeof(mqtt_buff));
            strncpy(mqtt_buff, (char*)event->data,event->data_len);
            parseScheduleJson(mqtt_buff);
            break;
        }  
        //upgradeAvailable
        else if(strstr(event->topic,"lcOtaService")){             
            vTaskDelay(10 /portTICK_PERIOD_MS);
            removeMqttParseTask();
            vTaskDelay(10 /portTICK_PERIOD_MS);           
            memset(mqtt_buff,'\0',sizeof(mqtt_buff));
            strncpy(mqtt_buff, (char*)event->data,event->data_len);
            executeMqttParseTask(mqtt_buff);                        
            ESP_LOGI(TAG,"\r\n parsing Data ... \r\n");   
            break;
        }            
        else
        {
            ESP_LOGI(TAG,"\r\n didnt Parse data ... %d\r\n",event->data_len);
            break;
        }
        }
        else{
            ESP_LOGI(TAG,"Mmonitor Topic Need to Ignore It\n");
        }
        break;
    case MQTT_EVENT_ERROR:
    //setCurrentStatus(MQTT_CONNECT_FAILED);
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
            if(esp_get_free_heap_size() < 30000)
            {
                ESP_LOGI(TAG, "Connection refused error Rebooting No Memory");
                esp_restart();
            }
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            executecommissionStatustask();

        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }   
        setConfigOnDisconnet(client);     
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "MQTT Free heap size: %d\n", esp_get_free_heap_size());
    printf("\nFree heap size: %d\n", esp_get_free_heap_size());
}
static void mqtt_app_start(void)
{
    
    memset(clientID,0,sizeof(clientID));
    char *tmp = "CLIENT_HSP_%s_MainMqttClient_%s";
    sprintf(clientID,tmp,getHUBMacID(),getUnitUUID());   
    
    memset(lwtTopic,0,sizeof(lwtTopic));
    tmp = "/monitor/%s/%s/heartbeat";
    sprintf(lwtTopic,tmp,getUnitUUID(),getDeviceUUID());
    onlineTime = currentSec();
    char* will_msg="{\"devices\" : {\"action\" : 140,\"deviceUUID\" : \"%s\",\"entityUUID\" : \"%s\",\"msgSequenceId\" : %d001,\"operationState\" : \"offline\",\"serviceProviderId\" : \"%s\",\"srcDeviceId\" : \"HSP_%s\",\"timestamp\" : %d001,\"unitUUID\" : \"%s\",\"userUUID\" : \"%s\"}}";
    memset(lwtpayload,0,sizeof(lwtpayload));
    sprintf(lwtpayload, will_msg,getDeviceUUID(),getEntityUUID(),onlineTime,getSpid(),getHUBMacID(),onlineTime,getUnitUUID(),/*getGWUUID()*/getDeviceUUID());

    
    memset(mqtts_url,0,sizeof(mqtts_url));
    sprintf(mqtts_url,"mqtts://%s",getmqttURL());
    ESP_LOGI(TAG, "[APP] mqtts_url: %s ",getmqttURL());
    ESP_LOGI(TAG, "[APP] mqtts_url: %s ",mqtts_url );
    ESP_LOGI(TAG, "[APP] clientID: %s ",clientID );
    ESP_LOGI(TAG, "[APP] getDeviceUUID: %s ",getDeviceUUID());
    ESP_LOGI(TAG, "[APP] getGWUUID: %s ",getGWUUID());
    ESP_LOGI(TAG, "[APP] getAccessToken: %s ",getAccessToken());
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = mqtts_url,//CONFIG_BROKER_URI,
            .verification.skip_cert_common_name_check = 1,
            //.verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start
        },
        .credentials = {
            .client_id = clientID,
            .username = getGWUUID(),
            .authentication.password = getAccessToken(),
        },
        .session = {
            .last_will.topic = lwtTopic,
            .last_will.msg = lwtpayload,
            .last_will.retain = 1,
            .last_will.qos = 0,
            .keepalive = 10,
        },
        .network = {
            .reconnect_timeout_ms = 5000,
        },
    };
    ESP_LOGW(TAG, "LWT Topic   --  %s ",lwtTopic);
    ESP_LOGW(TAG, "LWT Message --  %s ", lwtpayload);
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);    
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
}

char feedback_msg[1024]={0};
char topic[128]={0};
int isCommisioned();
int publishTimeStamp = 100;

void MQTTPublishMsg(char *servName, char *attrName,char *state)
{
    if(!isCommisioned()) return;
    ESP_LOGI(TAG,"\r Service   Name    -->>  %s",servName);
    ESP_LOGI(TAG,"\r Attribute Name    -->>  %s",attrName);
    ESP_LOGI(TAG,"\r Attribute State   -->>  %s",state);
    publishTimeStamp++;
    if(publishTimeStamp > 998)
    {
        publishTimeStamp = 100;
    }
    char *stateChangeEvent = "{\"devices\":{\"Subscriber-Key\":\"\",\"deviceUUID\":\"%s\",\"entityUUID\":\"%s\",\"msgSequenceId\":%d%d,\"serviceProviderId\":\"%s\",\"services\":{\"%s\":{\"events\":{\"stateChanged\":{\"%s\":\"%s\"}},\"instanceId\":\"0\"}},\"srcDeviceId\":\"HSP_%s\",\"timestamp\":%d%d,\"unitUUID\":\"%s\",\"userUUID\":\"%s\"}}";
    char* sub_topic_m = "/monitor/%s/%s/%s";    
    memset(feedback_msg,0,sizeof(feedback_msg));
    sprintf(feedback_msg, stateChangeEvent,getDeviceUUID(),getEntityUUID(),currentSec(),publishTimeStamp,getSpid(),servName,attrName,state,getHUBMacID(),currentSec(),publishTimeStamp,getUnitUUID(),getGWUUID()/*getDeviceUUID()*/);
    
    
    memset(topic,0,sizeof(topic));
    sprintf(topic,sub_topic_m,getUnitUUID(),getDeviceUUID(),servName);
        
    ESP_LOGI(TAG,"\r Topic    -->>  %s",topic);
    ESP_LOGI(TAG,"\r Message  -->>  %s",feedback_msg);
    int msg_id = esp_mqtt_client_publish(client, topic, feedback_msg, 0, 0, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}


void MqttPublishMetering(float curr, float voltage, float power, float dayUnit, int dayTime ){

   //To-Do add from above function

    if(!isCommisioned()) return;

    char *stateChangeEvent = "{\"devices\":{\"Subscriber-Key\":\"\",\"deviceUUID\":\"%s\",\"entityUUID\":\"%s\",\"msgSequenceId\":%d100,\"serviceProviderId\":\"%s\",\"services\":{\"plugMetering\":{\"events\":{\"stateChanged\":{\"consumption\":\"%f\",\"power\":\"%f\",\"current\":\"%f\",\"voltage\":\"%f\",\"duration\":\"%d\"}},\"instanceId\":\"0\"}},\"srcDeviceId\":\"HSP_%s\",\"timestamp\":%d466,\"unitUUID\":\"%s\",\"userUUID\":\"%s\"}}";
    // char *stateChangeEvent = "{\"devices\":{\"Subscriber-Key\":\"\",\"deviceUUID\":\"%s\",\"entityUUID\":\"%s\",\"msgSequenceId\":%d100,\"serviceProviderId\":\"%s\",\"services\":{\"plugMetering\":{\"events\":{\"stateChanged\":{\"consumption\":%f,\"power\":%f,\"current\":%f,\"voltage\":%f}},\"instanceId\":\"0\"}},\"srcDeviceId\":\"HWB01_%s\",\"timestamp\":%d466,\"unitUUID\":\"%s\",\"userUUID\":\"%s\"}}";

    char* sub_topic_m = "/monitor/%s/%s/plugMetering";
    memset(feedback_msg,0,sizeof(feedback_msg));
    sprintf(feedback_msg, stateChangeEvent,getDeviceUUID(),getEntityUUID(),currentSec(),getSpid(),dayUnit,power,curr,voltage,dayTime,getHUBMacID(),currentSec(),getUnitUUID(),getGWUUID()/*getDeviceUUID()*/);


    memset(topic,0,sizeof(topic));
    sprintf(topic,sub_topic_m,getUnitUUID(),getDeviceUUID());

    ESP_LOGI(TAG,"\r Topic    -->>  %s",topic);
    ESP_LOGI(TAG,"\r Message  -->>  %s",feedback_msg);
    int msg_id = esp_mqtt_client_publish(client, topic, feedback_msg, 0, 0, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}

void publishWifiStatus()
{          
        publishTimeStamp++;
        // printf("\r\n wifiSettings    getWiFiUpdateStatus refresh ... \r\n");
        char *tmp = "{" \
        "\"devices\" : "\
        "{ "\
        "\"deviceUUID\":\"%s\", "\
        "\"entityUUID\":\"%s\", "\
        "\"serviceProviderId\":\"%s\", "\
        "\"msgSequenceId\":%d%d, "\
        "\"services\": "\
        "{"\
        "\"wifiSettings\": "\
        "{ "\
        "\"events\": "\
        "{ "\
        "\"stateChanged\": "\
        "{ "\
        "\"SSIDName\":\"%s\", "\
        "\"ipAddress\":\"%s\", "\
        "\"signalStrength\":\"%d\" "\
        "} "\
        "}, "\
        "\"instanceId\":\"0\" "\
        "}"\
        "},"\
        "\"srcDeviceId\":\"HSP_%s\", "\
        "\"timestamp\":%d%d, "\
        "\"unitUUID\":\"%s\", "\
        "\"userUUID\":\"%s\" "\
        "}} ";
        char wifi_set[650];
        memset(wifi_set,0,sizeof(wifi_set));

        char wifi_ssid[64];
        memset(wifi_ssid,0,sizeof(wifi_ssid));
        
        char *s = getSsid();
        int n = strlen(s);
        if(n>0){
            for(int i = 0;i < n ;i++){
                if(s[i] == '\\'){
                    wifi_ssid[i++] = '\\';
                    wifi_ssid[i++] = '\\';
                   n = n+2;
                }
                if(s[i] == '"'){
                    wifi_ssid[i++] = '\\';
                    wifi_ssid[i++] = '\"';
                   n = n+2;
                }
                wifi_ssid[i] = s[i];
            }
        }
        // printf("\n\nwifi_ssid - { %s }\n\n",wifi_ssid);
        sprintf(wifi_set,tmp,getDeviceUUID(),getEntityUUID(),getSpid(),currentSec(),publishTimeStamp,wifi_ssid/*getSsid()*/,getIPAddress(),getSignalStrength(),getHUBMacID(),currentSec(),publishTimeStamp,getUnitUUID(),getGWUUID(),getHUBMacID());
        //printf("\n\nwifi_set - { %s }\n\n",wifi_set);
        
        ESP_LOGI(TAG,"wifiSettings getWiFiUpdateStatus refresh ... \r\n %s",wifi_set);
        char* sub_topic_m = "/monitor/%s/%s/wifiSettings";
        char topic[128]={0};
        memset(topic,0,sizeof(topic));
        sprintf(topic,sub_topic_m,getUnitUUID(),getDeviceUUID());
        int msg_id = esp_mqtt_client_publish(client, topic, wifi_set, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        vTaskDelay(100/portTICK_PERIOD_MS);
	//getTurnONState() - Sachin implement it and check current state need to be published
        if(getTurnONState())
        {
           MQTTPublishMsg("lcSwitchControl", "power","on");
        }
        else
        {
           MQTTPublishMsg("lcSwitchControl", "power","off");
        }
        char * timerStarted = readNVData("timerStarted");
        ESP_LOGI(TAG,"Timer started [%s]", timerStarted);
        if(strcmp(timerStarted,"1") == 0)
        {
             vTaskDelay(100/portTICK_PERIOD_MS);
        MQTTPublishMsg("timerControl", "duration","0"); 
        writeNVData("timerStarted", "0");
        }
}

void init_mqtt(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_ERROR);
    esp_log_level_set("mqtt_client", ESP_LOG_INFO);    
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_ERROR);
    esp_log_level_set("TRANSPORT", ESP_LOG_ERROR);
    esp_log_level_set("OUTBOX", ESP_LOG_ERROR);

    mqtt_app_start();
}
