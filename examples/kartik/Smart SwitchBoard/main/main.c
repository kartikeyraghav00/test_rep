#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json_parse.h"
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "example_common_private.h"
#include "esp_ota_ops.h"
#include "esp_mac.h"
#include "esp_sleep.h"
//mdns or dns-sd

#include "driver/gpio.h"
#include "rom/gpio.h"
/////////// SPIFFS ////////////////
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
//////////////////////////////////

#include <stdlib.h>
#include <time.h>
#include "lwip/apps/sntp.h"
#include <esp_timer.h>

#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "ble_spp_server.h"
#include "driver/uart.h"
#include "mbedtls/aes.h"
#include "esp_bt.h"
#include "driver/gptimer.h"
//#include "freertos/timers.h"

extern char* getJson();
extern int init_mqtt();

static void timer_callback(void* arg);
esp_timer_handle_t timer_handler;

//bool Bp5758dSetChannels(uint16_t *cur_col_10);

#define LED_GPIO 6
#define BUTTON_GPIO 20

//Input pins for sensing the touch buttons
#define BUTTON_GPIO_1   0
#define BUTTON_GPIO_2   1
#define BUTTON_GPIO_3   2
#define BUTTON_GPIO_4   4

//Output GPIOs for Relay control
#define RELAY_1     7
#define RELAY_2     8
#define RELAY_3     9
#define RELAY_4     10

#define POWER_GPIO 4


static const char *TAG = "SmartPlug";

volatile int internet_connected = 0;

int com_data = 0;
volatile int is_commisioned = 0;
int spiffs_status = -1;
char buff[1300];
char mac_str[18]={0};
int r=0,i=0,g=0,b=0,c=0,w=0;
char buffer[200]; 
bool blinkingMode = false;
bool connectFromConfig = false;
char rbuff[100] = {0};
int countBraces = 0;
#define OUT_PUT_NUM 5
char *getIPAddress();
uint16_t control_notif_handle;
uint16_t conn_handle_notify = -1;
esp_timer_handle_t oneshot_timer;
#define defaultKey "NdRgUkXp2s5vByAD";

void startOTA(char * ota);



/*Addresable GPIO */
// max duty 1024 ( 0 to 1023) 
// 0 - 255 -> 256 value divide in interval 4  * r or g or b value
//void Bp5758dModuleSelected_init(bool value);
//void set_led_color(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white);
//void fadeLedColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white);
void MQTTPublishMsg(char *servName, char *attrName,char *state);
void startBLE(char * bleName);


//TaskHandle_t tcp_server_task_hdl = NULL;
//TaskHandle_t blink_task_hdl = NULL;
TaskHandle_t button_task_hdl  = NULL;
TaskHandle_t off_after_task_hdl  = NULL;
TaskHandle_t reset_task_hdl  = NULL;
TaskHandle_t schedule_task_hdl = NULL;
TaskHandle_t commissioning_task_hdl = NULL;
//TaskHandle_t sceneexexecute_task_hdl = NULL;
//TaskHandle_t fadeColor_task_hdl = NULL;
TaskHandle_t timerExecute_task_hdl = NULL;

int off_after_enabled = 0;
int timerExecute_task_hdl_active = 0;

 
void encryptA(char * plainText, unsigned char * outputBuffer){
 
  mbedtls_aes_context aes;
  char * key = "ABCDEFGHIJKLMNOP";
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)plainText, outputBuffer);
  mbedtls_aes_free( &aes );
}
 
void decryptA(unsigned char * chipherText, unsigned char * outputBuffer, char * key){
 
  mbedtls_aes_context aes;  
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_dec( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)chipherText, outputBuffer);
  mbedtls_aes_free( &aes );
}

//char *data_cmd = "53264BF44BBAAF73744E1285958AF2D2CE392E8B831C398A07989AC32D76D6A42694A27B4E6C4FBC2BC1ACC59309263F1FB16C95C0F4A8FF3ADAB479A97115137AD9C2C6B4A804F3E4F160E04C6F1E6A86FE212F0BDF8DD7316D069FC43510C0F6DD97F0ADA5AE14CC6B94920BF46F8D1CE2319A93D152366DEBFEE09CA9D710EBBC5DF7B840A4693A7409188816373ABAB17D7B611DA481FBA316BF286356E4CAE7B8523A179A8ED3DE454290F7485533E455721D0CDE6F6D3A46A38B873DC9A2E32767F7B8768F497373AA808EC19211398F45ED7D973773D0A41835387AE8FB5CA43E840ACEE698553BC7002A3F93F2E8152138EE4BA1E35BED7000108DE496B688C6B61B5A8BEB60BCD43EC56A21E961D9AFBE985574427443185C481DDA5729A5076F113CF38E44527CE44381BF";
//char *data_cmd = "6e25c2e1dc6c37d77a545df2648566581496f6233476516c787a74fec06bc9fc5a116aaf22b6f8cc48f4438af1d2ace29d6b27e31ea251c91b752d3fee39a7dde5703a7a6c54b6aae82ae5ed445ac4c385cf88215ec763de1c1f3c44dfc376a6dae352f41d5c9f2c74526c066536c91bae999ae6ca8158b85dca4afc13159a01bc36cf2ec51178971645e5df3f7214179fb47891ce046bec1e22952e9c80ab7dbc4f765aad2be3482b956ffb76fddcb10357a188dd096edc0e8bff9ba52304c6ce9ff3b372580dbcd5194";
uint8_t byte_data[1024] = {0x00};
char plain_arr[1024];
static int id = 0;//for plain text 

void append_plain(uint8_t *val){
    for(int i = 0;i<16;i++){
        plain_arr[id++] = (char)val[i];
    }
}

void char2byteArray2Plain(char *data, char *key){

    memset(byte_data,0x00,sizeof(byte_data));
    int size = strlen(data);
    int len = size/2;

    for(int i = 0,j=0;i < len;i++){
        sscanf(&data[i*2],"%02x",&byte_data[j++]);
    }
    //printf("\nbyte array - \n");
    int j = 0;
    
    uint8_t in[16];
    uint8_t out[16];
    memset(in,0x00,16);
    memset(out,0x00,16);
    for(int i = 0;i < len;i++){
        //printf("%02x ",byte_data[i]);
        if(i>0 && i%16 == 0){
            decryptA(in,out, key);
            memset(in,0x00,16);
            append_plain(out);
            memset(out,0x00,16);
        }
        in[i%16] = byte_data[i];
    }
    decryptA(in,out, key);//for last 16 bytes
    append_plain(out);
    printf("\n");
}

void printPlainText(){
    int sz = strlen(plain_arr);
    printf("\nprintPText\n");
    for(int i = 0;i< sz;i++){
        printf("%c",plain_arr[i]);
    }
    printf("\n");
}

void AESECB128_NOPADDING(char *data, char * plainTex, char * key){
  // printf("\nAESECB128_NOPADDING");
  memset(plain_arr,0,sizeof(plain_arr));
  id = 0;
  char2byteArray2Plain(data, key);
  strcpy(plainTex,plain_arr);
  //printPlainText();
}

bool fadestate = true;

int getInternetStatus(){
    return internet_connected;
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "0.in.pool.ntp.org");
    sntp_setservername(1, "1.in.pool.ntp.org");
    sntp_setservername(2, "2.in.pool.ntp.org");
    sntp_setservername(3, "3.in.pool.ntp.org");
    sntp_init();
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 1000;// -- ???? keep trying change later now try for 5 Min

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        printf("\nWaiting for system time to be set... (%d/%d)\n", retry, retry_count);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
        if(retry == 100)
        {
            setCurrentStatus(INTERNET_CONNECTED_FAILED);
        }
        else if(retry > 200)
        {
            setCurrentStatus(INTERNET_CONNECTED_FAILED);
        }
    }
    if(retry < retry_count){
        ESP_LOGI(TAG, "Internet Connected----------  %d  %d",retry,retry_count);
        internet_connected = 1;
         setCurrentStatus(INTERNET_CONNECTED);
    } else {
        ESP_LOGI(TAG, "Internet Not Connected----------  %d  %d",retry,retry_count);
        internet_connected = 0;
    }
}

void sntp_time_init(char* timezone)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
    }
    //https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    time(&now);
    ESP_LOGI(TAG, "The current date/time in seconds:=> %ld", now);
    setenv("TZ","IST-5:30",1);
    tzset();
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGE(TAG, "The current date/time error");
        //????? - retry
    } else {
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        //ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
        ESP_LOGI(TAG, "The current date/time is:=> %s", strftime_buf);
        internet_connected = 1;
    }
    ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());
}

char* getHUBMacID(){
    return mac_str;
}

long currentSec(void)
{
    time_t now = 0;
    if(internet_connected){//while
        time(&now);
        ESP_LOGI(TAG, "The current date/time is SYNCED:=> %ld",now);
    }
    return now;
}

int init_spiffs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return -1;
    }
    return 0;
}

void deinit_spiffs(){

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "SPIFFS unmounted");

}

char * readNVData(char *key)
{
    ESP_LOGI(TAG, "readNVData Entry -----\n");
    // printf("\nreadNVData Entry ----- key : %s\n",key);
    int err=0;
    memset(buffer, 0, sizeof(buffer));
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        // printf("\n=======> Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        size_t required_size;
        err = nvs_get_str(my_handle, key, NULL, &required_size );
        if(err != ESP_OK){
            printf("\n=======> Error (%s) getting nvs_get_str !\n", esp_err_to_name(err));
        }
        printf("\nnv req sz => %d\n",required_size);
        err = nvs_get_str(my_handle, key, (char *)&buffer, &required_size);
        if(err != ESP_OK){
            printf("\n=======> Error (%s) getting nvs_get_str !\n", esp_err_to_name(err));
        }
        printf("\nnv str buf => %s\n",buffer);

        switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "NV Data Read Buffer @@@@= %s\n\n", buffer);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGE(TAG, "The value is not initialized yet!\n");
            memset(buffer, 0, sizeof(buffer));
            break;
        default :
            ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
            break;
        }
    }
    nvs_close(my_handle);
    ESP_LOGI(TAG, "Read NVData -Exit----\n");
   return buffer;
}

void eraseNVPartition()
{
  ESP_LOGI(TAG, "eraseNVPartition Entry -----\n");
    int err=0;
    memset(buffer, 0, sizeof(buffer));
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGE(TAG, "Don't Erase NVS !!!\n");
            // ESP_ERROR_CHECK(nvs_flash_erase());
    }
    nvs_close(my_handle);
    ESP_LOGI(TAG, "eraseNVPartition -Exit----\n");

}

// void turnOnAfterBoot(bGPIO_INTR_ANYEDGEool fromReset)
// {
//     char* test = "lastKnownStatus";
//     char *nvData = readNVData(test);
//     ESP_LOGI(TAG, " NV Data on Boot ###########   %s",nvData);
//     uint16_t grays[7] = {0,0,0,0,0,1023,1023};
//     if(strlen(nvData) > 0)
//     {
//     char *token = strtok(nvData, ",");
//     int i = 0;
//     while( token != NULL ) {
//                     printf( " %s\n", token );
//                     grays[i] = atoi(token);
//                     token = strtok(NULL, ",");
//                     i++;
//                     }
//                 }
//     if(grays[0] == 1)
//     {
//         if(fromReset)
//         {
//         ESP_LOGI(TAG, "########### Dont Turn on its on DND Mode ");
//         char payload[100];
//         char* strUpdate="0,%d,%d,%d,%d,%d,%d";
//         memset(payload,0,sizeof(payload));
//         sprintf(payload, strUpdate,grays[1],grays[2],grays[3],grays[4],grays[5],grays[6]);
//         setLastKnownStatus(payload);
//         }
//         else
//         {
//             return;
//         }
//     }

//     fadeLedColor(grays[4],grays[3],grays[2],grays[5],grays[6]);
//     ESP_LOGI(TAG, "###########   Done Turn On After Boot");
//     setMostRecentColor(grays[4],grays[3],grays[2],grays[5],grays[6]);

// }

void factoryReset()
{
        ESP_LOGI(TAG, "\n\n\n\n config Unlink ........ \n");
        unlink("/spiffs/config.txt");
        ESP_LOGI(TAG, "\n reset Unlink ........ \n");
        unlink("/spiffs/reset.txt");
        ESP_LOGI(TAG, "\n settings Unlink ........ \n");
        unlink("/spiffs/settings.txt");
        ESP_LOGI(TAG, "\n dndProperty Unlink ........ \n");
        unlink("/spiffs/dndProperty.txt");
        ESP_LOGI(TAG, "\n erasing SPIFFS........ \n");
        esp_spiffs_format(NULL);
        ESP_LOGI(TAG, "\n eraseNVPartition Unlink ........ \n");
        //eraseNVPartition();// not erasing as we are storing calibration values in case of Plug
        writeNVData("day_0","");
        writeNVData("day_1","");
        writeNVData("day_1970","");
        writeNVData("ssid", "");
        writeNVData("pass", "");
        writeNVData("svVersion","");
        writeNVData("dateChanged", "0");
        writeNVData("meteringRest", "0");
        setLastKnownStatus("on");//refreshing default value to ON
        vTaskDelay(100 / portTICK_PERIOD_MS);
        // reset nvm also as wifi config is saved there or disable it and save in config file
        esp_restart();
}

static void led_blink_plug(void *pv){
    while(1){// take handle and terminate thread if necessasy
        gpio_set_level(LED_GPIO,0);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
	vTaskDelete(NULL);
}


static void reset_task(void *pvParameters){

    int r_count=0;

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG,"Enter Reset Task");
    nvs_handle_t my_handle;
    int err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG,"Done\n");
        }

        err = nvs_set_i32(my_handle, "restart_counter", 0);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    vTaskDelete(NULL);
}


// void loadSavedSettings()
// {
//     size_t bytCount=0;
//      FILE *fin = fopen("/spiffs/settings.txt", "r");
//         if (fin == NULL) {
//             ESP_LOGE(TAG, "Failed to open config.txt file for reading");
//             return;
//         }

//         char block[500];
//         while (! feof (fin)) {
//             bytCount = fread (block, 1, sizeof(block), fin);//(void *ptr, size_t size, size_t nmemb, FILE *stream);
//             ESP_LOGI(TAG, "Read from config file: bytes : %d\n",bytCount);
//         }
//         printf("Loaded Settings ----  %s",block);
//         parseSettings(block);
//         //ESP_LOGI(TAG, "Read from config file: '%s' \n", block);      

//         fclose(fin);  
// }

// void saveSettings()
// {
//     struct stat st;
//       if(stat("/spiffs/settings.txt", &st) == 0) unlink("/spiffs/settings.txt");          
//         FILE *fin = fopen("/spiffs/settings.txt", "w");
//         if (fin == NULL) {
//             ESP_LOGE(TAG, "Failed to open config.txt file for reading");
//             return;
//         }         
//         char payload[200];
//         char* settingsData="{\"lastKnowStatus\" : %d,\"customColor\" : \"%s\"}";
//         memset(payload,0,sizeof(payload));
//         char *temp = "%d,%d,%s";
//         char nvData[128]={0};                   
//         memset(nvData,0,sizeof(nvData));
//         if(getLastKnownProperty() == 0)
//         {            
//             sprintf(payload, settingsData,getLastKnownProperty(),"0,0,0,1023,1023");
//             sprintf(nvData, temp,getDndStatus(),getLastKnownProperty(),"0,0,0,1023,1023");
//             printf("Setting Default Property Color--- %s", nvData);
//         }
//         else if(getLastKnownProperty() == 1)
//         {
//             sprintf(payload, settingsData,getLastKnownProperty(),getMostRecentColor());
//             sprintf(nvData, temp,getDndStatus(),getLastKnownProperty(),getMostRecentColor());
//             printf("Setting LastKnown Property Color--- %s", nvData);
//         }
//         else if(getLastKnownProperty() == 2)
//         {
//             sprintf(payload, settingsData,getLastKnownProperty(),getCustomColor());
//             sprintf(nvData, temp,getDndStatus(),getLastKnownProperty(),getCustomColor());
//             printf("Setting Custom Property Color--- %s", nvData);
//         }
//         fprintf(fin, "%s",payload);
//         fclose(fin);
//         printf("Setting NV Data for reboot Color---  %s",nvData);
//         setLastKnownStatus(nvData);
// }

static void rest_Call_task(void *pvParameters){

    ESP_LOGI(TAG,"Starting Rest Call Task  ");

    rest_call(GET_OTA_URLV2);
    //startOTA("");

    vTaskDelete(NULL);
}

void startRestCallTask()
{
    ESP_LOGI(TAG,"Request Rest Call Task  ");
    xTaskCreate(rest_Call_task, "rest_Call_task", 8192, NULL, 5, NULL);
}

static void versionUpdate_task(void *pvParameters){

    ESP_LOGI(TAG,"Starting versionUpdate_task  ");

    if(isinventoryUpdateReq())
    {
        ESP_LOGI(TAG,"INVENTORY Version Updating In Cloud");
        rest_call(UPDATEVERSION);
    }

    vTaskDelete(NULL);
}

static void otaTask(void *pvParameters){

    ESP_LOGI(TAG,"Starting OTA Call Task  ");
    int counter = 0;
    vTaskDelay(20000 /portTICK_PERIOD_MS);    
    xTaskCreate(versionUpdate_task, "versionUpdate_task", 8192, NULL, 5, NULL);    
    /*while(counter < 60)
    {
        vTaskDelay(1000 /portTICK_PERIOD_MS);
        counter++;
    }  
    startRestCallTask(); */
    vTaskDelete(NULL);
}

static void ble_Control_task(void *pvParameters){

    ESP_LOGI(TAG,"Starting BLE Control Task  ");
    while(1)
    {
        char *cfg = getJson();
        // printf("\nbl cfg :: %s\n",cfg);
        parseBleControlCommand(cfg);
        //printf("\n%s\n",cfg);
        vTaskDelay(100 /portTICK_PERIOD_MS);    
    }    


    vTaskDelete(NULL);
}

void connectWifi()
{
         wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
        },
    };
    char ssid[100]={};
    char password[100]={};

    memset(ssid,0,sizeof(ssid));
    memset(password,0,sizeof(password));

    char * tmpssid = readNVData("ssid");

    strncpy(ssid,tmpssid,strlen(tmpssid));

    char * tmppass = readNVData("pass");

    strncpy(password,tmppass, strlen(tmppass));

    ESP_LOGI(TAG, " SSID on Boot ###########   %s",ssid);
    ESP_LOGI(TAG, " PASS on Boot ###########   %s",password);
    // printf("\nSSID on Boot NV ###########   %s\n",ssid);
    // printf("\nPASS on Boot NV ###########   %s\n",password);
    if((strlen(ssid)) > 0)
    {
        ESP_LOGI(TAG, " Connecting Using NV Data ###########   %s",ssid);
        // printf("\nConnecting Using CNV Data ###########   %s\n",ssid);
        memcpy(wifi_config.sta.ssid,ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        setSsid(ssid);
        setPwd(password);
        if(strlen(password) > 0)
        {
            ESP_LOGI(TAG, "Password[%s]",password);
            // printf("\nPassword[%s]\n",password);

            wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
        }
        else
        {
            //ESP_LOGI(TAG, "Password[%s]",password);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        }
        example_wifi_sta_do_connect(wifi_config, true);
    }
    else if(strlen(getSsid())>0){
        ESP_LOGI(TAG, " Connecting Using Config File ###########   %s",ssid);
        // printf("\nConnecting Using Config File ###########   %s\n",ssid);
        memcpy(wifi_config.sta.ssid, getSsid(), sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, getPwd(), sizeof(wifi_config.sta.password));
    	if(strlen(getPwd()) > 0)
    	{
    		ESP_LOGI(TAG, "Password[%s]",getPwd());
    		wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
    	}
        else
    	{
    		//ESP_LOGI(TAG, "Password[%s]",getPwd());
    		wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    	}
        example_wifi_sta_do_connect(wifi_config, true);
    }

    ESP_LOGI(TAG, " Connecting ###############################   [%s]",getIPAddress());
}

void connectWifiCommision(char *s, char *p)
{
         wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
        },
    };
    char ssid[100]={};
    char password[100]={};

    memset(ssid,0,sizeof(ssid));
    memset(password,0,sizeof(password));

    char * tmpssid = readNVData("ssid");

    strncpy(ssid,tmpssid,strlen(tmpssid));

    char * tmppass = readNVData("pass");

    strncpy(password,tmppass, strlen(tmppass));

    ESP_LOGI(TAG, "\nSSID on Boot ###########   %s\n",ssid);
    ESP_LOGI(TAG, "\nPASS on Boot ###########   %s\n",password);
    // printf("\nSSID on Boot connectWifiCommision NV ###########   %s\n",ssid);
    // printf("\nPASS on Boot connectWifiCommision NV ###########   %s\n",password);
    if(s != NULL && p != NULL)
    {
        ESP_LOGI(TAG, " Connecting Using changed ssid ###########   %s",s);
        printf("\nConnecting Using changed password ###########   %s\n",p);
        // printf("\ns[%s]\n",s);
        // printf("\np[%s]\n",p);
        memcpy(wifi_config.sta.ssid,s, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, p, sizeof(wifi_config.sta.password)); 
        if(strlen(p) > 0)
        {
            ESP_LOGI(TAG, "p[%s]",p);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
        }
        else
        {
            //ESP_LOGI(TAG, "Password[%s]",p);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        }
    }
    else if((strlen(ssid)) > 0)
    {
        ESP_LOGI(TAG, " Connecting Using NV Data ###########   %s",ssid);
        // printf("\nConnecting Using CNV Data ###########   %s\n",ssid);
        memcpy(wifi_config.sta.ssid,ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
      setSsid(ssid);
      setPwd(password);
        if(strlen(password) > 0)
        {
            ESP_LOGI(TAG, "Password[%s]",password);
            // printf("\nPassword[%s]\n",password);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
        }
        else
        {
            //ESP_LOGI(TAG, "Password[%s]",password);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        }
    }
    else if(strlen(getSsid())>0){
        //ESP_LOGI(TAG, " Connecting Using Config File ###########   %s",ssid);
        // printf("\nConnecting Using Config File ########### => %s\n",getSsid());
        memcpy(wifi_config.sta.ssid, getSsid(), sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, getPwd(), sizeof(wifi_config.sta.password));
        if(strlen(getPwd()) > 0)
        {
            ESP_LOGI(TAG, "Password[%s]",getPwd());
            // printf("\nPassword-[%s]\n",getPwd());

            wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
        }
        else
        {
            //ESP_LOGI(TAG, "Password[%s]",getPwd());
            wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        }
    }
    example_wifi_sta_do_connect(wifi_config, false);
    ESP_LOGI(TAG, " Connecting ###############################   [%s]",getIPAddress());
    int countConnection = 0;
    while(strlen(getIPAddress()) <= 0)
    {
        ESP_LOGI(TAG, " Waiting For IP ###############################   ");
        // printf("\nWaiting For IP ###############################   \n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        countConnection++;
        if(countConnection > 15)
        {
            setCurrentStatus(WIFI_SSID_PASSWORD_ERROR);
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            esp_restart();
        }
    }
    setCurrentStatus(WIFI_CONNECTED);

    if(sntp_enabled()){
        sntp_stop();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    sntp_time_init(NULL);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    setCurrentStatus(INTERNET_CONNECTED);
    if(s != NULL && p != NULL){
        writeNVData("ssid", s);
        writeNVData("pass", p);
        esp_restart();
    }
}

void startReCommision()
{
    ESP_LOGI(TAG, "Start ReCommision ---------------- ");

    vTaskDelay(1000/ portTICK_PERIOD_MS);
    rest_call(GETSCHEDULES);
    ESP_LOGI(TAG, "downloadedRecords ---------------- [%d]",getdownloadedRecords());
    ESP_LOGI(TAG, "TotalRecords      ---------------- [%d]",getTotalRecords());
    while(getdownloadedRecords() <= getTotalRecords())
    {
        ESP_LOGI(TAG, "downloadedRecords ---------------- [%d]",getdownloadedRecords());
        ESP_LOGI(TAG, "TotalRecords      ---------------- [%d]",getTotalRecords());
        vTaskDelay(1000/ portTICK_PERIOD_MS);
        rest_call(GETSCHEDULES);
    }
    writeNVData("rccommission","0");
    esp_restart();
    ESP_LOGI(TAG, "Complete ReCommision ---------------- ");

}

void init_verification();
void PowerMonitorFunction(void);
int getVerificationRouterStatus();
void init_unit_calculation();
static void commissioning_task(void *pvParameters)
{

    struct stat st;
    size_t bytCount=0;
    //ble_gatt_init();

    if(stat("/spiffs/config.txt", &st) == 0 && st.st_size > 100){//commissioned
        is_commisioned = 1;
        ESP_LOGI(TAG, "commissioned config file: size : %ld\n",st.st_size);
        FILE *fin = fopen("/spiffs/config.txt", "r");
        if (fin == NULL) {
            ESP_LOGE(TAG, "Failed to open config.txt file for reading");
            return;
        }

        char block[1500];
        while (! feof (fin)) {
            bytCount = fread (block, 1, sizeof(block), fin);//(void *ptr, size_t size, size_t nmemb, FILE *stream);
            ESP_LOGI(TAG, "Read from config file: bytes : %d\n",bytCount);
        }
        //printf("\n%s\n",block);
        if(stat("/spiffs/atok.txt", &st) == 0 && st.st_size > 50){
            parseConf(block,0);
            setAccessToken(NULL);//load
        } else 
            parseConf(block,1);
        fclose(fin);

        //setAccessToken(NULL);//load from file

    } else { //not commissioned
        if(stat("/spiffs/config.txt", &st) == 0) unlink("/spiffs/config.txt");//if empty or corrupted file present


        uint8_t mac[6];
        char   *blename;
        esp_read_mac(mac, ESP_IF_WIFI_STA);
        if (-1 == asprintf(&blename, "QUBO_P_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])) {
            abort();
        }
        startBLE(blename);
        setCurrentStatus(COMMISION_READY);
         //blinkingMode = true;
        // if(getVerificationRouterStatus())
        //     vTaskDelay(10*1000 / portTICK_PERIOD_MS);
        xTaskCreate(led_blink_plug, "led_blink_plug", 512, NULL, 5, NULL);
        char *cfg = getJson();
        int status = parseConf(cfg,1);

        if(status == -1)
        {
            setCurrentStatus(CONFIG_PARSING_ERROR);
            vTaskDelay(3000/portTICK_PERIOD_MS);
            esp_restart();
        }
        //printf("\n%s\n",cfg);
        setCurrentStatus(CONFIG_RECIEVED);
        if(strlen(getSsid()) > 0)
        {
            example_wifi_start();
            connectWifiCommision(NULL,NULL);
        }
        else
        {
            setCurrentStatus(INTERNET_CONNECTED);
        }

        vTaskDelay(3000/portTICK_PERIOD_MS);
        // First create a file.
        ESP_LOGI(TAG, "Opening config file");
        FILE* f = fopen("/spiffs/config.txt", "w");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open config file for writing");
            return;
        }
        bytCount = fwrite (cfg, 1, 1500, f);
        fclose(f);
        ESP_LOGI(TAG, "written bytes : %d\n", bytCount);

        if(getRecommisionStatus() == 1)
        {
            ESP_LOGI(TAG, "Recommsioned ################################ : %d", getRecommisionStatus());
            writeNVData("rccommission", "1");
        }
        esp_restart();
    }
    uint8_t mac[6];
    char   *blename;
    esp_read_mac(mac, ESP_IF_WIFI_STA);
    if (-1 == asprintf(&blename, "*QBO_P_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])) {
        abort();
    }
    init_unit_calculation();
    startBLE(blename);
    xTaskCreate(ble_Control_task, "ble_Control_task", 3072, NULL, 5, NULL);
    example_wifi_start(); //will be done from verification task ....
    connectWifi();

    if(strlen(getIPAddress()) > 0)
    {
      ESP_LOGI(TAG, "GOt IP address ################################ : [%d][%s]",strlen(getIPAddress()), getIPAddress());  
    //esp_ble_tx_power_set_enhanced(ESP_BLE_ENHANCED_PWR_TYPE_DEFAULT, 0, ESP_PWR_LVL_P3);

    //esp_wifi_set_ps(WIFI_PS_NONE);
    char * tzVal = readNVData("tzVal");
    if((strlen(tzVal)) > 1)
    {
        setTimeZone(tzVal);
    }
    else
    {
        setTimeZone("+05:30");
    }
    sntp_time_init(NULL);

    int commission_state = getCommissionState();
    if(commission_state == 0 || commission_state == -1){
        ESP_LOGI(TAG,"\r\nCommissioned\r\n");

        if(strcmp(readNVData("rccommission"),"1") ==0)
        {
           ESP_LOGI(TAG,"##################### Download Here %s",readNVData("rccommission"));
           setCurrentStatus(RECOMMISION_DATA);
           startReCommision();

        }
        init_mqtt();
        //executeMqttpublishTask();
    } else if(commission_state == 3) {// not commissioned state == 3
        ESP_LOGI(TAG,"\r\n Not commissioned state = 3\r\n");
        //factory Reset Here format partition
       factoryReset();
    } 
    }   

    //loadSavedSettings(); 
    //executeMqttpublishTask();
    if(isinventoryUpdateReq())
    {
        xTaskCreate(otaTask, "otaTask", 2048, NULL, 5, NULL);
    }  
    
    vTaskDelete(NULL);
}

int isCommisioned(){
    return is_commisioned;
}

// static void sceneExecute_task(void *pvParameters)
// {
//     ESP_LOGI(TAG,"\r\n Executing Task FOr Scene\r\n");
//     ESP_LOGI(TAG,"Scene Data -Main-----------%s",(char*)pvParameters);
//     cJSON_Hooks memoryHook;
//     //printf("\nparseMqttJson:: %s\n",jsonbuff);
//     memoryHook.malloc_fn = malloc;
//     memoryHook.free_fn = free;
//     cJSON_InitHooks(&memoryHook);    
//     cJSON *root = cJSON_Parse((char*)pvParameters);
//     if(root != NULL){
//         cJSON *command = NULL;
//         cJSON *devices = NULL;
//         cJSON *services = NULL;              
//         cJSON *sceneControl = NULL;
//         cJSON *attributeName = NULL;        
//         cJSON *parameters = NULL;
//         command = cJSON_GetObjectItem(root, "command");
//         if(command != NULL){
//             devices = cJSON_GetObjectItem(command, "devices");
//         }
//         if(devices != NULL){
//             //lock deviceUUID get and use to fetch mpin pass1 macid
//             if(cJSON_GetObjectItem(devices, "deviceUUID") != NULL){
//                 char *tmp = cJSON_GetObjectItem(devices,"deviceUUID")->valuestring;
//                 printf("\nlduid : %s \r\n",tmp);
//             }
//             services = cJSON_GetObjectItem(devices, "services");            
//         }else{
//             ESP_LOGI(TAG,"devices is NULL:");
//         }
//         if(services != NULL){            
//             sceneControl  = cJSON_GetObjectItem(services, "sceneControl");
//             ESP_LOGI(TAG,"==>>services parsed:");
//         }       
//         else{
//             ESP_LOGI(TAG,"n==>>services is NULL:");
//         }        
//         if(sceneControl != NULL)
//         {
//             attributeName = cJSON_GetObjectItem(sceneControl, "commands");
//             ESP_LOGI(TAG,"==>>services is sceneControl:");
            
//         }
//         else{
//             ESP_LOGI(TAG,"==>>lockControl is NULL:");
//         }           
//         if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "action") != NULL)         
//             {
//                 parameters = cJSON_GetObjectItem(attributeName, "action");                
    
//     char *flash_mode = cJSON_GetObjectItem(parameters, "mode")->valuestring;
//     char *flash_speed = cJSON_GetObjectItem(parameters, "timeDelay")->valuestring;
//     cJSON *colors = cJSON_GetObjectItem(parameters, "colors");

    
//     ESP_LOGI(TAG,"Scene flash_mode => %s ", flash_mode);
//     ESP_LOGI(TAG,"Scene flash_speed => %s ", flash_speed);    

//     int colors_count = cJSON_GetArraySize(colors);
//     ESP_LOGI(TAG,"\nchunks_count => %d \n", colors_count);
//     int timedelay = atoi(flash_speed); 
//     if(timedelay <= 0)
//     {
//         timedelay = 1;
//     }   
//    ESP_LOGI(TAG,"\nTime Delay => %d \n", timedelay);
//     //scene1.no_of_colors = colors_count;    
//     int colorSet[colors_count][5];
//     for(int i = 0; i < colors_count; i++){
//         char *color = cJSON_GetArrayItem(colors, i)->valuestring;
//         ESP_LOGI(TAG,"red => %s\n",color);  
//         colorSet[i][0] = atoi(strtok(color,","));
//         colorSet[i][1] = atoi(strtok(NULL,","));
//         colorSet[i][2] = atoi(strtok(NULL,","));
//         colorSet[i][3] = atoi(strtok(NULL,","));
//         colorSet[i][4] = atoi(strtok(NULL,","));
//         //set_led_color(blue,green,red,cool,warm);
//         ESP_LOGI(TAG,"colors ----- %d - %d - %d -%d - %d \n\n",colorSet[i][2],colorSet[i][1],colorSet[i][0],colorSet[i][3],colorSet[i][4]);
//     }
//     int counter = 0;
//     while(true)
//     {
//         if(strcmp(flash_mode,"flash") == 0)
//         {
//             ESP_LOGI(TAG,"Changing Color in Flash Mode ");
//             set_led_color(colorSet[counter][2],colorSet[counter][1],colorSet[counter][0],colorSet[counter][3],colorSet[counter][4]);
//         }
//         else if(strcmp(flash_mode,"fade") == 0)
//         {
//             ESP_LOGI(TAG,"Changing Color in Fade Mode");
//             fadeLedColor(colorSet[counter][2],colorSet[counter][1],colorSet[counter][0],colorSet[counter][3],colorSet[counter][4]);
//         }
//         else
//         {
//              ESP_LOGI(TAG,"Changing Color in Default Flash");
//             set_led_color(colorSet[counter][2],colorSet[counter][1],colorSet[counter][0],colorSet[counter][3],colorSet[counter][4]);
//         }
//         counter++;
//         if(counter > colors_count-1)
//         {
//             counter = 0;
//         }
//         vTaskDelay(timedelay * 1000 / portTICK_PERIOD_MS);
//     }
    
// }           
        
//         // free the memory!
//         cJSON_Delete(root);
//         setJsonParseStatus(1);
//     } else {
//         printf("\nparseResponseJson root is NULL\n");
//         setJsonParseStatus(2);//json parse fails
//     }
//     printf("\nparseMqttJson Exit\n");
//    vTaskDelete(sceneexexecute_task_hdl);
//    sceneexexecute_task_hdl = NULL;
// }

// void executeScene(char *sceneData)
// {
//     xTaskCreate(sceneExecute_task, "sceneExecute_task", 2048, (void*)sceneData, 5, &sceneexexecute_task_hdl);
// }

// void removeSceneTask()
// {
//     if(sceneexexecute_task_hdl != NULL)
//     {
//         ESP_LOGI(TAG,"\nDeleting Task\n");
//    vTaskDelete(sceneexexecute_task_hdl);
//    sceneexexecute_task_hdl = NULL;
//     }
//     else
//     {
//         ESP_LOGI(TAG,"\nNot Deleting sceneexexecute_task_hdl coz its null\n");
//     }

// }

void turnOffPlug(int viaMqtt, uint8_t numb);
void turnOnPlug(int viaMqtt, uint8_t numb);
int getCount();
int getPowerMonitoringStatus();

static void timerExecute_task(void *pvParameters)
{
    writeNVData("timerStarted", "1");
    ESP_LOGI(TAG,"\r\n Executing Task FOr timerExecute_task\r\n");
    ESP_LOGI(TAG,"timerExecute_task Data -Main-----------%s",(char*)pvParameters);
    timerExecute_task_hdl_active = 1;
    cJSON_Hooks memoryHook;
    //printf("\nparseMqttJson:: %s\n",jsonbuff);
    memoryHook.malloc_fn = malloc;
    memoryHook.free_fn = free;
    cJSON_InitHooks(&memoryHook);
    ESP_LOGI(TAG,"\nparseMqttJson Entry\n");
    cJSON *root = cJSON_Parse((char*)pvParameters);
    if(root != NULL){
        cJSON *command = NULL;
        cJSON *devices = NULL;
        cJSON *services = NULL;              
        cJSON *timerControl = NULL;
        cJSON *attributeName = NULL;        
        cJSON *parameters = NULL;
        command = cJSON_GetObjectItem(root, "command");
        if(command != NULL){
            devices = cJSON_GetObjectItem(command, "devices");
        }
        if(devices != NULL){
            //lock deviceUUID get and use to fetch mpin pass1 macid
            if(cJSON_GetObjectItem(devices, "deviceUUID") != NULL){
                char *tmp = cJSON_GetObjectItem(devices,"deviceUUID")->valuestring;
                ESP_LOGI(TAG,"\nlduid : %s \r\n",tmp);
            }
            services = cJSON_GetObjectItem(devices, "services");            
        }else{
            ESP_LOGI(TAG,"\n\n==>>devices is NULL:\n\n");
        }
        if(services != NULL){            
            timerControl  = cJSON_GetObjectItem(services, "timerControl");
            ESP_LOGI(TAG,"\n\n==>>services parsed:\n\n");
        }       
        else{
            ESP_LOGI(TAG,"\n\n==>>services is NULL:\n\n");
        }        
        if(timerControl != NULL)
        {
            attributeName = cJSON_GetObjectItem(timerControl, "attributes");
            ESP_LOGI(TAG,"\n\n==>>services is timerControl:\n\n");
            
        }
        else{
            ESP_LOGI(TAG,"\n\n==>>timerControl is NULL:\n\n");
        }           
        if (attributeName != NULL && cJSON_GetObjectItem(attributeName, "duration") != NULL)         
            {
                cJSON *dur = cJSON_GetObjectItem(attributeName, "duration");
                int duration = 0;                   
                if(cJSON_IsNumber(dur))
                {
                    
                    duration = cJSON_GetObjectItem(attributeName, "duration")->valueint;                   
                    ESP_LOGI(TAG,"\n Its a numberDuration => %d \n", duration);
                }
                else
                {                                        
                    duration = atoi(cJSON_GetObjectItem(attributeName, "duration")->valuestring);                      
                    ESP_LOGI(TAG,"\n Its not numberDuration => %d \n", duration);
                }
                
                ESP_LOGI(TAG,"\n Duration => %d \n", duration);
                if(duration > 0)
                {
                int counter = 0;
                while(counter < duration)
                {
                    printf("\n Time Passed => %d / %d \n",counter, duration);
                    counter++;
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }

                //fadeLedColor(0,0,0,0,0);
            //     struct stat st;
            //     if(stat("/spiffs/dndProperty.txt", &st) == 0)
            //     {
            //     char *temp = "%d,%d,%s";
            //     char nvData[128]={0};
            //     memset(nvData,0,sizeof(nvData));
            //     // if(getLastKnownProperty() == 1)
            //     // {
            //     // sprintf(nvData, temp,1,getLastKnownProperty(),getMostRecentColor());
            //     // }
            //     // else if((getLastKnownProperty() == 2))
            //     // {
            //     // sprintf(nvData, temp,1,getLastKnownProperty(),getCustomColor());
            //     // }
            //     // setLastKnownStatus(nvData);
            // }
//???????????????????? if ON then start Counter .......................????????????????????????????????????????
            //turnOffPlug(0);
	    }	
            // MQTTPublishMsg("lcSwitchControl", "power","off"); // off if on or on if off   ????
        }   
        
        
        // free the memory!
        cJSON_Delete(root);
        setJsonParseStatus(1);
    }
     else {
        ESP_LOGI(TAG,"\nparseResponseJson root is NULL\n");
        setJsonParseStatus(2);//json parse fails
    }
    ESP_LOGI(TAG,"\nparseMqttJson Exit\n");
    timerExecute_task_hdl_active = 0;
   vTaskDelete(timerExecute_task_hdl);
   timerExecute_task_hdl = NULL;
}

void executeTimerTask(char *sceneData)
{
    xTaskCreate(timerExecute_task, "timerExecute_task", 2048, (void*)sceneData, 5, &timerExecute_task_hdl);
}

void removeTimerTask()
{
    if(timerExecute_task_hdl_active == 1)
    {
        ESP_LOGI(TAG, "\nDeleting Timer Task\n");
        vTaskDelete(timerExecute_task_hdl);
        timerExecute_task_hdl = NULL;
        timerExecute_task_hdl_active = 0;
    }
    else
    {
        ESP_LOGI(TAG,"\nNot Deleting Timer Task coz its null\n");
    }

}

// void sceneFadeColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white)
// {

//     ESP_LOGI(TAG,"\r\n Executing Task FOr fadeColor_task_hdl\r\n");

//   uint16_t grays[5];  
//   memset(grays, 0, sizeof(grays));
//   uint16_t color[5];   
//   memset(color, 0, sizeof(color));
//   color[2]=r;
//   color[1]=g;
//   color[0]=b;
//   color[4]=c;
//   color[3]=w;  

//  grays[0] = blue;
//   grays[1] = green;
//   grays[2] = red;
//   grays[3] = warm_white;
//   grays[4] = cool_white;

//   for(int i = 0; i < 5; i++)
//   {
//     if(grays[i] == 255)
//     {
//       grays[i] = 1023;
//     }
//     else
//     {
//       grays[i] = grays[i] * 4;
//     }    
//   }    

//   for(int i =0 ; i < 5; i++)
//   {
//     for(int j =0 ; j < 5; j++)
//     {        
//      /* printf("\n\ncolor[%d]     %d",j,color[j]);
//       printf("\n\ngrays[%d]     %d",j,grays[j]);*/
//       if(color[j] < grays[j])
//       {
//       color[j] = color[j] + 200;
//       }        
//       else if(color[j] > grays[j])
//       {
//         color[j] = color[j] - 200;
//       }
//       if(color[j] <= 0)
//       {
//         color[j] = 0;
//       }
//       if(color[j] > 1023)
//       {
//         color[j] = 1023;
//       }  
//     }
//   r=color[2];
//   g=color[1];
//   b=color[0];
//   c=color[4];
//   w=color[3];
//   if(color[0] == grays[0] && color[1] == grays[1] && color[2] == grays[2] && color[3] == grays[3] && color[4] == grays[4] && color[4] == grays[4])
//   {
//     ESP_LOGI(TAG,"\n\n\n\n  Breaking the loop ####################");
//     break;
//   }

//     Bp5758dSetChannels(color);
//     vTaskDelay(100 / portTICK_PERIOD_MS);
//   }

  

//   Bp5758dSetChannels(grays);
//   r=color[2];
//   g=color[1];
//   b=color[0];
//   c=color[4];
//   w=color[3];
//   ESP_LOGI(TAG,"\fadeColor_task Exit\n");
  
// }

// static void fadeColor_task(uint16_t *pvParameters)
// {
//   ESP_LOGI(TAG,"Executing Task FOr fadeColor_task_hdl");

//   uint16_t *destination = (uint16_t *)pvParameters;  
//   uint16_t grays[5];  
//   memset(grays, 0, sizeof(grays));
//   int source[5];   
//   source[2]=r;
//   source[1]=g;
//   source[0]=b;
//   source[4]=c;
//   source[3]=w;   
//   int diff[5] = {0,0,0,0,0};  
  
//   uint16_t steps = 8;
//   uint16_t fadeTimer = 300; 
  
//   for(int i =0 ;i < 5; i++)
//   {
//         diff[i] = destination[i] - source[i];        
//         diff[i] = diff[i]/(steps-1);;        
//         //printf("***********      %d  ",diff[i]);
//   }
//   //printf("\n");
//   for(int i =0 ;i < steps-1; i++)
//   {
//     for(int j = 0; j < 5; j++)
//     {
//       source[j] = source[j] + diff[j];
//       ESP_LOGI(TAG,"***********     %d  ",source[j]);   
//       grays[j] = (uint16_t)source[j];
//     } 
//     if(!fadestate)    
//     {
//         break;
//     }
//   Bp5758dSetChannels(grays);    
//   vTaskDelay((fadeTimer/steps) / portTICK_PERIOD_MS);
//   //printf("\n");
//   }
//   Bp5758dSetChannels(destination);    
  
//   r=destination[2];
//   g=destination[1];
//   b=destination[0];
//   c=destination[4];
//   w=destination[3];
//     ESP_LOGI(TAG,"fadeColor_task Exit");
//    vTaskDelete(NULL);
//    fadeColor_task_hdl = NULL;
//   }

// void setLedColorTask(uint16_t *fadeData)
// {
    
//     if(fadeColor_task_hdl != NULL)
//     {
//         ESP_LOGI(TAG,"Deleting Task");
//         vTaskDelay(100/portTICK_PERIOD_MS);
//         vTaskDelete(fadeColor_task_hdl);
//         fadeColor_task_hdl = NULL;        
//     }
//     else
//     {        
//         printf("\nNot Deleting fadeColor_task_hdl coz its null\n");

//     }       
//     xTaskCreate(fadeColor_task, "fadeColor_task", 2048, fadeData, 2, &fadeColor_task_hdl);
// }
// uint16_t grays[5];
// void fadeLedColor(uint16_t blue,uint16_t green,uint16_t red,uint16_t warm_white, uint16_t cool_white)
// {
//   ESP_LOGI(TAG,"fadeLedColor Enter ----- %d - %d - %d -%d - %d \n\n",blue,green,red,warm_white,cool_white);  
//   if(blue > 0 || green > 0 || red > 0)
//   {
//     c = 0;
//     w = 0;
//   }
//   else if(warm_white > 0 || cool_white > 0)
//   {
//     r = 0;
//     g = 0;
//     b = 0;
//   }  
//   memset(grays, 0, sizeof(grays));
//   uint16_t color[5];   
//   memset(color, 0, sizeof(color));
//   color[2]=r;
//   color[1]=g;
//   color[0]=b;
//   color[4]=c;
//   color[3]=w;
//   grays[0] = blue;
//   grays[1] = green;
//   grays[2] = red;
//   grays[3] = warm_white;
//   grays[4] = cool_white;

//   for(int i = 0; i < 5; i++)
//   {
//     if(grays[i] == 255)
//     {
//       grays[i] = 1023;
//     }    
//     else
//     {
//       grays[i] = grays[i] * 4;
//     }  
//     if(grays[i] > 1023)
//     {
//         grays[i] = 1023;
//     }  
//   } 

//    for(int i = 0; i < 5; i++)
//   {
//     if(grays[i] > 1023)
//     {
//       grays[i] = 1023;
//     }    
//   }    
//   setLedColorTask(grays);
//   printf("fadeLedColor EXit -----");
// }

int writeNVData(char *key, char *value)
{
    memset(buffer, 0, sizeof(buffer));

    int err = 0;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    ESP_LOGI(TAG,"Adding text to NVS ... ");
    memset(buffer, 0 ,sizeof(buffer));
    strcpy(buffer, value);
    printf("\nNV K : %s \n",key);
    if(strlen(value) > 0){
        ESP_LOGI(TAG,"writeNVData ---- %s  -- %s",key,value);
        printf("\nNV K:%s  -- V:%s\n",key,value);
        ESP_LOGI(TAG,"writeNVData ----- %s ",buffer);
    }
    
    err = nvs_set_str(my_handle, key, (const char*)buffer);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    ESP_LOGI(TAG,"Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
    return err;
}

void setLastKnownStatus(char *value)
{
    writeNVData("lastKnownStatus", value);
}
static int ble_spp_server_gap_event(struct ble_gap_event *event, void *arg);
static uint8_t own_addr_type;
int gatt_svr_register(void);
QueueHandle_t spp_common_uart_queue = NULL;
// uint16_t connection_handle[CONFIG_BT_NIMBLE_MAX_CONNECTIONS];
static uint16_t ble_svc_gatt_read_val_handle,
                ble_spp_svc_gatt_read_val_handle,
                ble_spp_svc_gatt_read_deviceState_handle,
                ble_spp_svc_gatt_read_currentStatus_handle,
                ble_spp_svc_gatt_read_systemVersion_handle,
                ble_spp_svc_gatt_read_meteringState_handle;

/* 16 Bit SPP Service UUID */
#define BLE_SVC_SPP_UUID16                  0x00EE
//000000ee
/* 16 Bit SPP Service Characteristic UUID */
#define BLE_SVC_SPP_CHR_UUID16              0xEE01
//0000ee01

#define BLE_SVC_CUS_UUID16                  0x00DD

#define BLE_SVC_CUS_SYSTEM_UUID16           0xDD01
#define BLE_SVC_CUS_CURRENT_STATUS_UUID16   0xDD02
#define BLE_SVC_CUS_DEVICE_STATE_UUID16     0xDD03
#define BLE_SVC_CUS_METERING_STATE_UUID16   0xDD04

void ble_store_config_init(void);

/**
 * Logs information about a connection to the console.
 */

void
print_bytes(const uint8_t *bytes, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        MODLOG_DFLT(INFO, "%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
}

void
print_addr(const void *addr)
{
    const uint8_t *u8p;

    u8p = addr;
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

static void
ble_spp_server_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void ble_spp_server_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        //printf("\n-----------RC [%d]------ [%s] - [%s] - [%d]",rc, __FILE__,__FUNCTION__,__LINE__);
        MODLOG_DFLT(ERROR, "1111error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_spp_server_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "2222 error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

int current_handle_arr[CONFIG_BT_NIMBLE_MAX_CONNECTIONS+1] = {0};// max connection in config  are 8
static int ble_spp_server_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        printf("\nConnected conn_hndl[%d]\n",event->connect.conn_handle);

    /* A new connection was established or a connection attempt failed. */
        // printf("connection %s; status=%d ",
        //             event->connect.status == 0 ? "established" : "failed",
        //             event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            ble_spp_server_print_conn_desc(&desc);
            // connection_handle[event->connect.conn_handle - 1] = event->connect.conn_handle;
            current_handle_arr[event->connect.conn_handle] = 1;
            conn_handle_notify = 1;
        }
        // test disconnect
        // vTaskDelay(2000 / portTICK_PERIOD_MS);
        // printf("\nBLE Connected ...disconnecting handle ->event->connect.conn_handle[%d]\n",event->connect.conn_handle);
        // int ret = ble_gap_terminate(event->connect.conn_handle, BLE_ERR_UNKNOWN_HCI_CMD/*BLE_ERR_CONN_LIMIT*/);
        // if (ret != 0) { ESP_LOGI(TAG, "Error 0x%X disconnecting", ret);}
        MODLOG_DFLT(INFO, "\n");
        if (event->connect.status != 0 || CONFIG_BT_NIMBLE_MAX_CONNECTIONS > 1) {
            /* Connection failed or if multiple connection allowed; resume advertising. */
            ble_spp_server_advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        // printf("\nBLE DisConnected ...event->connect.conn_handle[%d]\n",event->connect.conn_handle);
        printf("\ndisconnect hndl[%d]\n",event->disconnect.conn.conn_handle);
        MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
        ble_spp_server_print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        /* Connection terminated; resume advertising. */
        ble_spp_server_advertise();
        countBraces = 0;
        current_handle_arr[event->disconnect.conn.conn_handle] = 0;
        conn_handle_notify = -1;
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        ble_spp_server_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete; reason=%d",
                    event->adv_complete.reason);
        ble_spp_server_advertise();
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

static void
ble_spp_server_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
ble_spp_server_on_sync(void)
{
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");
    /* Begin advertising. */
    ble_spp_server_advertise();
}

void ble_spp_server_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

char cbuff[1280] = {0};//allocate it and free in commissioning loop
static int cnt = 0;
uint8_t buff_filled=0;

char* getJson(){
    //validate json in main thread ...
    cnt = 0;
    memset(cbuff,0,sizeof(cbuff));
    printf("\r\nWaiting for cbuff to fill ........ \r\n");
    while(1){
        if(buff_filled )
        {
            buff_filled = 0;
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        //printf("**********************Blinking !!!");
        // if(!is_commisioned){//for bluetooth mode  // ?????
            // gpio_set_level(LED_GPIO,0);
            // vTaskDelay(500 / portTICK_PERIOD_MS);
            // gpio_set_level(LED_GPIO, 1);
            // vTaskDelay(500 / portTICK_PERIOD_MS);
        // } else{
        //     vTaskDelay(1000 / portTICK_PERIOD_MS);
        // }
    }
    //printf("\r\ncbuff filled ........  %s",cbuff);
    cnt = 0;
    return cbuff;
}

float getCurrent();
float getPower();
float getVoltage();
int getTotalTime();
float getTotalUnit();
/* Callback function for custom service */
//ble comminication
void notify_over_control_chr(int type){
    // printf("\n\n< notify_over_control_chr >\n\n");
    struct os_mbuf *om;
    int rc = 0;
    int buffLen = 0;
    uint16_t conn_handle = conn_handle_notify;
    uint16_t active_connection = 0;
    //------------------------
    for(int i = 0;i<9;i++){
        if(current_handle_arr[i] == 1){
            active_connection++;
        }
    }
    //-----------------------

    printf("\n--------- -------------- ------------- conn_handle_notify - %d  active_connection[%d]\n",conn_handle_notify,active_connection);
    //if(conn_handle > -1 && conn_handle < CONFIG_BT_NIMBLE_MAX_CONNECTIONS){
    if(active_connection > 0 ){
        switch(type)
        {
            case 1:
                ESP_LOGI(TAG, "Notification ble_spp_svc_gatt_read_deviceState_handle  ");
                memset(rbuff,0,sizeof(rbuff));
                char* status = getCurrentStatus();
                for(int i =0 ; i < strlen(status); i++)
                {
                    rbuff[i] = status[i];
                }
                uint32_t test = 1;
                for(int i = 0;i<9;i++){
                    if(current_handle_arr[i] == 1){
                        ESP_LOGI(TAG, "Notifying conn=%d", i);
                        om = ble_hs_mbuf_from_flat(&rbuff, sizeof(&status));
                        rc = ble_gattc_notify_custom((uint16_t)i, ble_spp_svc_gatt_read_deviceState_handle, om);
                        if (rc != 0) {
                            ESP_LOGE(TAG, "error notifying; rc=%d", rc);
                        }
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                
            break;
            case 2:
            ESP_LOGI(TAG, "NOtification ble_spp_svc_gatt_read_currentStatus_handle ");
                memset(rbuff,0,sizeof(rbuff));
                
               if(getTurnONState())
                {
                    ESP_LOGI(TAG, "Callback for read");
                    //sprintf(rbuff,"1,%.2f,%.2f,%.2f,%d,%.2f",getCurrent(),getVoltage(),getPower(),getTotalTime(),getTotalUnit());
                    rbuff[0] = '1';
                }
                else
                {
                    //on/off,c,v,p,ttime,tunit
                    rbuff[0] = '0';
                }
                buffLen = strlen(rbuff);
                for(int i = 0;i<9;i++){
                    if(current_handle_arr[i] == 1){
                        ESP_LOGI(TAG, "Notifying conn=%d", i);
                        //printf("\nNotifying conn=%d\n", i);
                        om = ble_hs_mbuf_from_flat(&rbuff, buffLen);
                        rc = ble_gattc_notify_custom((uint16_t)i, ble_spp_svc_gatt_read_currentStatus_handle, om);
                        if (rc != 0) {
                            ESP_LOGE(TAG, "error notifying; rc=%d", rc);
                        } //else {
                        //     printf("\nNotify Success.\n");
                        // }
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;
            case 3:
                ESP_LOGI(TAG, "NOtification metering val update");
                memset(rbuff,0,sizeof(rbuff));
                if(getTurnONState())
                {
                    ESP_LOGI(TAG, "Callback for metering val read");
                    if(getPower() > 1)
                        sprintf(rbuff,"%.2f,%.2f,%.2f,%d,%.2f",getCurrent(),getVoltage(),getPower(),getTotalTime(),getTotalUnit());
                    else    
                        sprintf(rbuff,"%.2f,%.2f,%.2f,%d,%.2f",0,getVoltage(),0,getTotalTime(),getTotalUnit());
                }
                else
                {
                    //on/off,c,v,p,ttime,tunit
                    rbuff[0] = '0';
                }
                buffLen = strlen(rbuff);
                for(int i = 0;i<9;i++){
                    if(current_handle_arr[i] == 1){
                        ESP_LOGI(TAG, "Notifying conn=%d", i);
                        printf("\nNotifying conn=%d\n", i);
                        om = ble_hs_mbuf_from_flat(&rbuff, buffLen);
                        rc = ble_gattc_notify_custom((uint16_t)i, ble_spp_svc_gatt_read_meteringState_handle, om);
                        if (rc != 0) {
                            ESP_LOGE(TAG, "error notifying; rc=%d", rc);
                        }
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;                
        }
    }

}
//Bluetooth Low Energy
static int  ble_svc_gatt_handler(uint16_t conn_handle, uint16_t attr_handle,struct ble_gatt_access_ctxt *ctxt, void *arg)
{
      switch(ctxt->op){
      case BLE_GATT_ACCESS_OP_READ_CHR:
          // printf("\n\n\n");
         ESP_LOGI(TAG, "ble_spp_svc_gatt_read_deviceState_handle [%d]",ble_spp_svc_gatt_read_deviceState_handle);
         ESP_LOGI(TAG, "ble_spp_svc_gatt_read_currentStatus_handle [%d]",ble_spp_svc_gatt_read_currentStatus_handle);
         ESP_LOGI(TAG, "ble_spp_svc_gatt_read_systemVersion_handle [%d]",ble_spp_svc_gatt_read_systemVersion_handle);

          if(attr_handle == ble_spp_svc_gatt_read_deviceState_handle)
              {
                ESP_LOGI(TAG, "ble_spp_svc_gatt_read_deviceState_handle  ");
                ESP_LOGI(TAG, "attr_handle  [%d]",attr_handle);
                memset(rbuff,0,sizeof(rbuff));
                char* status = getCurrentStatus();
                for(int i =0 ; i < strlen(status); i++)
                {
                    rbuff[i] = status[i];
                }
                os_mbuf_append(ctxt->om, &rbuff, sizeof status);
            }

            if(attr_handle == ble_spp_svc_gatt_read_currentStatus_handle)
                {
                    ESP_LOGI(TAG, "ble_spp_svc_gatt_read_currentStatus_handle ");
                    ESP_LOGI(TAG, "attr_handle  [%d]",attr_handle);
                    memset(rbuff,0,sizeof(rbuff));
                    int buffLen = 0;
                    if(getTurnONState())
                    {
                        ESP_LOGI(TAG, "Callback for read");

                        rbuff[0] = '1';
                        //sprintf(rbuff,"1,%f,%f,%f,%d,%f",getCurrent(),getVoltage(),getPower(),getTotalTime(),getTotalUnit());
                    }
                    else
                    {
                        rbuff[0] = '0';
                    }
                    buffLen = strlen(rbuff);
                    os_mbuf_append(ctxt->om, &rbuff, buffLen);
                }
            if(attr_handle == ble_spp_svc_gatt_read_systemVersion_handle)
            {
                ESP_LOGI(TAG, "ble_spp_svc_gatt_read_systemVersion_handle ");
                ESP_LOGI(TAG, "attr_handle  [%d]",attr_handle);
                memset(rbuff,0,sizeof(rbuff));
                char* version = getCurrentSWVersion();
                    ESP_LOGI(TAG, "version  -- [%s]",version);
                    for(int i =0 ; i < strlen(version); i++)
                    {
                        rbuff[i] = version[i];
                    }
                    os_mbuf_append(ctxt->om, &rbuff, strlen(version));
            }
            if(attr_handle == ble_spp_svc_gatt_read_meteringState_handle)
            {
                ESP_LOGI(TAG, "NOtification metering val update");
                memset(rbuff,0,sizeof(rbuff));
                if(getTurnONState())
                {
                    ESP_LOGI(TAG, "Callback for metering val read");
                    sprintf(rbuff,"%.2f,%.2f,%.2f,%d,%.2f",getCurrent(),getVoltage(),getPower(),getTotalTime(),getTotalUnit());
                }
                else
                {
                    //on/off,c,v,p,ttime,tunit
                    rbuff[0] = '0';
                }
                int buffLen = strlen(rbuff);
                // ctxt->om = ble_hs_mbuf_from_flat(&rbuff, buffLen);
                // ESP_LOGI(TAG, "Notifying conn=%d", conn_handle);
                // printf("\nmetering Notifying conn=%d\n", conn_handle);
                os_mbuf_append(ctxt->om, &rbuff, buffLen);
            }
                
      break;

      case BLE_GATT_ACCESS_OP_WRITE_CHR:      
      if(attr_handle == ble_spp_svc_gatt_read_currentStatus_handle)
      {
        for(uint8_t i = 0;i< ctxt->om->om_len;++i){
                //printf(" %02X ", *(p_simp_cb_data->msg_data.write.p_value + i));
                char ch = *(ctxt->om->om_data + i);
//                printf("%c", ch);                                
                    if(ch == '{')
                    {
                       memset(cbuff,0,sizeof(cbuff));
                       cnt=0;                                                               
                        }                                                                  
                cbuff[cnt++] = ch;
                if(ch == '}')
                {//end                
                    if(validateJson(cbuff))
                    {
                        buff_filled = 1;                                            
                    }                    
                }                
            }               
      }
      else
      {
       for(uint8_t i = 0;i< ctxt->om->om_len;++i){
                //printf(" %02X ", *(p_simp_cb_data->msg_data.write.p_value + i));
                char ch = *(ctxt->om->om_data + i);
//                printf("%c", ch);                                
                    if(ch == '{')
                    {//start                         
                        if(countBraces == 0)
                        {                            
                            memset(cbuff,0,sizeof(cbuff));
                            cnt=0;                                        
                            countBraces++;
                        }                                              
                    }
                cbuff[cnt++] = ch;
                if(ch == '}')
                {//end                
                    if(validateJson(cbuff))
                    {
                        buff_filled = 1;                    
                        countBraces = 0;
                    }                    
                }                
            }               
       }
      break;

      default:
         ESP_LOGI(TAG, "\nDefault Callback");
      break;
      }
      return 0;
}

/* Define new custom service */
static const struct ble_gatt_svc_def new_ble_svc_gatt_defs[] = {
     {
        /*** Service: SPP */
          .type = BLE_GATT_SVC_TYPE_PRIMARY,
          .uuid = BLE_UUID16_DECLARE(BLE_SVC_SPP_UUID16),
          .characteristics = (struct ble_gatt_chr_def[]) { {
                        /* Support SPP service */
                        .uuid = BLE_UUID16_DECLARE(BLE_SVC_SPP_CHR_UUID16),
                        .access_cb = ble_svc_gatt_handler,
                        .val_handle = &ble_spp_svc_gatt_read_val_handle,
                        .flags = BLE_GATT_CHR_F_WRITE ,
        },
                {
                         0, /* No more characteristics */
                }
           },
      },
      {
        /*** Service: SPP */
          .type = BLE_GATT_SVC_TYPE_PRIMARY,
          .uuid = BLE_UUID16_DECLARE(BLE_SVC_CUS_UUID16),
          .characteristics = (struct ble_gatt_chr_def[]) { {
                        /* Support SPP service */
                        .uuid = BLE_UUID16_DECLARE(BLE_SVC_CUS_SYSTEM_UUID16),
                        .access_cb = ble_svc_gatt_handler,
                        .val_handle = &ble_spp_svc_gatt_read_systemVersion_handle,
                        .flags = BLE_GATT_CHR_F_READ,
        },
        {
                        /* Support SPP service */
                        .uuid = BLE_UUID16_DECLARE(BLE_SVC_CUS_CURRENT_STATUS_UUID16),
                        .access_cb = ble_svc_gatt_handler,
                        .val_handle = &ble_spp_svc_gatt_read_currentStatus_handle,
                        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        },
        {
                        /* Support SPP service */
                        .uuid = BLE_UUID16_DECLARE(BLE_SVC_CUS_METERING_STATE_UUID16),
                        .access_cb = ble_svc_gatt_handler,
                        .val_handle = &ble_spp_svc_gatt_read_meteringState_handle,
                        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        },
        {
                        /* Support SPP service */
                        .uuid = BLE_UUID16_DECLARE(BLE_SVC_CUS_DEVICE_STATE_UUID16),
                        .access_cb = ble_svc_gatt_handler,
                        .val_handle = &ble_spp_svc_gatt_read_deviceState_handle,
                        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        },

                {
                         0, /* No more characteristics */
                }
           },
      },
      {
          0, /* No more services. */
      },
};

int gatt_svr_register(void)
{
     int rc=0;

     rc = ble_gatts_count_cfg(new_ble_svc_gatt_defs);

     if (rc != 0) {
         return rc;
     }

     rc = ble_gatts_add_svcs(new_ble_svc_gatt_defs);
     if (rc != 0) {
         return rc;
     }

     return 0;
}

void startBLE(char * bleName)
{
    ESP_LOGI(TAG,"Starting BLE");
    int rc;

    nimble_port_init();

    /* Initialize uart driver and start uart task */
   // ble_spp_uart_init();

    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = ble_spp_server_on_reset;
    ble_hs_cfg.sync_cb = ble_spp_server_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
/*
    ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
#ifdef CONFIG_EXAMPLE_BONDING*/
    ble_hs_cfg.sm_bonding = 1;
/*#endif
#ifdef CONFIG_EXAMPLE_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;
#endif
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
#endif*/


    rc = new_gatt_svr_init();
    assert(rc == 0);

    /* Register custom service */
    rc = gatt_svr_register();
    assert(rc == 0);

    /* Set the default device name. */

    // printf("\n######################################Ble Name   [%s]",bleName);
    rc = ble_svc_gap_device_name_set(bleName);
    assert(rc == 0);
    /* XXX Need to have template for store */
    ble_store_config_init();
    nimble_port_freertos_init(ble_spp_server_host_task);
}

void init_schedule_idx();

void clearScheduleFromSpiffs(int path_idx){

    char fPath[20];
    memset(fPath, 0, sizeof(fPath));
    //printf("\nunlinking fPath : %s\n",fPath);
    sprintf(fPath,"/spiffs/sch%d.cfg",path_idx);
    ESP_LOGI(TAG,"unlinking fPath : %s",fPath);
    unlink(fPath);
}

void persistSchedule(char *data, int path_idx){

    char fPath[20];
    memset(fPath, 0, sizeof(fPath));
    sprintf(fPath,"/spiffs/sch%d.cfg",path_idx);

    ESP_LOGI(TAG,"\nopening file -> %s\n",fPath);

    FILE *f = fopen(fPath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open schedule for writing");
    } else {
       ESP_LOGI(TAG,"%s open success",fPath);
    }
    fprintf(f, "%s",data);
    fclose(f);
    f = NULL;
}
void loadScheduleFile(char *path);

int getClosestActionTime(int c_min);

int getClosestStartAction();
int getClosestEndAction();
char *getClosestStartActionVal();
char *getClosestEndActionVal();

typedef struct sched_task_s{
    int curr_min;
    int sched_min;
}stask_t;
stask_t task_data = {0};
// static void exec_schedule_task(void *pvParameters){
//     printf("\nexec_schedule_task entry \n");

//     vTaskDelay(60*1000 / portTICK_PERIOD_MS);
//     printf("\nexec_schedule_task exit \n");

// }
int isScheduleValidToday(int id, int day_index, bool isEndTime,int curr_time);
int getClosestActionID();
int getClosestAction();
char *getClosestActionVal();
void setClosestAction(int act);
schedule_t getCurrentSchedule(int id);

static void schedule_task(void *pvParameters){
    ESP_LOGI(TAG,"schedule_task entry ");    
    vTaskDelay(200/portTICK_PERIOD_MS);
    while(!getInternetStatus()){
        //ESP_LOGI(TAG,"schedule waiting for internet connection success ... ");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time_t now = 0;
    struct tm timeinfo = {0};

    int curr_sch_min = -1;

    while(1){
        //printf("\n\n<<< ----------------------- schedule ----------------- >>>>> \n\n");
        time(&now);
        localtime_r(&now, &timeinfo);

        int curr_min = (60 * timeinfo.tm_hour + timeinfo.tm_min);
        // int sch_min = getClosestTime();
        printf("\nCurr_min -> %d\n",curr_min);
        int sch_min = getClosestActionTime(curr_min);
        //printf("\nsch_min -> %d\n",sch_min);
        // if(sch_min >= 1440 && curr_min < 1440){
        //     sch_min = sch_min - 1440;//24hr = 24 * 60 minutes -- > for 12::00 AM -> minutes will be 0 
        // }
        ESP_LOGI(TAG,"\nhh:mm:ss -> %d:%d:%d, Total Min = %d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,curr_min);
        if(sch_min == -1){
            printf("\nNo sch available\n");
            vTaskDelay(10*1000 / portTICK_PERIOD_MS);
            continue;
        }
        //timeinfo.tm_wday - 0-sunday 1-monday .... 6-saturday
        int isScheduleExecuting = 0;
        static int at_boot = 0;


        
        schedule_t c_sch = getCurrentSchedule(getClosestActionID());

        int valid = 0;
        
        if(c_sch.s_time == sch_min){
            valid = isScheduleValidToday(getClosestActionID(), timeinfo.tm_wday,false,curr_min);// based on start time only
        }else if(c_sch.duration > 0){

            int sch_end = c_sch.s_time + c_sch.duration;
            if(sch_end >= 1440){
                sch_end = sch_end - 1440;
            }
            if(sch_end == sch_min){
                valid = isScheduleValidToday(getClosestActionID(), timeinfo.tm_wday,true, curr_min);// based on start time only
            }  
        } 
        // printf("\nCurrent Schedule struct ...");
        // printf("\nid:%d",getClosestActionID());
        // printf("\nstart_time : %d",c_sch.s_time);
        // printf("\tduration:%d",c_sch.duration);
        // printf("\ts_act:%d",c_sch.s_action );
        // printf("\te_act:%d\n",c_sch.e_action );
        //printf("\ns_actV:%c",c_sch.s_action_val );
        //printf("\ne_actV:%c",c_sch.e_action_val );
        printf("\n");
        // int target_time = c_sch.s_time + c_sch.duration;
        // printf("\tB:Target time:%d\n",target_time);
        // if(c_sch.duration > 0 && (curr_min < 1440 && target_time >= 1440))
        //     target_time = target_time - 1440;
        // printf("\tA:Target time:%d\n",target_time);
        if(valid && c_sch.duration > 0 &&!isScheduleExecuting && at_boot != 1){
            int in_between = 0;
            if((c_sch.s_time + c_sch.duration) >= 1440 ){
                if(sch_min == (c_sch.s_time + c_sch.duration) - 1440){
                    in_between = 1;
                    //valid = 1;//after boot
                }
            }else if(sch_min == (c_sch.s_time + c_sch.duration)){
                in_between = 1;
            }
            if(in_between){
                sch_min = curr_min;
                setClosestAction(c_sch.s_action);
                printf("\n\n in b/w sch [%d]\n\n",c_sch.s_action);
                at_boot = 1;
            }
        }
        //printf("\n\n VALID DAY OF WEEK - %d \n\n", valid);
        // if(c_sch.duration > 0 &&!isScheduleExecuting && at_boot != 1 && (curr_min > c_sch.s_time) && (curr_min < (c_sch.s_time + c_sch.duration))){
        //     sch_min = curr_min;
        //     setClosestAction(c_sch.s_action);
        //     printf("\n\n in between schedule [%d]\n\n",c_sch.s_action);
        //     at_boot = 1;
        // }
        printf("\nsch_min -> %d\n",sch_min);

        // handle case when rebooten between schedule ........
        // ESP_LOGI(TAG,"\nhh:mm:ss -> %d:%d:%d, Total Min = %d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,curr_min);
        if(valid && (curr_min == sch_min) && !isScheduleExecuting){
            at_boot = 1;
            isScheduleExecuting = 1;
            int act = getClosestAction();

            if(act == 0){//ON
                printf("\n\nOOONNNN\n");
                //turnOnPlug(0);
            }else if(act == 1){ // OFF
                printf("\n\nOOOFFF\n");
                //turnOffPlug(0);
            }
            vTaskDelay(60*1000 / portTICK_PERIOD_MS);
            isScheduleExecuting = 0;

        }
        ESP_LOGI(TAG,"Check schedule available again");
        //printf("\n\n<<< ----------------------- schedule ----------------- >>>>> \n\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
void init_action_schedule_idx();

int resetCounter(int factory_reset)
{    int counter = 0;
    ESP_LOGI(TAG,"Enter Reset Counter");
    nvs_handle_t my_handle;
    int err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG,"Done\n");

        // Read
        ESP_LOGI(TAG,"Reading restart counter from NVS ... ");
        int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);
        switch (err) {
            case ESP_OK:
                ESP_LOGI(TAG,"Restart counter = %d\n", restart_counter);
                counter = restart_counter;
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(TAG,"The value is not initialized yet!\n");
                break;
            default :
                ESP_LOGI(TAG,"Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Write
        //printf("Updating restart counter in NVS ... ");
        if(!factory_reset)
            restart_counter++;
        else
            restart_counter = 0;//in case factory reset set counter to 0
        err = nvs_set_i32(my_handle, "restart_counter", restart_counter);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing N V S ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);

    }
     return counter;
}

#if 0 /* BUTTON GPIO IMPLEMENTATION */

// Semaphore handle
SemaphoreHandle_t button_semaphore = NULL;

void IRAM_ATTR isr_button_pressed(void *args)
{
    // Notify the button task
    xSemaphoreGiveFromISR(button_semaphore, NULL);

    //int btn_state = gpio_get_level(BUTTON_GPIO);
    printf("\n\nButton Interrupt isr_button_pressed\n\n");
    //gpio_set_level(LED_GPIO,btn_state);
}

void _button_task(void *pvParam)
{
    for (;;)
    {
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) ==  pdTRUE)
        {
            printf("\n\n _button_task notified by isr_button_pressed ...............\n\n");

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

void set_button_gpio_config(){
    
    // Create the binary semaphore
    button_semaphore = xSemaphoreCreateBinary();

    gpio_pad_select_gpio(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);
    // Create the Wifi reset button task
    xTaskCreatePinnedToCore(&_button_task, "_button_task", WIFI_RESET_BUTTON_TASK_STACK_SIZE, NULL, WIFI_RESET_BUTTON_TASK_PRIORITY, NULL, WIFI_RESET_BUTTON_TASK_CORE_ID);
    //xTaskCreate(&_button_task, "_button_task", 2048, NULL, 5, NULL);
    gpio_install_isr_service(0/*ESP_INTR_FLAG_DEFAULT*/);
    gpio_isr_handler_add(BUTTON_GPIO, isr_button_pressed, NULL);

}

#endif /* BUTTON GPIO IMPLEMENTATION */

int state = 0;
QueueHandle_t interputQueue;
uint32_t button_time = 0;  
uint32_t last_button_time = 0;

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    button_time = esp_log_timestamp();
    if (button_time - last_button_time > 30)
    {
        int pinNumber = (int)args;
        xQueueSendFromISR(interputQueue, &pinNumber, NULL);
        last_button_time = button_time;
    }
}

void LED_Control_Task(void *params)
{
    int status = 2;
    int pinNumber;
    int curr_level=-1;
    int prev_level=-1;
    uint32_t start = 0, end = 0;
    char *nv_data = readNVData("lastKnownStatus");
    while (true)
    {
        if(getPowerMonitoringStatus() == 0 || !isCommisioned()){
            status = 0;
        }
        
        if (xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY))
        {
            curr_level = gpio_get_level(BUTTON_GPIO_1);

            if(curr_level == 0 && curr_level != prev_level){
                start = esp_log_timestamp();
                printf("start_ts: %u",start);
                prev_level = curr_level;
                printf("Button Press...\n");
                int count = getCount();
                // if((strstr(nv_data,"on") != NULL))
                // {
                //     printf("LKS:ON...........ON\n");
                //     count--;
                // }
                    
                int on_off = count%2;

                int64_t t1 = esp_timer_get_time();
                ESP_LOGI(TAG, "Entering timer section: %lld us", t1);

                const esp_timer_create_args_t my_timer_args = {
                    .callback = &timer_callback,
                    .name = "My Timer"};
                ESP_ERROR_CHECK(esp_timer_create(&my_timer_args, &timer_handler));
                ESP_ERROR_CHECK(esp_timer_start_once(timer_handler, 5000000));

                printf("\ncount = %d, on_off = %d\n",count, on_off);
                if (on_off)
                {
                    //turnOnPlug(status);
                }
                else
                {
                    //turnOffPlug(status);
                }
            }

            if(curr_level == 1 && curr_level != prev_level){
                end = esp_log_timestamp();
                printf("end_ts: %u",end);
                prev_level = curr_level;
                printf("Button Release...\n");

                if(end - start >= 5000)
                {
                    printf("Factory ..........................................Reset\n");
                    factoryReset();
                }
                else
                {
                    ESP_ERROR_CHECK(esp_timer_stop(timer_handler));
                    ESP_ERROR_CHECK(esp_timer_delete(timer_handler));
                    ESP_LOGI(TAG, "\nStopped and deleted timers");
                }
                
            }                
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}


void task_button(void *pvParameter)
{
    //Configure LED
    // gpio_pad_select_gpio(LED_GPIO);                   //Set pin as GPIO
    // gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);   //Set as Output
    //printf("LED configured\n");
    
    //gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);//ok

    gpio_set_direction(BUTTON_GPIO_1, GPIO_MODE_INPUT);//ok
    // gpio_set_direction(BUTTON_GPIO_2, GPIO_MODE_INPUT);//ok
    // gpio_set_direction(BUTTON_GPIO_3, GPIO_MODE_INPUT);//ok
    // gpio_set_direction(BUTTON_GPIO_4, GPIO_MODE_INPUT);//ok

    // gpio_pad_select_gpio(POWER_GPIO);
    // gpio_set_direction(POWER_GPIO, GPIO_MODE_OUTPUT);

    //printf("\n current level of control gpio POWER_GPIO 4 - %d \n",gpio_get_level(POWER_GPIO));

    // int ret = gpio_set_level(POWER_GPIO,1);//default on

    // printf("ret -%d - stat -> %d \n",ret,4);

    int status = 2;//getPowerMonitoringStatus();
    // printf("\npow =>> %d - comm_sts - %d\n",getPowerMonitoringStatus(),isCommisioned());
    // printf("\nSTATUS =>> %d\n",status);
    int curr_level=-1;
    int prev_level=-1;
    long start = 0, end = 0,curr=0,cnt=0;
    static int once = 0;
    while (1)
    {
        if(getPowerMonitoringStatus() == 0 || !isCommisioned()){
            status = 0;
        }
        curr_level = gpio_get_level(BUTTON_GPIO);
// esp_timer_get_time();
        //printf("gpio - %d curr val - %d \n",20,curr_level);

        //if(curr_level == 0 && curr_level != prev_level){

        if(curr_level == 0 && curr_level != prev_level){
            prev_level = curr_level;
            printf("Button Pressed setting POWER_GPIO - 4 as LOW ...\n");
            //gpio_set_level(POWER_GPIO,0);
            int actual_event = 0;
            for(int i = 0;i<3;i++){
                curr_level = gpio_get_level(BUTTON_GPIO);
                if(curr_level == 0){
                    actual_event++;
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            printf("\n >>>>>>>>>>>>>>>>> Act_event - %d \n",actual_event);

if(actual_event == 3){// to avoid fake relay on/off events.....
            int count = getCount();
            int on_off = count%2;
            printf("\ncount = %d, on_off = %d\n",count, on_off);
            //gpio_set_level(POWER_GPIO, on_off);//on off toggle
            
            if(on_off){
                //turnOnPlug(status/*0*/);
            }else{
                //turnOffPlug(status/*0*/);
            }
}
            //gpio_set_level(POWER_GPIO,!gpio_get_level(20));//on off toggle
            //press time t1
            start = currentSec();
            printf("\n BUTTON Pressed TIME - %d \n",start);
            cnt = 1;
            }

        //if(state == PRESSED)
        //if(state == RELEASED)

        if(curr_level == 1 && curr_level != prev_level){
            prev_level = curr_level;
            printf("Button Released setting POWER_GPIO- 4 as HIGH...\n");
            //gpio_set_level(POWER_GPIO,1);
            end = currentSec();
            printf("\n BUTTON RELEASED TIME - %d \n",end);
            if(cnt > 50){
                factoryReset();
            }
            cnt = 0;
        }
        // end = currentSec();

        // curr = currentSec();
        if(cnt > 0 && ++cnt == 50 ){

            printf("\nFormatting device  BLINKING ...................\n");
            xTaskCreate(led_blink_plug, "led_blink_plug", 512, NULL, 5, NULL);
            // for(int i =0;i<5;i++){
            //     gpio_set_level(LED_GPIO,0);
            //     vTaskDelay(250 / portTICK_PERIOD_MS);
            //     gpio_set_level(LED_GPIO, 1);
            // }

        }

        // if(start > 0 && end - start > 5){
        //     printf("\nFormatting device ...................\n");
        //     // for(int i =0;i<5;i++){
        //     //     gpio_set_level(LED_GPIO,0);
        //     //     vTaskDelay(250 / portTICK_PERIOD_MS);
        //     //     gpio_set_level(LED_GPIO, 1);
        //     //     vTaskDelay(250 / portTICK_PERIOD_MS);
        //     //     gpio_set_level(LED_GPIO,0);
        //     //     vTaskDelay(250 / portTICK_PERIOD_MS);
        //     //     gpio_set_level(LED_GPIO, 1);
        //     //     vTaskDelay(250 / portTICK_PERIOD_MS);
        //     // }
        //     factoryReset();
        //     start = 0;
        // }
        //running time t2
        //if(t2 - t1 > 5){//led blink //factoryReset}
        //printf("gpio - %d curr val - %d \n",4,curr_level);
        vTaskDelay(100/portTICK_PERIOD_MS);

    }

}

void Button_Led_task()
{
#if 0
    set_button_gpio_config();
#else    
    xTaskCreate(&task_button, "task_button", 2048, NULL, 5, &button_task_hdl);
#endif
}

void removeButtonTask(){

    if(button_task_hdl != NULL)
    {
        ESP_LOGI(TAG, "\nDeleting Task button_task_hdl\n");
        vTaskDelete(button_task_hdl);
        button_task_hdl = NULL;
    }
    else
    {
        ESP_LOGI(TAG, "Not Deleting button_task_hdl coz its null");
    }
}

static void off_after_task(void *pv)
{
    int tm = *((int*)pv); // minutes
    ESP_LOGI(TAG, "off_after_task [%d] minutes", tm);
    while(1){
        vTaskDelay(tm*60*1000 / portTICK_PERIOD_MS);
        if(getTurnONState() && getInternetStatus())
            ;//turnOffPlug(1,2);
    }
    vTaskDelete(NULL);
}

void startOffAfterTask(int time){
    if(time == 0 && off_after_task_hdl != NULL){
        vTaskDelay(100 / portTICK_PERIOD_MS);
        vTaskDelete(off_after_task_hdl);
    }
    else if (time  > 0){
        xTaskCreate(&off_after_task, "off_after_task", 2048, &time, 5, &off_after_task_hdl);
    }
}

int getCalibrationStatus();

// ---------------------------------------------
// implement

// /control-monitor/api/v1/sp/{spid}/device/energyMeasurementData
// {
//     "deviceUUID":"deviceUUID",
//     "powerUnit":"powerUnit",
//     "timestamp":"millis"
// }

// /device/{deviceUUID}/notifications
// {
//     "deviceUUID":"deviceUUID",
//     "powerUnit":"powerUnit",
//     "timestamp":"millis"
// }

// schedule index also ........
// ------------------------------------------------

void set_ssl_version(char *ver );
void setTotalTimeFromNVS(int ut);
void setDayUnitFromNVS(float ut);
void setTotalUnitFromNVS(float ut);


// typedef struct {
//     uint64_t event_count;
// } example_queue_element_t;

// static bool IRAM_ATTR example_timer_on_alarm_cb_v1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
// {
//     BaseType_t high_task_awoken = pdFALSE;
//     QueueHandle_t queue = (QueueHandle_t)user_data;
//     // stop timer immediately
//     gptimer_stop(timer);
//     // Retrieve count value and send to queue
//     example_queue_element_t ele = {
//         .event_count = edata->count_value
//     };
//     xQueueSendFromISR(queue, &ele, &high_task_awoken);
//     // return whether we need to yield at the end of ISR
//     return (high_task_awoken == pdTRUE);
// }





void app_main()
{
    int resetCount = 0;

    esp_err_t ret;


    //timer interrupt start
   


    // while (true)
    // {
    //     esp_timer_dump(stdout);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
    //timer interrupt end


    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGI(TAG, "\nNVS Flash Failed formatting and re initializing !!!\n");
        ret = nvs_flash_init();
    } else{
        ESP_LOGI(TAG, "\nNVS Flash Successfully Initialized !!!\n");
    }
    ESP_ERROR_CHECK( ret );
    resetCount = resetCounter(0);
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG,"\n##################\n##################\n");
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
        ESP_LOGI(TAG,"\n##################\n##################\n");
        //setCurrentSWVersion(running_app_info.version);
        set_ssl_version(running_app_info.version);
    }
    ESP_LOGI(TAG,"\n##################Reset Count ################## %d",resetCount);
    if(resetCount >= 10)
    {
       ESP_LOGI(TAG,"Start Bulb on Factory Reset resetCount = %d\n", resetCount);
       printf("\nFactory rstCNT = %d\n", resetCount);
       resetCounter(1);
       factoryReset();
    }
    
    // if(strstr(running_app_info.version,"SYSTEM-10A"))
    // {
    //     ESP_LOGI(TAG, "Init with 10A");
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Init with 16A");
    // }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    PowerMonitorFunction();

    while(getPowerMonitoringStatus() != 1) vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    char *versionNV = readNVData("svVersion");
    if(strcmp(running_app_info.version,versionNV)!=0)
    {
        ESP_LOGI(TAG,"\n##################\n##################\n");
        ESP_LOGI(TAG, "Running       firmware version: %s", running_app_info.version);
        ESP_LOGI(TAG, "Older Version firmware version: %s", versionNV);
        printf("\nRunning       firmware version: %s", running_app_info.version);
        printf("\nOlder Version firmware version: %s", versionNV);
        ESP_LOGI(TAG,"\n##################\n##################\n");
        setinventoryUpdateReq(true);
    }

    int spiffs_status = init_spiffs();
     uint8_t mac[6];
     esp_read_mac(mac, ESP_IF_WIFI_STA);
     memset(mac_str,0,sizeof(mac_str));
     if (-1 == sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])) {
         abort();
     }
     ESP_LOGI(TAG,"WiFi-Mac ==>> [ %s ]",mac_str);

    gpio_pad_select_gpio(POWER_GPIO);
    gpio_set_direction(POWER_GPIO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_GPIO);                   //Set pin as GPIO
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);   //Set as Output

//Interrupt based sensing START
    //gpio_pulldown_dis(BUTTON_GPIO);
    //gpio_pullup_en(BUTTON_GPIO);

    //ESP TIMER enable

    // example_queue_element_t ele;
    // QueueHandle_t queue = xQueueCreate(10, sizeof(example_queue_element_t));

    // printf("WHat Happened here...................................");
    // ESP_LOGI(TAG, "Create timer handle");
    // gptimer_handle_t gptimer = NULL;
    // gptimer_config_t timer_config = {
    //     .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    //     .direction = GPTIMER_COUNT_UP,
    //     .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    // };
    // ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // gptimer_event_callbacks_t cbs = {
    //     .on_alarm = example_timer_on_alarm_cb_v1,
    // };
    // ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, queue));

    // ESP_LOGI(TAG, "Enable timer");
    // ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // ESP_LOGI(TAG, "Start timer, stop it at alarm event");
    // gptimer_alarm_config_t alarm_config1 = {
    //     .alarm_count = 1000000, // period = 1s
    // };
    // ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));
    // ESP_ERROR_CHECK(gptimer_start(gptimer));
    // if (xQueueReceive(queue, &ele, pdMS_TO_TICKS(2000))) {
    //     ESP_LOGI(TAG, "Timer stopped, count=%llu", ele.event_count);
    // } else {
    //     ESP_LOGW(TAG, "Missed one count event");
    // }

    // ESP_LOGI(TAG, "Set count value");
    // ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, 100));
    // ESP_LOGI(TAG, "Get count value");
    // uint64_t count;
    // ESP_ERROR_CHECK(gptimer_get_raw_count(gptimer, &count));
    // ESP_LOGI(TAG, "Timer count value=%llu", count);

    // // before updating the alarm callback, we should make sure the timer is not in the enable state
    // ESP_LOGI(TAG, "Disable timer");
    // ESP_ERROR_CHECK(gptimer_disable(gptimer));

    // ESP_LOGI(TAG, "Stop timer");
    // ESP_ERROR_CHECK(gptimer_stop(gptimer));
    // ESP_LOGI(TAG, "Delete timer");
    // ESP_ERROR_CHECK(gptimer_del_timer(gptimer));

    // //printf("\nTIMER...............................STOPPED");
    // ESP_LOGI(TAG, "Stopped and deleted timers");

    
    //gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(BUTTON_GPIO_1, GPIO_INTR_ANYEDGE);
    // gpio_set_intr_type(BUTTON_GPIO_2, GPIO_INTR_ANYEDGE);
    // gpio_set_intr_type(BUTTON_GPIO_3, GPIO_INTR_ANYEDGE);
    // gpio_set_intr_type(BUTTON_GPIO_4, GPIO_INTR_ANYEDGE);

    interputQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(LED_Control_Task, "LED_Control_Task", 2048, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO_1, gpio_interrupt_handler, (void *)BUTTON_GPIO_1);

//Interrupt based sensing END

    //Button_Led_task(); // as per factory process button should not be enabled untill calibration and verification successful
    //printf("\n current level of control gpio POWER_GPIO 4 - %d \n",gpio_get_level(POWER_GPIO));

    // enable if required -----------
    // char *off_aft = readNVData("offAfter");

    // if(strlen(off_aft) > 1 /*&& isCommisioned()*/){//as we are not resetting NVS
    //     ESP_LOGI(TAG, "\nNV Data offAfter at Boot ##   %s\n",off_aft);
    //     int f;
    //     sscanf(off_aft, "%d", &f);
    //     ESP_LOGI(TAG, "\nNV Data offAfter int ->: %d\n",f);
    //     startOffAfterTask(f);
    //     off_after_enabled = 1;// will be used in other features to stop
    // }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //----------------------------------------------------------
    memset(current_handle_arr,0,sizeof(current_handle_arr));        
    //----------------------------------------------------------
    // PowerMonitorFunction();

    // while(getPowerMonitoringStatus() != 1) vTaskDelay(1000 / portTICK_PERIOD_MS);

    int cur_lvl = gpio_get_level(LED_GPIO);
    if(cur_lvl == 0 && strstr(running_app_info.version,"SYSTEM-16A") != NULL){
       gpio_set_level(6/*LED_GPIO*/,1);
    }
    
    //Button_Led_task();
    char *nv_data = readNVData("lastKnownStatus");

    ESP_LOGI(TAG, "\nNV Data  lastKnownStatus ## (on/off)len -> %d  %d\n",strlen(nv_data));
    if(strlen(nv_data) > 0 /*&& isCommisioned()*/){//as we are not resetting NVS
        ESP_LOGI(TAG, "\nNV Data on or off at Boot lastKnownStatus ## >0  %s\n",nv_data);
        /*if(strcmp(nv_data,"on") == 0){
            turnOnPlug(2);
        } else */if(strstr(nv_data,"off") != NULL){
            printf("\nstatus 2 OFF....................................status 2 OFF");
            //turnOffPlug(2);
        } else {//default merge on and else part
            printf("\nstatus 2 ON......................................status 2 ON");
            //turnOnPlug(2);
        }
    } else {
         ESP_LOGI(TAG, "\nNV Data on or off at Boot lastKnownStatus ##  <==0 %s\n");
         printf("\nelse ON......................................else ON");
        //turnOnPlug(2);
    }
//------------------------------------------------------------------------------

/*    
        char tmp[96];
        memset(tmp, 0, sizeof(tmp));
        writeNVData("day_1", tmp);

        test read write ......... <<<<<<<<<<<<<=======================
    

    {
        int try = 5;
        while(try--){
            char res[64];
            memset(res,0,sizeof(res));
            //9876543.023302|9876543.023302|1051200|2102400|23-12-2022
            float totalunit_= 499999.023302;
            float unit_= 499999.023302;
            int dayMinutes = 1051200;
            int totalMinutes = 2051200;
            char *today = "23-12-2022";
            sprintf(res,"%f|%f|%d|%d|%s",totalunit_,unit_,dayMinutes,totalMinutes,today);
            writeNVData("day_0",res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            char *tunit = readNVData("day_0");
            printf("\nToday: %s\n",tunit);
        }
    }*/
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // char *tunit = readNVData("totalunit");

    // if(strlen(tunit) > 1 /*&& isCommisioned()*/){//as we are not resetting NVS
    //     ESP_LOGI(TAG, "\nNV Data totalunit at Boot ##   %s\n",tunit);
    //     float f;
    //     sscanf(tunit, "%f", &f);
    //     ESP_LOGI(TAG, "\nNV Data totalunit float ->: %f\n",f);
    //     setTotalUnitFromNVS(f);
    // }
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // char *dunit = readNVData("dayunit");

    // if(strlen(dunit) > 1 /*&& isCommisioned()*/){//as we are not resetting NVS
    //     ESP_LOGI(TAG, "\nNV Data dayunit at Boot ##   %s\n",dunit);
    //     float f;
    //     sscanf(dunit, "%f", &f);
    //     ESP_LOGI(TAG, "\nNV Data dayunit float ->: %f\n",f);
    //     setDayUnitFromNVS(f);
    // }
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // char *tmin = readNVData("totalmin");

    // if(strlen(tmin) > 1 /*&& isCommisioned()*/){//as we are not resetting NVS
    //     ESP_LOGI(TAG, "\nNV Data totalmin at Boot ##   %s\n",tmin);
    //     int f;
    //     sscanf(tmin, "%d", &f);
    //     ESP_LOGI(TAG, "\nNV Data int ->: %f\n",f);
    //     setTotalTimeFromNVS(f);
    // }
//--------------------------------------------------------------------------------------
    // TODO's
/*
    1.After calibration and verification we need to enable button
    check whether calibration and verification is done.

    2. local communication and initial commissioning is via ble we need to include wifi also for range coverage and schedule(timer) toworking...
    3. for ios or android get wifi list from device which are in range of device via BLE and then connect.
    4. feature like  if enabled then plug or light will always turn off after 10 or 20 or 30 min later
    or it will turn on/off alternatively in 10static void oneshot_timer_callback(void* arg); min interval ....

    5. when is there a case when we do erase of calibration data ...???????
*/

    if(spiffs_status == 0){

        // if(getPowerMonitoringStatus() == 1){
            xTaskCreate(commissioning_task, "commissioning_task", 8192, NULL, 5, &commissioning_task_hdl);
            xTaskCreate(reset_task, "reset_task", 2048, NULL, 5, &reset_task_hdl); // Start light after 2 restart if light was off from app and power off                                                            reset to factory default after 5 restart each before 5 sec
            init_schedule_idx();
            init_action_schedule_idx();
            loadScheduleFile("/spiffs");
            vTaskDelay(200 / portTICK_PERIOD_MS);
            xTaskCreate(schedule_task, "schedule_task", 2048, NULL, 5, &schedule_task_hdl); // Start light after 2 restart if light was off from app and power off                                                            reset to factory default after 5 restart each before 5 sec
        // }
        //vTaskDelay(10000 / portTICK_PERIOD_MS);
        //PowerMonitorFunction();

    }
//#endif
}


void timer_callback(void *param)
{
    static bool ON;
    ON = !ON;
        printf("\n This is timer calling %d", ON);
        printf("\nFormatting device  BLINKING ...................\n");
        xTaskCreate(led_blink_plug, "led_blink_plug", 512, NULL, 5, NULL);
        ESP_ERROR_CHECK(esp_timer_delete(timer_handler));
        ESP_LOGI(TAG, "\nStopped and deleted timers");
    
    //gpio_set_level(GPIO_NUM_2, ON);
}
