#include <stdlib.h>
#include <string.h>

//#include "esp_system.h"
//#include "esp_log.h"

/*#include "esp_netif.h"
#include "esp_event.h"

#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
*/
#include <netdb.h>
#include <sys/socket.h>

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
//#include "mbedtls/certs.h"

/*#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"*/
//#include "esp_crt_bundle.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include <mbedtls/config.h>
//#include <mbedtls/platform.h>
//#include <mbedtls/net_sockets.h>
//#include <mbedtls/ssl.h>

//#include "esp_system.h"
//#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
//#include "esp_flash_partitions.h"
//#include "esp_partition.h"

/*#include <mbedtls/debug.h>
#include <mbedtls/ctr_drbg.h>*/
#include "json_parse.h"


//#define SERVER_HOST    "srvc.stage.platform.quboworld.com"
//#define SERVER_PORT    "443"
//#define BUFFER_SIZE    2048

char version[32] = {0};

void set_ssl_version(char *ver ){
    memset(version,0,sizeof(version));
    strcpy(version,ver);
}

char* getCurrentSWVersion()
{
    printf("\n############### => getCurrentSWVersion SSL=> %s\n",version);
    return version;
}


int getInventoryResource(char *buf){

    // ptz - 2C:D2:6B:41:C9:6D

    //char *resource =  "GET /commissioning/api/v1/sp/"+getSpid()+"/devices?macAddress="+ getHUBMacID();
    // Subscriber-Key: "+getAccessToken() ???
    //Token-Type: DEVICE that's why user-UUID: getDeviceUUID() ??? is ok ...

    //replace + with %s or %d
    char *resource = "GET /commissioning/api/v1/sp/%s/devices?macAddress=%s  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Content-type: Application/json\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n\r\n";// <<<< ========

    return sprintf( buf, resource,getSpid(),getHUBMacID(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/);
}

int getAccessTokenResource(char *buf){
    // users/userUUID  for Ptz what in case of HUB.
    //only used for length calculation
    char*tmp ="{\r\n" \
    "   \"accessToken\": \"%s\"," \
    "\r\n   \"refreshToken\": \"%s\"\r\n" \
    "}\r\n";  

    char post_data[700];
    memset(post_data,0,sizeof(post_data));
    sprintf(post_data,tmp,getAccessToken(),getRefreshToken());


    char *resource = "POST /sms/api/v1/sp/%s/users/%s/auth/refresh?token=%s  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n" \
    "Content-type: Application/json\r\nContent-Length: %d\r\n\r\n" \
    "{\r\n" \
    "   \"accessToken\": \"%s\"," \
    "\r\n   \"refreshToken\": \"%s\"\r\n" \
    "}\r\n";

    return sprintf( buf, resource,getSpid(),getDeviceUUID(),getRefreshToken(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/,strlen(post_data),getAccessToken(),getRefreshToken());
}

int getPrevDayTime();
float getPrevDayUnit();
char *getPrevDate();

int getMeteringResource(char *buf){

    char*tmp ="{" \
    "    \"usageDuration\":\"%d\", " \
    "    \"powerUnit\":\"%f\"," \
    "    \"date\":\"%s\" " \
    "}";

    char post_data[350];
    memset(post_data,0,sizeof(post_data));
    sprintf(post_data,tmp,getPrevDayTime(),getPrevDayUnit(),getPrevDate());

// /control-monitor/api/v1/sp/%s/unit/%s/device/%s/energy/metering
    char *resource = "POST /control-monitor/api/v1/sp/%s/unit/%s/device/%s/energy/metering  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n" \
    "Content-type: Application/json\r\nContent-Length: %d\r\n\r\n" \
    "{" \
    "    \"usageDuration\":\"%d\", " \
    "    \"powerUnit\":\"%f\"," \
    "    \"date\":\"%s\" " \
    "}";

    return sprintf( buf, resource,getSpid(),getUnitUUID(),getDeviceUUID(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/,strlen(post_data),getPrevDayTime(),getPrevDayUnit(),getPrevDate());
}
//https://srvc.platform.quboworld.com/device/a2053cba-1d4a-4104-a868-88330cde2341/notifications


int getUpdateVersionResource(char *buf){

    //inventoryDeviceType - 1 As THING
    char *tmp ="{\r\n" \
    "   \"macAddress\": \"%s\"," \
    "\r\n   \"inventoryDeviceType\": %d," \
    "\r\n   \"swVersion\": \"%s\"\r\n" \
    "}\r\n";

    char put_data[200];
    memset(put_data,0,sizeof(put_data));
    //sprintf(put_data,tmp,getHUBMacID(),1,getCurrentSWVersion());//GATEWAY = 0 , THING = 1
    // sprintf(put_data,tmp,getHUBMacID(),0,getCurrentSWVersion());//GATEWAY = 0 , THING = 1
    sprintf(put_data,tmp,getHUBMacID(),0,version);//GATEWAY = 0 , THING = 1

    char *resource = "PUT /inventory/api/v1/sp/%s/devices/version  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n" \
    "Content-type: Application/json\r\nContent-Length: %d\r\n\r\n" \
    "{\r\n" \
    "   \"macAddress\": \"%s\"," \
    "\r\n   \"inventoryDeviceType\": %d," \
    "\r\n   \"swVersion\": \"%s\"\r\n" \
    "}\r\n";

    //return sprintf( buf, resource,getSpid(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/,strlen(put_data),getHUBMacID(),0,getCurrentSWVersion());
    return sprintf( buf, resource,getSpid(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/,strlen(put_data),getHUBMacID(),0,version);
    //return "";//sprintf( buf, resource,getSpid(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/,strlen(put_data),getHUBMacID(),0,getCurrentSWVersion());
}

int getOTA_URL(char *buf){
//https://srvc.stage.platform.quboworld.com/ota-service/default/controller/v1/24:14:07:A5:80:E2
    //'https://srvc.dev.platform.quboweb.com:443/lc-ota-service/api/v1/sp/d10e4bfb0153496e8e8bb955f7ebe413/generate?modelNo=HBW01_01'
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI("SSL_REST", "Running firmware version: %s", running_app_info.version);
                    }                
    ESP_LOGI("SSL_REST", "Version Available in Device %s",running_app_info.version);  
    char dv[50] = "";
    memset(dv, 0 ,sizeof(dv));
    strcpy(dv, running_app_info.version);    
    char dav[5][12] = {""};  
    int i = 0;
    char * dVToken = strtok(dv, "_");
    while( dVToken != NULL ) {   
    memset(dav[i], 0 ,sizeof(dav[i]));
    strcpy(dav[i], dVToken);
    dVToken = strtok(NULL, "_");    
    i++;      
    }

    char *resource = "GET /lc-ota-service/api/v1/sp/%s/generate?modelNo=%s_%s&macAddress=%s&swVersion=%s_%s  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Content-type: Application/json\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n\r\n";// <<<< ======== 

    return sprintf( buf, resource,getSpid(),dav[0],dav[1],getHUBMacID(),dav[2],dav[3],gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/);

}

int getOTA_URLV2(char *buf){

    //inventoryDeviceType - 1 As THING
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI("SSL_REST", "Running firmware version: %s", running_app_info.version);
                    }                
    ESP_LOGI("SSL_REST", "Version Available in Device %s",running_app_info.version);  
    char dv[50] = "";
    memset(dv, 0 ,sizeof(dv));
    strcpy(dv, running_app_info.version);    
    char dav[5][12] = {""};  
    int i = 0;
    char * dVToken = strtok(dv, "_");
    while( dVToken != NULL ) {   
    memset(dav[i], 0 ,sizeof(dav[i]));
    strcpy(dav[i], dVToken);
    dVToken = strtok(NULL, "_");    
    i++;      
    }
    //https://srvc.dev.platform.quboweb.com:443/lc-ota-service/api/v1/sp/d10e4bfb0153496e8e8bb955f7ebe413/distribution/device/ota?currentVersion=HLB01_01_01_19_SYSTEM-09W.bin&macAddress=10:97:BD:F1:D2:BD&modelNo=HLB01_01
    char *resource = "GET /lc-ota-service/api/v1/sp/%s/distribution/device/ota?currentVersion=%s&macAddress=%s&modelNo=%s_%s  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Content-type: Application/json\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n\r\n";// <<<< ======== 

    return sprintf( buf, resource,getSpid(),running_app_info.version,getHUBMacID(),dav[0],dav[1],gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/);

}

/*
GET --  
https://srvc.dev.platform.quboweb.com:443/unit-entity-management/api/v1/sp/d10e4bfb0153496e8e8bb955f7ebe413/bulb/3a011bb5-56e1-4c11-9b44-b46a0ddb8ee7/rule

App-Id: M7KPZQV6CHM7HR33253MQH45N4P3VRC6C22USZPFCEBYHL4EGYBRTUG7D5445KNC
Content-Type: application/json
Source: ANDROID
Source-Device-Id: 5a904575dc2456f0
Subscriber-Key: eyJhbGciOiJIUzI1NiJ9.eyJtb2IiOiI1YTkwNDU3NWRjMjQ1NmYwIiwic3ViIjoiVVNFUiIsImlzcyI6ImQxMGU0YmZiMDE1MzQ5NmU4ZThiYjk1NWY3ZWJlNDEzIiwiZXhwIjoxNjY1NDc5ODk4LCJ1c2VyIjoiZTI4ODMzYjItODI4Ny00Zjc4LWIyY2MtYmQ3NmQ1YjhjMDNhIiwiaWF0IjoxNjY1NDcyNjk4fQ.61O39gP8kc0LQ7-Ig5i5S63y3PSK-LP0akBekqquSK4
Token-Type: USER
User-UUID: e28833b2-8287-4f78-b2cc-bd76d5b8c03a
*/

int getScheduleResource(char *buf){

    printf("Getting Schedule ######################");
    // ptz - 2C:D2:6B:41:C9:6D

    //char *resource =  "GET /commissioning/api/v1/sp/"+getSpid()+"/devices?macAddress="+ getHUBMacID();
    // Subscriber-Key: "+getAccessToken() ??? 
    //Token-Type: DEVICE that's why user-UUID: getDeviceUUID() ??? is ok ...

    //replace + with %s or %d
    //https://srvc.dev.platform.quboweb.com/unit-entity-management/api/v2/sp/d10e4bfb0153496e8e8bb955f7ebe413/bulb/d92502a4-f83b-4c90-bd7c-d5dfa7f294f7/rule?pageNumber=5&pageSize=2


    char *resource = "GET /unit-entity-management/api/v2/sp/%s/bulb/%s/rule?pageNumber=%d&pageSize=1  HTTP/1.1\r\n" \
    "Host: %s\r\n" \
    "Content-type: Application/json\r\n" \
    "Subscriber-Key: %s\r\n" \
    "Source-Device-Id: HSP_%s\r\n" \
    "Token-Type: DEVICE\r\n" \
    "user-UUID: %s\r\n\r\n";// <<<< ========
    return sprintf( buf, resource,getSpid(),getDeviceUUID() ,getdownloadedRecords(),gethttpURL(),getAccessToken(),getHUBMacID(),getGWUUID()/*getDeviceUUID()*/);
}



extern int getInternetStatus();

//char buf[BUFFER_SIZE + 1];//use malloc not global
extern int getWifiConnectStatus();

// #define WEB_SERVER "srvc.dev.platform.quboweb.com"
// #define WEB_PORT "443"
// #define WEB_URL "https://srvc.dev.platform.quboweb.com/unit-entity-management/api/v1/sp/d10e4bfb0153496e8e8bb955f7ebe413/device/3d7cb920-915d-4e23-a62e-0710c122904b/lock"

static const char *TAG = "SSL_REST";

// static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
//     "Host: "WEB_SERVER"\r\n"
//     "User-Agent: esp-idf/1.0 esp32\r\n"
//     "Content-type: Application/json\r\n"
//     "Subscriber-Key: eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJERVZJQ0UiLCJ1bml0IjoiYTk2YmYxMmYtYTk2Mi00MmJlLWFmZGItMGMwZGU3OTMwNTFhIiwiaXNzIjoiZDEwZTRiZmIwMTUzNDk2ZThlOGJiOTU1ZjdlYmU0MTMiLCJleHAiOjE2NjEzMTc2MDcsImRldmljZSI6IjA2MTE0ZTEyLTNkN2EtNDBiOC05YjA0LTZiM2FiZDg4YWYzOCIsImlhdCI6MTY1ODcyNTYwN30.yvGkXfAITDwkvRNuNgc1wf_KK8CbX_nLmy9uDYIq0qk\r\n"
//     "Source-Device-Id: HSP_DC:0D:30:80:8C:35\r\n"
//     "Token-Type: DEVICE\r\n"
//     "user-UUID: 06114e12-3d7a-40b8-9b04-6b3abd88af38\r\n"
//     "\r\n";
char *resp_code = NULL;
char* rest_call(enum rest_call_type rtype)
//static void https_get_task(void *pvParameters)
{
    //tmp change later
    //return NULL;

    char buf[2048];
    int ret, flags, len=0;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;
    int counter = 0;
    while(!getInternetStatus()){
        printf("\r\n waiting for internet connection success ... \r\n");
        vTaskDelay(2000);
        counter++;
        if(counter > 5)
        {
            return NULL;
        }
    }

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(TAG, "Seeding the random number generator");
    ESP_LOGI(TAG, "1.Free heap size: %d\n", esp_get_free_heap_size());
    mbedtls_ssl_config_init(&conf);
    //ESP_LOGI(TAG, "2.Free heap size: %d\n", esp_get_free_heap_size());
    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }
    //ESP_LOGI(TAG, "3. Free heap size: %d\n", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Attaching the certificate bundle...");

    // ret = esp_crt_bundle_attach(&conf);

    // if(ret < 0)
    // {
    //     ESP_LOGE(TAG, "esp_crt_bundle_attach returned -0x%x\n\n", -ret);
    //     abort();
    // }


    //     ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
    //                                  server_root_cert_pem_end-server_root_cert_pem_start);

    //     if(ret < 0)
    //     {
    //         ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
    //         abort();
    //     }
    // ESP_LOGI(TAG, "4. Free heap size: %d\n", esp_get_free_heap_size());

    ESP_LOGI(TAG, "Setting hostname for TLS session...");

    char *host = gethttpURL();

     /* Hostname set here should match CN in server certificate */
    //if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
    if((ret = mbedtls_ssl_set_hostname(&ssl, host)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");
    //ESP_LOGI(TAG, "5. Free heap size: %d\n", esp_get_free_heap_size());
    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }
    //ESP_LOGI(TAG, "6.Free heap size: %d\n", esp_get_free_heap_size());
    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
       a warning if CA verification fails but it will continue to connect.

       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif
    //ESP_LOGI(TAG, "7. Free heap size: %d\n", esp_get_free_heap_size());
    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }
// 22 kb 7 - 8 point

    mbedtls_net_init(&server_fd);
    ESP_LOGI(TAG, "8.Free heap size: %d\n", esp_get_free_heap_size());
    //ESP_LOGI(TAG, "Connecting to %s:%s...", WEB_SERVER, WEB_PORT);

    ESP_LOGI(TAG, "Connecting to %s:%s...", host, getHttpPort());
    // if ((ret = mbedtls_net_connect(&server_fd, WEB_SERVER,
    //                               WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
    if ((ret = mbedtls_net_connect(&server_fd, host,
                                  getHttpPort(), MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
        goto exit;
    }

    ESP_LOGI(TAG, "Connected.");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
    ESP_LOGI(TAG, "9. Free heap size: %d\n", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
            goto exit;
        }
    }
    //ESP_LOGI(TAG, "10. Free heap size: %d\n", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
    {
        /* In real life, we probably want to close connection if ret != 0 */
        ESP_LOGW(TAG, "Failed to verify peer certificate!");
        bzero(buf, sizeof(buf));
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
        ESP_LOGW(TAG, "verification info: %s", buf);
    }
    else {
        ESP_LOGI(TAG, "Certificate verified.");
    }

    ESP_LOGI(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

    ESP_LOGI(TAG, "Writing HTTP request...");
    ESP_LOGI(TAG, "11. Free heap size: %d\n", esp_get_free_heap_size());

    //size_t written_bytes = 0;

    memset( buf, 0, sizeof( buf ) );

    /*if(rtype == ALEXA)
        len = getAlexaResource((char *)buf,controlType());
    else */if(rtype == INVENTORY)
        len = getInventoryResource((char *)buf);
    else if(rtype == ACCESSTOKEN)
        len = getAccessTokenResource((char *)buf);
    else if(rtype == UPDATEVERSION)
        len = getUpdateVersionResource((char *)buf);
    else if (rtype == GETSCHEDULES)
        len = getScheduleResource((char*)buf);
    else if (rtype == GET_OTA_URL)
        len = getOTA_URL((char*)buf);
    else if (rtype == GET_OTA_URLV2)
        len = getOTA_URLV2((char*)buf);
    else if (rtype == UPDATE_METERING)
        len = getMeteringResource((char*)buf);
    //do {
        // ret = mbedtls_ssl_write(&ssl,
        //                         (const unsigned char *)REQUEST + written_bytes,
        //                         strlen(REQUEST) - written_bytes);
        while( ( ret = mbedtls_ssl_write( &ssl, (unsigned char *)buf, len ) ) <= 0 )
        {
            if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                ESP_LOGI(TAG, " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
                break;
            }
        }
        //len = ret;
        // mbedtls_printf( " %d bytes written\n\n%s", len, (char *) buf );
       //printf( " %d bytes written\n\n%s", len, (char *) buf );
        // if (ret >= 0) {
        //     ESP_LOGI(TAG, "%d bytes written", ret);
        //     written_bytes += ret;
        // } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
        //     ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
        //     goto exit;
        // }
    //} while(written_bytes < strlen(REQUEST));
    ESP_LOGI(TAG, "12. Free heap size: %d\n", esp_get_free_heap_size());
    // ESP_LOGI(TAG, "Reading HTTP response...");
    printf("\nReading HTTP response...\n");

   // do
    {
        len = sizeof(buf) - 1;
        // bzero(buf, sizeof(buf));
         memset( buf, 0, sizeof( buf ) );
        ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

        if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "11mbedtls_ssl_read returned -0x%x", -ret);
           // continue;
        }

        if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            ESP_LOGE(TAG, "22mbedtls_ssl_read returned -0x%x", -ret);
            ret = 0;
           // break;
        }

        if(ret < 0)
        {
            ESP_LOGE(TAG, "33mbedtls_ssl_read returned -0x%x", -ret);
            //break;
        }

        if(ret == 0)
        {
            ESP_LOGI(TAG, "connection closed");
           // break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        // for(int i = 0; i < len; i++) {
        //     putchar(buf[i]);
        // }
    } //while(1);
    ESP_LOGI(TAG, "13. Free heap size: %d\n", esp_get_free_heap_size());
    mbedtls_ssl_close_notify(&ssl);

exit:
    mbedtls_ssl_session_reset(&ssl);
    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free( &cacert );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    //parse buff get json
    ESP_LOGI(TAG, "\n\r parse buff get json .... \n\r");
    printf("\n\r parse buff get json ....[%s] \n\r",buf);
    resp_code = jsonFromHtmlResponse((char *)buf, rtype);

    ESP_LOGI(TAG, "14. Free heap size: %d\n", esp_get_free_heap_size());
    return resp_code;
}
