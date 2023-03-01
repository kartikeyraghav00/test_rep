#include <stdio.h>
#include "esp_spiffs.h"
#include "esp_log.h"

#define TAG "spiffs"

void app_main(void)
{
  esp_vfs_spiffs_conf_t config = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true,
  };
  esp_vfs_spiffs_register(&config);

  FILE *file = fopen("/spiffs/data.txt", "r");
  if(file ==NULL)
  {
    ESP_LOGE(TAG,"File does not exist!");
  }
  else 
  {
    char line[256];
    while(fgets(line, sizeof(line), file) != NULL)
    {
      printf(line);
    }
    fclose(file);
  }
  esp_vfs_spiffs_unregister(NULL);
}
