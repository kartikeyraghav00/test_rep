/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#if 0
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "errno.h"
#include "json_parse.h"

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

static const char *TAG = "ota_BULB";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
/*extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
*/
#define OTA_URL_SIZE 256
char urlForOta[1024] = "";

bool
checkSotwareVersion (char *systemVersion, char *cloudVersion);

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static void ota_example_task(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example task");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

  /*  if(strlen(getStaticOTAURL() > 0)
    {
       esp_http_client_config_t config = {
        .url = getStaticOTAURL(),
        //.cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
    }; 
    }
    else*/
    //{

    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

//    char urlForOta[1024] = "";
//    memset(urlForOta,0,sizeof(urlForOta));
//    if(strstr(running_app_info.version,"SYSTEM-10A"))
//   { 
//        strcpy(urlForOta,"http://tempheroiotapp.s3.ap-south-1.amazonaws.com/HSP02_01_10A.bin");        
//        // strcpy(urlForOta,"http://192.168.0.128:8000/HSP02_01_10A.bin");        
//    }    
//    else if(strstr(running_app_info.version,"SYSTEM-16A"))
//    {
//        // strcpy(urlForOta,"http://192.168.0.128:8000/HSP10_01_16A.bin");
//        strcpy(urlForOta,"http://tempheroiotapp.s3.ap-south-1.amazonaws.com/HSP10_01_16A.bin");
//    }


    esp_http_client_config_t config = {
        .url = urlForOta,
        //.cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
    };
//}
    config.skip_cert_common_name_check = true;

    ESP_LOGI(TAG, "URL for Downloading OTA %s",config.url);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);
                    

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            //infinite_loop();
                            goto EXIT_OTA;
                        }
                    }

                   /* if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        //infinite_loop();
                        goto EXIT_OTA;
                    }*/

                    if(!checkSotwareVersion(running_app_info.version, new_app_info.version))
                    {
                        ESP_LOGW(TAG, "There is no Software Available for this Device");
                        ESP_LOGW(TAG, "Current Version Available in Device [ %s ]",running_app_info.version);
                        ESP_LOGW(TAG, "Upgrade Version Available in Cloud  [ %s ]",new_app_info.version);
                        http_cleanup(client);
                        //infinite_loop();
                        goto EXIT_OTA;
                    }

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    esp_ota_abort(update_handle);
                    task_fatal_error();
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                esp_ota_abort(update_handle);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        esp_ota_abort(update_handle);
        task_fatal_error();
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();

    EXIT_OTA:
    vTaskDelete(NULL);
    return ;
}

static bool diagnostic(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << 4);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    bool diagnostic_is_ok = gpio_get_level(4);

    gpio_reset_pin(4);
    return diagnostic_is_ok;
}

bool
checkSotwareVersion (char *systemVersion, char *cloudVersion)
{    
  ESP_LOGI(TAG, "Version Available In Cloud  %s",cloudVersion);  
  ESP_LOGI(TAG, "Version Available in Device %s",systemVersion);
  char cv[50] = "";
  char dv[50] = "";
  memset(dv, 0 ,sizeof(dv));
  memset(cv, 0 ,sizeof(cv));
  strcpy(cv, cloudVersion);
  strcpy(dv, systemVersion);  
  char cav[5][12] = {""};
  char dav[5][12] = {""};  
  char *cvtoken = strtok(cv, "_");
    int i = 0;
    while( cvtoken != NULL ) {    
    memset(cav[i], 0 ,sizeof(cav[i]));
    strcpy(cav[i], cvtoken);
     cvtoken = strtok(NULL, "_");     
     i++;       
     }
     
     i = 0;
    char * dVToken = strtok(dv, "_");
    while( dVToken != NULL ) {   
     memset(dav[i], 0 ,sizeof(dav[i]));
    strcpy(dav[i], dVToken);
    dVToken = strtok(NULL, "_");    
    i++;      
    }
    
    if(strcmp(cav[0],dav[0]) == 0 && strcmp(cav[1],dav[1]) == 0 && strcmp(cav[4],dav[4]) == 0 )
    {
        ESP_LOGI(TAG, "Compatiable Software Check Software version");
        if(atoi(cav[2]) > atoi(dav[2]) || atoi(cav[3]) > atoi(dav[3]))
        {
            ESP_LOGI(TAG, " New Software Version Available Lets Start OTA -------------------");
            return true;
        }
    }
    
    return false;
    
    
    
}

void startOTA(char * ota)
{
    ESP_LOGI(TAG, "OTA app_main start");
    ESP_LOGI(TAG, "OTA URL  %s",ota);
    memset(urlForOta ,0 ,sizeof(urlForOta));
    strcpy(urlForOta,ota);

    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
   }  
   

    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
}
#endif 

/* Advanced HTTPS OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
//#include "esp_crt_bundle.h"
#include "json_parse.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

static const char *TAG = "advanced_https_ota_example";
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256
char urlForOta[1500] = "";

bool
checkSotwareVersion (char *systemVersion, char *cloudVersion)
{
  ESP_LOGI(TAG, "Version Available In Cloud  %s",cloudVersion);
  ESP_LOGI(TAG, "Version Available in Device %s",systemVersion);
  printf("\nVersion Available In Cloud  %s\n",cloudVersion);
  printf("\nVersion Available in Device %s\n",systemVersion);

  char cv[50] = "";
  char dv[50] = "";
  memset(dv, 0 ,sizeof(dv));
  memset(cv, 0 ,sizeof(cv));
  strcpy(cv, cloudVersion);
  strcpy(dv, systemVersion);
  char cav[5][12] = {""};
  char dav[5][12] = {""};
  char *cvtoken = strtok(cv, "_");
    int i = 0;
    while( cvtoken != NULL ) {
    memset(cav[i], 0 ,sizeof(cav[i]));
    strcpy(cav[i], cvtoken);
     cvtoken = strtok(NULL, "_");
     i++;
     }

     i = 0;
    char * dVToken = strtok(dv, "_");
    while( dVToken != NULL ) {
     memset(dav[i], 0 ,sizeof(dav[i]));
    strcpy(dav[i], dVToken);
    dVToken = strtok(NULL, "_");
    i++;
    }

    if(strcmp(cav[0],dav[0]) == 0 && strcmp(cav[1],dav[1]) == 0 && strcmp(cav[4],dav[4]) == 0 )
    {
        ESP_LOGI(TAG, "Compatiable Software Check Software version");
        if(atoi(cav[2]) > atoi(dav[2]) || atoi(cav[3]) > atoi(dav[3]))
        {
            ESP_LOGI(TAG, " New Software Version Available Lets Start OTA -------------------");
            return true;
        }
    }

    return false;
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    if(checkSotwareVersion(running_app_info.version,new_app_info->version))
    {
        return ESP_OK;
    }

    return ESP_FAIL;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

void advanced_ota_example_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting Advanced OTA example");
    vTaskDelay(5000/portTICK_PERIOD_MS);
    esp_err_t ota_finish_err = ESP_OK;
    //ESP_LOGI(TAG,"Performing OTA On URL ------  %s",urlForOta);
    printf("\nPerforming OTA On URL ------  %s\n",urlForOta);
    esp_http_client_config_t config = {
        .url = urlForOta,
        .skip_cert_common_name_check = true,
        //.use_global_ca_store = true,
        .buffer_size = 1024 * 4,
        //.crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 100000,
        .keep_alive_enable = true,
    };

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
        .partial_http_download = true,
        .max_http_request_size = 1024 * 4,

    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        printf("\nESP HTTPS OTA Begin failed\n");
        vTaskDelete(NULL);
        goto ota_end;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        printf("\nesp_https_ota_read_img_desc failed\n");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "image header verification failed");
        printf("\nimage header verification failed\n");
        goto ota_end;
    }
    int current = 0;
    int total = esp_https_ota_get_image_size(https_ota_handle);
    float percentage = 0;
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        current = esp_https_ota_get_image_len_read(https_ota_handle);
        char status[5];
        percentage = (float)current / total * 100;
        memset(status,0,sizeof(status));
        sprintf(status, "O_%d",(int)percentage);
        setCurrentStatus(status);
        //ESP_LOGI(TAG, "Downloaded [%d]  Total [%d]  Percentage[%d] Memory Available[%d]", current,total,(int)percentage,esp_get_free_heap_size());
        printf("\nDownloaded [%d]  Total [%d]  Percentage[%d] Memory Available[%d]\n", current,total,(int)percentage,esp_get_free_heap_size());

        //ESP_LOGI(TAG, "Image bytes read: [%d] / [%d]", esp_https_ota_get_image_len_read(https_ota_handle),esp_https_ota_get_image_size(https_ota_handle));
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
        printf("\nComplete data was not received.\n");
    } else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            //ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            printf("ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                printf("\nImage validation failed, image is corrupted\n");
            }
            // ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            printf("\nESP_HTTPS_OTA upgrade failed 0x%x\n", ota_finish_err);
            esp_restart();
            vTaskDelete(NULL);

        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
    esp_restart();
    vTaskDelete(NULL);
}

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[32 * 2 + 1];
    hash_print[32 * 2] = 0;
    for (int i = 0; i < 32; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

void startOTA(char * ota) // enable bundle in menuconfig uncomment from CMake
{
    return;

    
    ESP_LOGI(TAG, "OTA app_main start");
    ESP_LOGI(TAG, "OTA URL  %s",ota);
    //char * ota1 = "http://192.168.0.138:8000/HSP10_01_16A.bin";
    memset(urlForOta ,0 ,sizeof(urlForOta));
    strcpy(urlForOta,ota);
    xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, NULL);
}
