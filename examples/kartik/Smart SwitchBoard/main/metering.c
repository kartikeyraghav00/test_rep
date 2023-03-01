#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <stdio.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include "freertos/queue.h"
#include "json_parse.h"
#include <sys/unistd.h>
//#include "driver/timer.h"
#include "driver/gptimer.h"

#include "metering.h"




//*******************************************************************
//*******************CONFIGURATIONS**********************************

PowerMonitor powerReadData;
DeviceData deviceData;
MessageData messageData;
Utils utilsData;

char *swver = NULL;

void setChnageStateValues(unsigned char index, unsigned char state, unsigned char dim);

static const char *TAG = "scan";
#define ESP_INTR_FLAG_DEFAULT 0

// int currentValues[1000],currentCount=0,currentFlag = 0;

unsigned long gpioFlag = 0;

unsigned long PresentValues[2], PrviousValues[2], Difference[2], PresentDiff[2], timings[2];
float frequency[2];
int toggleLED = 0;
int iocount = 0;
extern int pert_update_type;

bool InCaliberation = false;

//#define NODE_COUNT 2

bool WS_UPDATE = true;



// static xQueueHandle s_timer_queue;
static QueueHandle_t s_timer_queue;

uint32_t t_current_us = 0, t_previous_us = 0, t_difference_us = 0, PrevPluctuation = 0, percentOfFluctuation = 0;
uint32_t t_current_power = 0, t_previous_power = 0, t_difference_power = 0;
unsigned int readType = 0;
int32_t power_pulse_count = 0;
int plugPrevState = 0;
float oldActivePower = 0;
int64_t powerUpdateTime = 0, energyUpdateTime = 0;

int32_t powerLowest = 36000000, currentLowest = 36000000, voltageLowest = 36000000;

int voltageCutoffPulse = 0, currentCutoffPulse = 0, PowerCutoffPulse = 0;

QueueHandle_t cap_queue;
QueueHandle_t gpio_evt_queue;
#define STRUCTURES "pert_data"

float PowerFactor = 1, ActivePower = 1, ReactivePower = 1;

extern int ledIndicationPaired;

unsigned char LED_PREV_STATE = 0;

int plugStatusChangedEvent = 0;
int flag_smrtconfig = 0;
Fluctuation fluctuation;
typedef struct
{
    float F_power;
    float F_current;
    float F_voltage;
    int flag;
} Caliberation;

Caliberation fistTimeCaliberation;
Caliberation runningData, previousData;

int pushButtonStart = 0, pushButtonStart2 = 0;
int ValuesUpdateFlagPower = 0, ValuesUpdateFlagCurrent = 0;
int voltage270Pulse = 455, voltage90Pulse = 1516, current10000Pulse10Amp = 510;

int currentMeasureType = 2;
int powerCutTimingVoltage = 0, powerCutTimingCurrent = 0, deviceRebooted = 0;

BaseType_t high_task_awoken = pdFALSE;

uint32_t PowerUpdateCount = 0;

int WriteFile(void *value, const char *key, size_t length)
{
    int ret = -1;
    nvs_handle handle;
    // printf("----------------------\r\n");
    esp_err_t err = nvs_open(STRUCTURES, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "\r\n*****************\r\nError (%d) opening NVS handle for !\r\n*****************\r\n", err);
        return -1;
    }
    else
    {
        // printf("==============================\r\n");
        // printf("%s %d\r\n",key,length);
        // nvs_set_blob(handle, key, cal_data, sizeof(*cal_data));
        err = nvs_set_blob(handle, key, value, length);
        // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "\r\n*****************\r\nError loading data %d!\r\n*****************\r\n", err);
            ret = -1;
        }
        else
        {
            ESP_LOGI(TAG, "\r\n*****************\r\nSuccess loading data %d!\r\n********************\r\n", err);
            nvs_commit(handle);
            ret = 0;
        }
        // printf("????????????????????????????????\r\n");
        nvs_close(handle);
    }
    return ret;
}

int ReadFile(void *value, const char *key, size_t length)

{
    int ret = -1;
    nvs_handle handle;
    esp_err_t err = nvs_open(STRUCTURES, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "\r\n*****************\r\nError (%d) opening NVS handle for\r\n*****************\r\n", err);
        return -1;
    }
    else
    {
        err = nvs_get_blob(handle, key, value, &length);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "\r\n*****************\r\nNVS handle opened length: %d ********************* \r\n", length);
            ret = 0;
        }
        else
        {
            ESP_LOGE(TAG, "\r\n*****************\r\nError getting structure data 0x%X!\r\n*****************\r\n", err);
        }
        nvs_close(handle);
    }
    return ret;
}

// static bool IRAM_ATTR timer_group_isr_callback(void *args)
// {
//     example_timer_info_t *info = (example_timer_info_t *)args;
//     uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx);
//     if (!info->auto_reload)
//     {
//         timer_counter_value += info->alarm_interval * TIMER_SCALE;
//         timer_group_set_alarm_value_in_isr(info->timer_group, info->timer_idx, timer_counter_value);
//     }

// #ifdef NODE_4R_M
// if(InCaliberation == false){
//     //CheckPowerMonitorData();
// }
// #endif
//     PowerUpdateCount = PowerUpdateCount + 1;
//     return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
// }

static bool IRAM_ATTR example_timer_on_alarm_cb_v1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // stop timer immediately
    gptimer_stop(timer);
    // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    PowerUpdateCount = PowerUpdateCount + 1;
    //CheckPowerMonitorData();
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

static void example_tg_timer_init(/*int group, int timer, bool auto_reload,*/ int timer_interval_sec)
{
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };

#if 1
    gptimer_new_timer(&config, &gptimer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb_v1,
    };
    gptimer_register_event_callbacks(gptimer, &cbs, s_timer_queue);

    ESP_LOGI(TAG, "Enable timer");
    gptimer_enable(gptimer);

    ESP_LOGI(TAG, "Start timer, stop it at alarm event");
    gptimer_alarm_config_t alarm_config1 = {
        .alarm_count = timer_interval_sec * TIMER_SCALE,//1000000, // period = 1s
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(gptimer, &alarm_config1);
    gptimer_start(gptimer);
#else    
    timer_init(group, timer, &config);
    timer_set_counter_value(group, timer, 0);
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE/*in MS*/);
    timer_enable_intr(group, timer);
    example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
    timer_info->timer_group = group;
    timer_info->timer_idx = timer;
    timer_info->auto_reload = auto_reload;
    timer_info->alarm_interval = timer_interval_sec;
    timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0);
    timer_start(group, timer);
#endif
}



static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    if (gpio_num == VOLTAGE_CURRENT)
    {
        if (powerReadData.MeasureType == VOLTAGE)
        {
            powerReadData.VoltageTimePres = esp_timer_get_time();
            if (powerReadData.VoltageTimePres - powerReadData.VoltageTimePrev > 50)
            {
                powerReadData.VoltageTimeDif[powerReadData.VoltageTimeIndex % 5] = powerReadData.VoltageTimePres - powerReadData.VoltageTimePrev;
                powerReadData.VoltageTimeIndex = powerReadData.VoltageTimeIndex + 1;
                if (powerReadData.VoltageTimeIndex > 4)
                {
                    powerReadData.VoltageTimeIndex = 0;
                }
                powerReadData.VoltageReadFlag = powerReadData.VoltageReadFlag + 1;
            }
            powerReadData.VoltageTimePrev = powerReadData.VoltageTimePres;
        }
        else if (powerReadData.MeasureType == CURRENT)
        {
            powerReadData.CurrentTimePres = esp_timer_get_time();
            if (powerReadData.CurrentTimePres - powerReadData.CurrentTimePrev > 50)
            {
                powerReadData.CurrentTimeDif[powerReadData.CurrentTimeIndex % 5] = powerReadData.CurrentTimePres - powerReadData.CurrentTimePrev;
                powerReadData.CurrentTimeIndex = powerReadData.CurrentTimeIndex + 1;
                if (powerReadData.CurrentTimeIndex > 4)
                {
                    powerReadData.CurrentTimeIndex = 0;
                }
                powerReadData.CurrentReadFlag = powerReadData.CurrentReadFlag + 1;
            }
            powerReadData.CurrentTimePrev = powerReadData.CurrentTimePres;
        }
    }
    else if (gpio_num == POWER_PIN)
    {
        powerReadData.PowerTimePres = esp_timer_get_time();
        if (powerReadData.PowerTimePres - powerReadData.PowerTimePrev > 50)
        {
            powerReadData.PowerTimeDif[powerReadData.PowerTimeIndex % 5] = powerReadData.PowerTimePres - powerReadData.PowerTimePrev;
            powerReadData.PowerTimeIndex = powerReadData.PowerTimeIndex + 1;
            if (powerReadData.PowerTimeIndex > 4)
            {
                powerReadData.PowerTimeIndex = 0;
            }
            powerReadData.PowerReadFlag = 1;
            powerReadData.powerCount++;
        }
        powerReadData.PowerTimePrev = powerReadData.PowerTimePres;
    }
}
char* getCurrentSWVersion();
void CheckPowerMonitorData(){
     bool flag = false;
     unsigned char similarValuesC[5],similarValuesV[5],similarValuesP[5];
     if (powerReadData.MeasureType == VOLTAGE){
           powerReadData.MeasureType = CURRENT;
            gpio_set_level(SELECT_PIN, CURRENT);
           powerReadData.CurrentReadFlag = 0;
           powerReadData.PowerReadFlag = 0;
           powerReadData.timerCount = 0;
           powerReadData.power_timerCount = 0;
     }else{
           if(powerReadData.CurrentReadFlag > 10 || powerReadData.timerCount > 40){
                   if(powerReadData.timerCount > 40 && powerReadData.PowerReadFlag == 0){
                        for(int i = 0 ; i < 5; i++){
                                powerReadData.CurrentTimeDif[i] = 20000000;
                        }
                       powerReadData.ActiveTemp = 0 ;
                       messageData.ActivePower  = 0;
                   }
                   powerReadData.MeasureType = VOLTAGE;
                   gpio_set_level(SELECT_PIN, VOLTAGE);
           }

           if(powerReadData.power_timerCount > 20 && powerReadData.PowerReadFlag == 0){
                    for(int i = 0 ; i < 5; i++){
                         powerReadData.PowerTimeDif[i] = esp_timer_get_time() - powerReadData.PowerTimePres;
                    }
                    powerReadData.power_timerCount  = 0;
           }

           powerReadData.timerCount = powerReadData.timerCount + 1;
           powerReadData.power_timerCount = powerReadData.power_timerCount + 1;

     }

     powerReadData.paramFlag = powerReadData.paramFlag + 1;
     if(powerReadData.paramFlag > 10){
           powerReadData.paramFlag = 0;
           powerReadData.voltageTemp = 0;
           powerReadData.currentTemp = 0;
           powerReadData.ActiveTemp = 0;
           for(int i = 0 ; i < 5; i++){
                    similarValuesC[i] = 0;
                    similarValuesV[i] = 0;
                    similarValuesP[i] = 0;
                    for(int j = 0 ; j < 5; j++){
                          if(i != j){
                               float percent = (float) abs(powerReadData.CurrentTimeDif[i] - powerReadData.CurrentTimeDif[j]);
                               percent = percent/ (float) powerReadData.CurrentTimeDif[i];
                               percent = percent * 100;
                               if(percent < 10){
                                    similarValuesC[i] = similarValuesC[i] + 1;
                               }
                               percent = (float) abs(powerReadData.VoltageTimeDif[i] - powerReadData.VoltageTimeDif[j]);
                               percent = percent/ (float) powerReadData.VoltageTimeDif[i];
                               percent = percent * 100;
                               if(percent < 10){
                                    similarValuesV[i] = similarValuesV[i] + 1;
                               }
                               percent = (float) abs(powerReadData.PowerTimeDif[i] - powerReadData.PowerTimeDif[j]);
                               percent = percent/ (float) powerReadData.PowerTimeDif[i];
                               percent = percent * 100;
                               if(percent < 10){
                                    similarValuesP[i] = similarValuesP[i] + 1;
                               }
                          }
                    }
            } 
            int data_bestP = similarValuesP[0];
            int data_bestC = similarValuesC[0];
            int data_bestV = similarValuesV[0];
            powerReadData.ActiveTemp = powerReadData.PowerTimeDif[0];
            powerReadData.currentTemp = powerReadData.CurrentTimeDif[0];
            powerReadData.voltageTemp = powerReadData.VoltageTimeDif[0];
            for(int i = 1 ; i < 5; i++){
                    if(data_bestP <  similarValuesP[i]){
                         powerReadData.ActiveTemp = powerReadData.PowerTimeDif[i];
                         data_bestP =  similarValuesP[i];
                    }
                    if(data_bestC <  similarValuesC[i]){
                         powerReadData.currentTemp = powerReadData.CurrentTimeDif[i];
                         data_bestC =  similarValuesC[i];
                    }
                    if(data_bestV <  similarValuesV[i]){
                         powerReadData.voltageTemp = powerReadData.VoltageTimeDif[i];
                         data_bestV =  similarValuesV[i];
                    }
            }
            if(powerReadData.currentTemp != 0){
                   powerReadData.currentTemp = (1000000.0 / powerReadData.currentTemp) * fistTimeCaliberation.F_current;
            }
            if(powerReadData.voltageTemp != 0){
                //printf("b:voltageTemp %f %f\r\n",powerReadData.voltageTemp);
                powerReadData.voltageTemp = (1000000.0 / powerReadData.voltageTemp) * fistTimeCaliberation.F_voltage;
                //printf("a:voltageTemp %f %f\r\n",powerReadData.voltageTemp);
            }
            
            if(powerReadData.ActiveTemp != 0){
                  powerReadData.ActiveTemp = (1000000.0 / powerReadData.ActiveTemp) * fistTimeCaliberation.F_power;
            }
            

            if( powerReadData.ActiveTemp < 3){
                    powerReadData.currentTemp = 0;
                    powerReadData.ActiveTemp = 0;
            }
            powerReadData.ApparentTemp = (powerReadData.currentTemp * powerReadData.voltageTemp) / 1000.0;
            if(powerReadData.ActiveTemp != 0 && powerReadData.ApparentTemp != 0){
                    powerReadData.FactorTemp = powerReadData.ActiveTemp / powerReadData.ApparentTemp;
                    if(powerReadData.FactorTemp > 1){
                          powerReadData.FactorTemp = 1;
                    }
            }else{
                    powerReadData.FactorTemp = 0;
            }
            float a2 = powerReadData.ApparentTemp * powerReadData.ApparentTemp;
            float a1 = powerReadData.ActiveTemp * powerReadData.ActiveTemp;
            a2 = a2 - a1;
            a2 = abs(a2);
            powerReadData.ReactiveTemp = sqrt(a2);

            //printf("===== Voltage %f Current %f ActivePower %f apparent %f reactive %f factor %f\r\n",powerReadData.voltageTemp,powerReadData.currentTemp,powerReadData.ActiveTemp,powerReadData.ApparentTemp,powerReadData.ReactiveTemp,powerReadData.FactorTemp);
            //###################CHECK VOLTAGE CHANGES#######################
            
            if(powerReadData.voltageTemp != messageData.Voltage){  
                    float change = abs(messageData.Voltage - powerReadData.voltageTemp);
                    //printf("VOltage change DATA %f %f\r\n",powerReadData.voltageTemp,messageData.Voltage);
                    if(change > 3){        
                            fluctuation.voltage_fluctuation_count = fluctuation.voltage_fluctuation_count + 1;
                           // printf("VOltage change DATA %f %f\r\n",powerReadData.voltageTemp,messageData.Voltage);
                            //printf("11  fluctuation clount %d\r\n",fluctuation.voltage_fluctuation_count);
                            if(fluctuation.voltage_fluctuation_count > 3){
                                messageData.Voltage = powerReadData.voltageTemp;
                                fluctuation.voltage_fluctuation_count = 0;
                                //printf("flag truw at 66666\r\n");
                                flag = true;
                           }
                   }else{
                       fluctuation.voltage_fluctuation_count = 0;
                   }
            }
             //###################CHECK CURRENT CHANGES#######################
            if(powerReadData.currentTemp != messageData.Current){
                
                    //printf("Current change DATA %f %f\r\n",powerReadData.currentTemp,messageData.Current);
                    if(powerReadData.currentTemp == 0 && messageData.Current == 0){
                        
                            fluctuation.current_fluctuation_count = 0;
                    }else if(powerReadData.currentTemp == 0 || messageData.Current == 0){
                            fluctuation.current_fluctuation_count = fluctuation.current_fluctuation_count + 1;
                            //printf("Current change DATA %f %f\r\n",powerReadData.currentTemp,messageData.Current);
                            //printf("222 l fluctuation clount %d\r\n",fluctuation.current_fluctuation_count);
                            if(fluctuation.current_fluctuation_count > 3){
                                   fluctuation.current_fluctuation_count = 0;
                                    messageData.Current = powerReadData.currentTemp;
                                    //printf("flag truw at 5555\r\n");
                                    flag = true;
                            }
                    }else{
                            float change = abs(messageData.Current - powerReadData.currentTemp);
                            change = (change / messageData.Current) * 100; 
                            if(messageData.Current < 1000 && change > 50){
                                fluctuation.current_fluctuation_count = fluctuation.current_fluctuation_count + 1;
                                //printf("222  fluctuation clount %d\r\n",fluctuation.current_fluctuation_count);
                               if(fluctuation.current_fluctuation_count > 3){
                                    fluctuation.current_fluctuation_count = 0;
                                    messageData.Current = powerReadData.currentTemp;
                                    //printf("flag truw at 4444\r\n");
                                    flag = true;
                               }
                            }else if(messageData.Current > 1000 && change > 30){
                                
                                fluctuation.current_fluctuation_count = fluctuation.current_fluctuation_count + 1;
                                //printf("222==  fluctuation clount %d\r\n",fluctuation.current_fluctuation_count);
                               if(fluctuation.current_fluctuation_count > 3){
                                    fluctuation.current_fluctuation_count = 0;
                                    messageData.Current = powerReadData.currentTemp;
                                    //printf("flag truw at 3333\r\n");
                                    flag = true;
                               }
                            }else{
                                
                                   fluctuation.current_fluctuation_count = 0;
                            }
                    }
                
            }  
            //###################CHECK ACTIVE CHANGES#######################
            //printf("Check power %f VS %f \r\n",powerReadData.ActiveTemp,messageData.ActivePower);
            if(powerReadData.ActiveTemp != messageData.ActivePower){
                 //printf("Check power %f VS %f \r\n",powerReadData.ActiveTemp,messageData.ActivePower);
                        if(powerReadData.ActiveTemp == 0 && messageData.ActivePower == 0){
                                fluctuation.power_fluctuation_count = 0;
                        }else if(powerReadData.ActiveTemp == 0 || messageData.ActivePower == 0){
                            fluctuation.power_fluctuation_count = fluctuation.power_fluctuation_count + 1;
                            //printf("333 2 fluctuation clount %d\r\n",fluctuation.power_fluctuation_count);
                            if(fluctuation.power_fluctuation_count > 3){
                                    fluctuation.power_fluctuation_count = 0;
                                    messageData.ActivePower = powerReadData.ActiveTemp;
                                    flag = true;
                                    //printf("flag truw at 1111\r\n");
                            }
                        }else{
                            float change = abs(messageData.ActivePower - powerReadData.ActiveTemp);
                            change = (change / messageData.ActivePower) * 100; 
                            if(messageData.ActivePower > 20 && messageData.ActivePower < 1000 && change > 20){
                                fluctuation.power_fluctuation_count = fluctuation.power_fluctuation_count + 1;
                                //printf("333 1  fluctuation clount %d\r\n",fluctuation.power_fluctuation_count);
                                if(fluctuation.power_fluctuation_count > 3){
                                        fluctuation.power_fluctuation_count = 0;
                                        messageData.ActivePower = powerReadData.ActiveTemp;
                                        //printf("flag truw at 22222\r\n");
                                        flag = true;
                                }
                            }else if(messageData.ActivePower > 1000 && change > 10){
                                fluctuation.power_fluctuation_count = fluctuation.power_fluctuation_count + 1;
                                //printf("333 2  fluctuation clount %d\r\n",fluctuation.power_fluctuation_count);
                                if(fluctuation.power_fluctuation_count > 3){
                                        fluctuation.power_fluctuation_count = 0;
                                        messageData.ActivePower = powerReadData.ActiveTemp;
                                        //printf("flag truw at jjjjjj\r\n");
                                        flag = true;
                                }
                            }else if(messageData.ActivePower <= 20  && change > 20){
                               fluctuation.power_fluctuation_count = fluctuation.power_fluctuation_count + 1;
                                //printf("333 p  fluctuation clount %d\r\n",fluctuation.power_fluctuation_count);
                                if(fluctuation.power_fluctuation_count > 10){
                                        fluctuation.power_fluctuation_count = 0;
                                        messageData.ActivePower = powerReadData.ActiveTemp;
                                        //printf("flag truw at pppppp\r\n");
                                        flag = true;
                                }
                            }

                            if(change < 5){
                                fluctuation.power_fluctuation_count = 0; 
                            }
                        }

            }
            if(flag == true){
                   //printf("Voltage change happend %f count %d\r\n",powerReadData.voltageTemp,messageData.voltageCutoff);
                   //    printf("1 Check power %f VS %f \r\n",powerReadData.ActiveTemp,messageData.ActivePower);
                   messageData.ApparentPower = powerReadData.ApparentTemp;
                   messageData.ReactivePower = powerReadData.ReactiveTemp;
                   messageData.Factor = powerReadData.FactorTemp;
                   messageData.ActivePower = powerReadData.ActiveTemp;

                   messageData.flag = true;
                   // example_timer_event_t evt = {.timer_flag = 2};
                   example_queue_element_t evt = {.event_count = 2};// check what need to be changed ????
                   xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
            }
            messageData.Consumstion_temp = (powerReadData.powerCount / 3600.0) * fistTimeCaliberation.F_power;
            //printf("count %d %f %f\r\n",powerReadData.powerCount,messageData.Consumstion_temp,fistTimeCaliberation.F_power);
             //printf(">> %f %f %f \r\n", fistTimeCaliberation.F_power, fistTimeCaliberation.F_current, fistTimeCaliberation.F_voltage);

            //printf("TIMES %d \r\n",PowerUpdateCount);

            if(PowerUpdateCount > 6000){
                       powerReadData.powerCount = 0;
                       PowerUpdateCount = 0;
                       messageData.store_flag = true;
                       //printf("================1 before %f - %f\r\n",messageData.Consumstion,messageData.Consumstion_temp);
                       messageData.Consumstion = messageData.Consumstion + messageData.Consumstion_temp;
                       //printf("================2 before %f - %f\r\n",messageData.Consumstion,messageData.Consumstion_temp);

                       messageData.Consumstion_temp = 0;
                       // example_timer_event_t evt = {.timer_flag = 3};
                       example_queue_element_t evt = {.event_count = 3};
                       xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
            }
            
            if( (strstr(swver,"SYSTEM-10A") && powerReadData.currentTemp > 10500 )||
                (strstr(swver,"SYSTEM-16A") && powerReadData.currentTemp > 16500)){//10Amp
                    messageData.currentCutoff = messageData.currentCutoff + 1;
            }else{
                messageData.currentCutoff = 0;
            }
            if(powerReadData.voltageTemp > 275 || powerReadData.voltageTemp < 85){
                    messageData.voltageCutoff = messageData.voltageCutoff + 1;
            }else{
                        messageData.voltageCutoff = 0;
            }
            if(messageData.currentCutoff > 4 || messageData.voltageCutoff > 4){
                //printf("537 ++++++++++++++++++++++++++++++++++++++++++++++++++++++Changing states \r\n");
                gpio_set_level(RELAY_PIN, 0);
                gpio_set_level(LED_PIN, 1);//<<<<---------------- sachin
                messageData.voltageCutoff = 0;
                messageData.flag = true;
                // example_timer_event_t evt = {.timer_flag = 4};
                example_queue_element_t evt = {.event_count = 4};
                xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
            }
            
           // printf("===== ActivePower %f ReactivePower %f ApparentPower %f Current %f Voltage %f Factor %f\r\n",messageData.ActivePower,messageData.ReactivePower,messageData.ApparentPower,messageData.Current,messageData.Voltage,messageData.Factor);
     }
}




void setRouter2Status(void){
    
        utilsData.Router2Flag = 1;
        // example_timer_event_t evt = {.timer_flag = 7};
        example_queue_element_t evt = {.event_count = 7};
        xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
}




void InitPins()
{
    gpio_config_t io_conf;

    s_timer_queue = xQueueCreate(10, sizeof(example_queue_element_t/*example_timer_event_t*/));

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);


    gpio_set_intr_type(VOLTAGE_CURRENT, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(POWER_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(VOLTAGE_CURRENT, gpio_isr_handler, (void *)VOLTAGE_CURRENT);
    gpio_isr_handler_add(POWER_PIN, gpio_isr_handler, (void *)POWER_PIN);
   // timerCount = esp_timer_get_time();

   example_tg_timer_init(/*TIMER_GROUP_0, TIMER_0, true,*/ 50);

}

void example_wifi_start(void);

volatile int verification_router = 0,verification_status = -1 ,verification_in_progress = -1;;
volatile int verif_cnt = 0;
int getVerificationRouterStatus(){
    return verification_router;
}

int getVerificationStatus(){
    return verification_status;
}


static void init_verification_task(void *arg)
{

    int timeoutLoop = 0, scanCount = 0;

    // // example_timer_event_t evt;
    // example_queue_element_t evt;
    // xQueueReceive(s_timer_queue, &evt, portMAX_DELAY);
    // // printf("-------------------------Timer value %d\r\n",evt.timer_flag);
    // //printf("-------------------------Timer value %ld\r\n",evt.event_count);

    //vTaskDelay( 5 * 1000 / portTICK_PERIOD_MS);
    example_wifi_start();

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    while (1)
    {
        ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
        printf("\nTotal APs scanned = %u\n", ap_count);
        for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++){
            //ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
            //ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
            //int ret = strcmp((char *)ap_info[i].ssid, "YOUR_WIFI_ROUTER");
            int ret = strcmp((char *)ap_info[i].ssid, "QuboFTV_s2rw3M2ixt");
            //ESP_LOGI(TAG, "%d Channel \t\t%d\n", ret, ap_info[i].primary);
            if (ret == 0){
                ESP_LOGI(TAG, "################# Verification Router found \t\t%s", ap_info[i].ssid);
                printf("\n################# Verification Router found \t\t%s\n", ap_info[i].ssid);
                verification_router = 1;
                break;
            }
        }
        if (verification_router == 1){//seems useless here
            gpio_set_level(LED_PIN, 0); 
            powerReadData.MeasureType = CURRENT;
            gpio_set_level(SELECT_PIN, CURRENT);
            vTaskDelay(200 / portTICK_PERIOD_MS);
            break;
        }
        scanCount = scanCount + 1;
        if (scanCount > 5){
            timeoutLoop = 1;
            verification_status = 0;
            verification_in_progress = 1;
            break;
        }
        ESP_LOGI(TAG, "Total scancount = %u/5\n", scanCount);
        //vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    if (timeoutLoop != 1){
        xTaskCreate(VerficationTask, "VerficationTask", 2048, NULL, 5, NULL);
    }

    vTaskDelete(NULL);
}

volatile float vol_= 0, curr_= 0, pow_= 0,unit_= 0,totalunit_ = 0,p_avg = 0;
volatile int totalMinutes = 0, dayMinutes = 0;

float getPower(){
    return pow_;
}

float getCurrent(){
    return curr_;
}

float getVoltage(){
    return vol_;
}

void setTotalUnitFromNVS(float ut){
    totalunit_ = ut;
}

void setDayUnitFromNVS(float ut){
    unit_ = ut;
}

void setTotalTimeFromNVS(int min){
    totalMinutes = min;
}

void setDayTimeFromNVS(int min){
    dayMinutes = min;
}

char da[11];
char *getTodayDate(){
    int yrs = 0;
    memset(da, 0, sizeof(da));
    time_t now = 0;
    struct tm timeinfo = { 0 };
    //save date also <= timeinfo.tm_mday /   timeinfo.tm_mon / timeinfo.tm_year
    time(&now);
    localtime_r(&now, &timeinfo);
    //month 0-11 yrs = starts from 1900

    if (timeinfo.tm_year >  100){
        yrs = timeinfo.tm_year - 100;
    } else
        yrs = timeinfo.tm_year;
    if(yrs != 70 )
        sprintf(da,"%d-%d-20%d",timeinfo.tm_mday,timeinfo.tm_mon+1,yrs); // 122 => 22 => 2022
    else
        sprintf(da,"%d-%d-19%d",timeinfo.tm_mday,timeinfo.tm_mon+1,yrs);// 70 is 1970
    printf("\nGet Today Date : %s\n",da);
    return da;
}

int getTodayMin(){

    time_t now = 0;
    struct tm timeinfo = { 0 };
    //save date also <= timeinfo.tm_mday /   timeinfo.tm_mon / timeinfo.tm_year
    time(&now);
    localtime_r(&now, &timeinfo);

    return timeinfo.tm_hour*60 + timeinfo.tm_min;
}

int getYear(){

    int yrs=0;
    time_t now = 0;
    struct tm timeinfo = { 0 };
    // any year given YEAR - 1900
    //save date also <= timeinfo.tm_mday /   timeinfo.tm_mon / timeinfo.tm_year
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > 100){
        yrs = timeinfo.tm_year - 100;//22
    } else
        yrs = timeinfo.tm_year;//70
    return yrs;
}
char *getMeteringDataFile();

void updateFile(char *path, char *data){
    FILE *f;
    f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open metering for writing");
        return;
    } else {
       ESP_LOGI(TAG,"open success %s",path);
    }

    fprintf(f, "%s", data);
    printf("\nwritten  metering file : %s ,data :: %s\n",path,data);
    fclose(f);
    f = NULL;
}

void setMeteringData(char *res){ // called @every 5 min
    // date change ????? not 1440
    //int curr_min = getTodayMin();
    // day change logic curr_min <= 1449 && curr_min > 1434

    updateFile("/spiffs/day_0.txt",res);
}
/// date 0 1 2 3 file or write json date as key
char action_buff[96];
char *getMeteringDataFile(char *path){//  /spiffs/day_0.txt
    memset(action_buff, 0, sizeof(action_buff));

    ESP_LOGI(TAG,"opening %s",path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open metering for reading");
        return NULL;
    } else {
       ESP_LOGI(TAG,"open success %s",path);
    }
    fscanf(f, "%s", action_buff);
    printf("\nread :: %s from file : %s\n",action_buff,path);
    fclose(f);
    f = NULL;
    return action_buff;
}

char prevDate[11];
int prev_time = 0;
float prev_unit = 0;

int getPrevDayTime(){
    return prev_time;
}
float getPrevDayUnit(){
    return prev_unit;
}
char *getPrevDate(){
    return prevDate;
}
char *getSsid();
int isCommisioned();
char * readNVData(char *key);
int writeNVData(char *key, char *value);
char *getRestResponseCode();
bool getTurnONState();

#if 1

// NVS data update ...
// else spiffs file update ... 

static void calculateUnitTask(void *arg){
    //calculate
    //store cummulative kwh. per minute .
/*
9W bulb.
1 min
(9 / 1000 ) kw * ( 1 / 60 )
0.00015 * 60 * 24 * 5 (day) => 1.08 kwh (unit)

day_0 + day1970 => day_0
*/
    static int test_cnt = 0;


    // BT_onlyMode || wifi connected ???????????
    struct stat st;
    int store_flag = 0;
    int prevday_updated = 0;
    int date_changed = 0;
    
    char *t = readNVData("meteringRest");
    if(strcmp(t,"1") ==0){
        prevday_updated = 1;//onr rest success
    }
    if(strcmp(t,"2") ==0){
        date_changed = 1;//onr rest success
    }

    char date[11];//1-1-70
    memset(date, 0, sizeof(date));
    char *data_1970 = NULL;
    char *data = readNVData("day_0");//getMeteringDataFile("/spiffs/day_0.txt");
    if (strlen(data) > 1){
        sscanf(data,"%f|%d|%s",&unit_,&dayMinutes,date);
    }else{
        data_1970 = readNVData("day_1970");//getMeteringDataFile("/spiffs/day_1970.txt");// for BT mode
        if (strlen(data_1970) > 1){
            sscanf(data_1970,"%f|%d|%s",&totalunit_,&totalMinutes,date);
        }else{
            printf("\nday_1970 not present initially ..... \n");
        }
    }
#if 0      //--------------- test conditions begin ------------------
                strcpy(date,"20-12-2022");

#endif      //--------------- test conditions begin ------------------
       
    printf(" ===== > >> @Boot ===== TUNIT ::->  %f  DUNIT::-> %f\r\n",totalunit_,unit_);
    printf(" ===== > >> @Boot ===== TTIME ::->  %d  DTIME::-> %d\r\n",totalMinutes,dayMinutes);
    char *today = NULL,*prevday = NULL;
    //int temp_test = 0;
    while( 1 ){ /////////   ???????? time when turned on only ............
        if(!isCommisioned()){
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            continue;
        }
        if (store_flag > 6)
            store_flag = 0;
        float p = getPower();// average need to be taken before calculation curr and power ??????
        //if( p > 1){
        if(getTurnONState()){
            float ut = (float)((float)p/(float)1000) *(float)((float)1/(float)60);
            unit_ += ut;
            totalunit_ += ut;

            printf("calculateUnitTask ===== TUNIT ::->  %f  DUNIT::-> %f\r\n",totalunit_,unit_);
            dayMinutes += 1;
            totalMinutes += 1;//dayMinutes;//1 minute
        }
        //} 
        vTaskDelay(60*1000 / portTICK_PERIOD_MS);//1 minute wait 
        store_flag++;
        
        // relay on time update if < 0 power ???
        // vTaskDelay(60*1000 / portTICK_PERIOD_MS);//1 minute wait 
        int yr = getYear();

        if( store_flag >= 5 ){ // in bluetooth mode updating in 3 minutes else in 5 minutes
            // save to nvr with unit and date
            char res[48];
            memset(res, 0, sizeof(res));
            yr = getYear();
     
            if(yr != 70 ){
                //printf("\ntoday assign date prevday: %s\n",today);
                if(strlen(date) > 1 && strstr(date,"1970") == NULL ){
                    prevday = date;
                    printf("\ndate assigned prevday: %s\n",prevday);
                } else if(today != NULL && strstr(today,"1970") == NULL){
                    memset(prevDate,0,sizeof(prevDate));
                    strcpy(prevDate, today);
                    prevday = prevDate;//today;
                    printf("\ntoday assigned prevday: %s\n",prevday);
                }
            }

            today = getTodayDate();

            // today = "21-12-2022";
            // test_cnt++;
            // if(test_cnt >= 3 && test_cnt <= 5)
            //     today = "22-12-2022";

            // if(test_cnt >= 6 && test_cnt <= 10)
            //     today = "23-12-2022";

            // if(test_cnt == 11 && test_cnt <= 30)
            //     today = "24-12-2022";

            // if(test_cnt >= 31)
            //     today = "25-12-2022";


            // 70 is 1970 <== 1-1-1970
            if(yr != 70){
                if ( prevday != NULL && ( strcmp(prevday,today) != 0)){
                    ESP_LOGI(TAG,"Updating Backup file day_1.txt previous day ...");
                    
                    data = readNVData("day_0");//getMeteringDataFile("/spiffs/day_0.txt");
                    printf("\nUpdating Backup file day_1 previous day ...len - %d\n",strlen(data));
                    if(strlen(data)>1){
                        char tmp[48];
                        memset(tmp, 0, sizeof(tmp));
                        strcpy(tmp, data);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        writeNVData("day_1",tmp);
                        dayMinutes = 0;
                        unit_=0;
                        prevday_updated = 0;// in case rest fails ............ 
                        date_changed = 1;
                        memset(date, 0, sizeof(date));
                        writeNVData("meteringRest", "2");//2 for date change indication
                        memset(tmp, 0, sizeof(tmp));
                        writeNVData("day_0",tmp);// empty write
                    }

                } else {
                    printf("\nDate not changed so not updating backup file for cloud ... \n");
                }

                if(date_changed && !prevday_updated){// prev day data
                    // printf("\n######=>>>>>opening backup file  /spiffs/day_1.txt uploading to cloud and unlink\n");
                    char *txt = readNVData("day_1");//getMeteringDataFile("/spiffs/day_1.txt");
                    printf("\nread day_1 for rest => %s\n",txt);

                    if (strlen(txt)>1){
                        int time1 = 0;
                        float ut = 0;
                        memset(prevDate,0,sizeof(prevDate));
                        sscanf(txt,"%f|%d|%s",&ut,&time1,prevDate);
                        prev_time = time1;
                        prev_unit = ut;
                        rest_call(UPDATE_METERING);
                        char *resp = getRestResponseCode();
                        printf("\nMetering Update : response -> %s\n", resp);
                        if(resp[0] == '2'){// response 200 - OK
                            printf("\n Deleting day_1.txt backup file \n");
                            prevday_updated = 1;
                            date_changed = 0;
                            writeNVData("meteringRest", "1");//1 for indicating rest is updated
                            char tmp[48];
                            memset(tmp, 0, sizeof(tmp));
                            writeNVData("day_1", tmp);
                        } else{
                            printf("\n else not able to Deleting day_1.txt backup file %s\n", resp);
                        }
                    }
                } else {
                    printf("\nDay_1 not created so not updating to cloud ... \n");
                }
                //9876543.023302|9876543.023302|1051200|2102400|23-12-2022
                printf("\nyr - %d\n",yr);
                if( getTurnONState()){ // Do this ??????? /*date changed flag date changed and day_1 not updated then do not overwrite date in day_0*/
                    sprintf(res,"%f|%d|%s",unit_,dayMinutes,today);
                    // setMeteringData(res);
                    writeNVData("day_0",res);
                    printf("\nDay_0 updating to N V ... \n");
                }
            }
            //for BT mode only
            if(yr == 70 && p > 1){
                sprintf(res,"%f|%d|%s",totalunit_,totalMinutes,today);
                // updateFile("/spiffs/day_1970.txt",res);
                writeNVData("day_1970",res);
            }

            // totalunit|totaltime|currdayunit|prevdayunit

            store_flag = 0;
        }
    }
}

#else

static void calculateUnitTask(void *arg){
    //calculate
    //store cummulative kwh. per minute .
/*
9W bulb.
1 min
(9 / 1000 ) kw * ( 1 / 60 )
0.00015 * 60 * 24 * 5 (day) => 1.08 kwh (unit)

day_0 + day1970 => day_0
*/

    // BT_onlyMode || wifi connected ???????????
    struct stat st;
    int store_flag = 0;
    int prevday_updated = 0;
    if(strcmp(readNVData("meteringRest"),"1") ==0){
        prevday_updated = 1;//onr rest success
    }
    char date[11];//1-1-70
    memset(date, 0, sizeof(date));
    if(strlen(getSsid()) < 1){// commisioned in BT mode ---  strlen(getSsid()) won't be useful as commissioning starts after this task 
        char *data_1970 = getMeteringDataFile("/spiffs/day_1970.txt");// for BT mode
        if (data_1970 != NULL){
            sscanf(data_1970,"%f|%f|%d|%d|%s",&totalunit_,&unit_,&dayMinutes,&totalMinutes,date);
        }
    }
    char *data = getMeteringDataFile("/spiffs/day_0.txt");
    if (data != NULL){
        sscanf(data,"%f|%f|%d|%d|%s",&totalunit_,&unit_,&dayMinutes,&totalMinutes,date);
    }
#if 0      //--------------- test conditions begin ------------------
                strcpy(date,"20-12-2022");

#endif      //--------------- test conditions begin ------------------
       
    printf(" ===== > >> @Boot ===== TUNIT ::->  %f  DUNIT::-> %f\r\n",totalunit_,unit_);
    printf(" ===== > >> @Boot ===== TTIME ::->  %d  DTIME::-> %d\r\n",totalMinutes,dayMinutes);
    char *today = NULL,*prevday = NULL;
    //int temp_test = 0;
    while( 1 ){ /////////   ???????? time when turned on only ............
        if(!isCommisioned()){
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            continue;
        }
        if (store_flag > 6)
            store_flag = 0;
        float p = getPower();// average need to be taken before calculation curr and power ??????
        if( p > 1){
            float ut = (float)((float)p/(float)1000) *(float)((float)1/(float)60);
            unit_ += ut;
            totalunit_ += ut;

            printf("calculateUnitTask ===== TUNIT ::->  %f  DUNIT::-> %f\r\n",totalunit_,unit_);
            dayMinutes += 1;
            totalMinutes += 1;//dayMinutes;//1 minute
        }
        
        // relay on time update if < 0 power ???
        // vTaskDelay(60*1000 / portTICK_PERIOD_MS);//1 minute wait -- 20sec
        int yr = getYear();
        vTaskDelay(60*1000 / portTICK_PERIOD_MS);//1 minute wait -- 20sec
        store_flag++;
        //temp_test++;
        // if( store_flag >= 1) { // in bluetooth mode updating in 3 minutes else in 5 minutes
        if( ((yr == 70 && store_flag >=3 ) || store_flag >= 5) /*&& p > 1*/){ // in bluetooth mode updating in 3 minutes else in 5 minutes
            // save to nvr with unit and date
            char res[96];
            memset(res, 0, sizeof(res));
            yr = getYear();
     
            if(yr != 70 ){
                //printf("\ntoday assign date prevday: %s\n",today);
                if(strlen(date) > 1 && strstr(date,"1970") == NULL ){
                    prevday = date;
                    printf("\ndate assigned prevday: %s\n",prevday);
                } else if(today != NULL && strstr(today,"1970") == NULL){
                    prevday = today;//today;
                    printf("\ntoday assigned prevday: %s\n",prevday);
                }
            }

            today = getTodayDate();

#if 0       //--------------- test conditions begin ------------------

            //today = "19-12-2022";

            if(prevday != NULL && strlen(prevday) > 1 && strstr(prevday,"1970") == NULL && yr != 70 && ( strcmp(prevday,today) != 0)){
                // writeNVData("dateChanged", "0");
                // writeNVData("meteringRest", "0");
                //writeNVData("dateChanged", "1");

            }    
            // prevday and today is different then we need to create day1 and upload to cloud 
            // if missed in this cycle then untill next date change we do not have option to create day1 file and 2 days data will be merged 
            // ???????????????  save a flag in nvs and reset flaf in factory reset and after successful rest_api response .... 

            // //---------unit test---
            // if(temp_test > 4){// for running day change
            //     prevday = "24-11-2022";
            //     temp_test = 0;
            //     printf("\ntest prevday: %s\n",prevday);
            // }
            // //---------
            if((stat("/spiffs/day_0.txt", &st) == 0 && st.st_size > 10)){
                printf("\nTest 1 file exist .....\n");
            }
            if(prevday != NULL && strlen(prevday) > 1 && strstr(prevday,"1970") == NULL){
                printf("\nTest 2 prev day not null and not 1970 .....\n");
            }
            if(( strcmp(prevday,today) != 0)){
                printf("\nTest 3 Date changed .....\n");
            }
#endif      //--------------- test conditions end ------------------
            // 70 is 1970 <== 1-1-1970
            // missing day  cause was today and prevday was pointing to same date buffer ...  
            if(yr != 70){
                if ( prevday != NULL && (stat("/spiffs/day_0.txt", &st) == 0 && st.st_size > 10) && ( strcmp(prevday,today) != 0)){
                    ESP_LOGI(TAG,"Updating Backup file day_1.txt previous day ...");
                    printf("\nUpdating Backup file day_1.txt previous day ...\n");
                    data = getMeteringDataFile("/spiffs/day_0.txt");
                    updateFile("/spiffs/day_1.txt",data);
                    dayMinutes = 0;
                    unit_=0;
                    prevday_updated = 0;// in case unlink fails ............ 
                    memset(date, 0, sizeof(date));
                    writeNVData("meteringRest", "0");
                    // update day_0 here to avoid multiple update rest  ...????????????????????????????????????????.........
                    //if p<1 and plug powered on then on every reboot  rest will go ..... 
                }

                if(stat("/spiffs/day_1.txt", &st) == 0 && st.st_size > 10 && !prevday_updated){// prev day data
                    ESP_LOGI(TAG,"######=>>>>>opening backup file  /spiffs/day_1.txt uploading to cloud and unlink\n");
                    char *txt = getMeteringDataFile("/spiffs/day_1.txt");
                    int time1 = 0,ttime1 = 0;
                    float ut = 0, tut = 0;
                    if (txt != NULL && strlen(txt) > 5){
                        memset(prevDate,0,sizeof(prevDate));
                        sscanf(txt,"%f|%f|%d|%d|%s",&tut,&ut,&time1,&ttime1,prevDate);
                        prev_time = time1;
                        prev_unit = ut;
                        rest_call(UPDATE_METERING);
                        char *resp = getRestResponseCode();
                        printf("\nMetering Update : response -> %s\n", resp);
                        if(resp[0] == '2'){// response 200 - OK
                   //         writeNVData("meteringRest", "1");
                            printf("\n Deleting day_1.txt backup file \n");
                            int status = unlink("/spiffs/day_1.txt");
                            if(status != 0){
                                printf("\n Deleting day_1.txt backup file failed 1\n");
                                vTaskDelay(100 / portTICK_PERIOD_MS);
                                status = unlink("/spiffs/day_1.txt");//2nd retry to delete
                                if(status != 0){
                                    printf("\n Deleting day_1.txt backup file failed 2 removing content\n");
                                    updateFile("/spiffs/day_1.txt","||||");
                                }
                            }
                            prevday_updated = 1;
                            writeNVData("meteringRest", "1");
                        } else{
                            printf("\n else not able to Deleting day_1.txt backup file %s\n", resp);
                        }
                    }
                }
                printf("\nyr - %d\n",yr);
                if( p > 1){// prevday, today different then don't write and loss pointer ad date will override .. and not update possible ..
                    sprintf(res,"%f|%f|%d|%d|%s",totalunit_,unit_,dayMinutes,totalMinutes,today);
                    setMeteringData(res);
    	        }
            }
            //for BT mode only
            if((strlen(getSsid()) < 1) && yr == 70 && p > 1){
                sprintf(res,"%f|%f|%d|%d|%s",totalunit_,unit_,dayMinutes,totalMinutes,today);
                updateFile("/spiffs/day_1970.txt",res);
            }

            // totalunit|totaltime|currdayunit|prevdayunit
            // |8||
// try spiffs <== all day total totaltime
/*
tm_sec seconds passed since last minute
tm_min minutes passed since hour
tm_hour hours passed since midnight
tm_mday day of the month
tm_mon months passed since January
tm_year year passed since 1900
tm_wday days since Sunday
tm_yday  days since first day of the year
tm_isdst Daylight Saving Time Flag
*/

            // memset(res, 0, sizeof(res));
            // sprintf(res,"%f",unit_);
            // writeNVData("dayunit", res);

            // memset(res, 0, sizeof(res));
            // sprintf(res,"%d",dayMinutes);
            // writeNVData("daymin", res);

            // memset(res, 0, sizeof(res));
            // sprintf(res,"%d",totalMinutes);
            // writeNVData("totalmin", res);

            store_flag = 0;
        }
    }
}
#endif

float getDayUnit(){
    return unit_;
}
float getTotalUnit(){
    return totalunit_;
}
int getTotalTime(){
    return totalMinutes;
}

void init_unit_calculation(){
    xTaskCreate(calculateUnitTask, "calculateUnitTask", 8*1024/*2048*/, NULL, 10, NULL);
}

volatile int mqtt_cnt = 0;
static void meteringUpdateTaskwifi(void *arg)
{
    while(mqtt_cnt >= 0 ){
        mqtt_cnt--;
        if( curr_ > 1 && pow_ > 1){
            printf("\n Metering Pub ..... \n");
            MqttPublishMetering(curr_, vol_, pow_,unit_,dayMinutes);
        }else{
            MqttPublishMetering(0, vol_, 0,unit_,dayMinutes);
        }
        vTaskDelay(5*1000 / portTICK_PERIOD_MS);// mqtt_cnt depends upon delay
    }
    mqtt_cnt = 0;
    vTaskDelete(NULL);
}
void notify_over_control_chr(int type);
static void meteringUpdateTaskble(void *arg)
{
    while(mqtt_cnt >= 0 ){
        mqtt_cnt--;
        if( curr_ > 1 && pow_ > 1){
            printf("\n Metering notify ble ..... \n");
            notify_over_control_chr(3);//UPDATE_METERING_STATUS == 3
        }
        vTaskDelay(5*1000 / portTICK_PERIOD_MS);// mqtt_cnt depends upon delay
    }
    mqtt_cnt = 0;
    vTaskDelete(NULL);
}

void meteringMqttUpdate(int viaBle){
    printf("\nmeteringMqttUpdate - [%d]\n",mqtt_cnt);
    if(mqtt_cnt == 0){
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if(viaBle){
            xTaskCreate(meteringUpdateTaskble, "meteringUpdateTaskble", 2 * 2048, NULL, 10, NULL);
        } else {
            xTaskCreate(meteringUpdateTaskwifi, "meteringUpdateTaskwifi", 2 * 2048, NULL, 10, NULL);
        }
    }

    mqtt_cnt = 14; // 12 * 5 = 60sec = 1 min 10 sec(1 count extra) extra
}

static void PowerMonitorTask(void *arg)
{
   int a = 0;
   //init mqtt timer
    while(1){
        if(a > 30){
            // p_avg = (pow_ + powerReadData.ActiveTemp)/2;
            vol_ = powerReadData.voltageTemp;
            curr_ = powerReadData.currentTemp;
            pow_ = powerReadData.ActiveTemp;
            printf("PowerMonitorTask ===== Voltage %f Current %f ActivePower %f apparent %f reactive %f factor %f\r\n",powerReadData.voltageTemp,powerReadData.currentTemp,powerReadData.ActiveTemp,powerReadData.ApparentTemp,powerReadData.ReactiveTemp,powerReadData.FactorTemp);
            // printf("PowerMonitorTask p_avg => %f \n",p_avg);
            //meteringMqttUpdate();// from mqtt only
            a = 0;
        }
        a = a + 1;
        //printf("===== F_current %f F_voltage %f F_power %f \r\n",fistTimeCaliberation.F_current,fistTimeCaliberation.F_voltage, fistTimeCaliberation.F_power);
       //printf("===== ActivePower %f ReactivePower %f ApparentPower %f Current %f Voltage %f Factor %f\r\n",messageData.ActivePower,messageData.ReactivePower,messageData.ApparentPower,messageData.Current,messageData.Voltage,messageData.Factor);
       CheckPowerMonitorData();

       vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
static void CaliberationTask(void *arg)
{
    int prodRouterFound = 0;
    int timeoutLoop = 0, scanCount = 0;
    unsigned char i = 0;
    printf("\nCaliberationTask\n");
    float AvgVoltage =0, AvgCurrent=0, AvgPower=0;
    InCaliberation = true;
    gpio_set_level(LED_PIN, 1);//OFF
    // gpio_set_level(RELAY_PIN, 1);// changed ...
    // powerReadData.MeasureType = VOLTAGE;
    // gpio_set_level(SELECT_PIN, VOLTAGE);
    // vTaskDelay(200 / portTICK_PERIOD_MS);

    // powerReadData.VoltageTimeIndex = 3;powerReadData.CurrentTimeIndex = 3;powerReadData.PowerTimeIndex = 3;


    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1)
    {
        while (1)
        {
            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
            ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
            printf("\nTotal APs scanned = %u\n", ap_count);
            for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++){
                ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
                printf("\nSSID \t\t%s\n", ap_info[i].ssid);
                ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);

                //int ret = strcmp((char *)ap_info[i].ssid, "YOUR_WIFI_ROUTER");
                int ret = strcmp((char *)ap_info[i].ssid, "QuboFT_fsNJUElwk4");
                ESP_LOGI(TAG, "%d Channel \t\t%d\n", ret, ap_info[i].primary);
                if (ret == 0){
                    ESP_LOGI(TAG, "#################Production Router found \t\t%s", ap_info[i].ssid);
                    printf("\nCaliberationTask prouter found\n");
                    prodRouterFound = 1;
                    break;
                }
            }
            if (prodRouterFound == 1){
                gpio_set_level(LED_PIN, 0);
                gpio_set_level(RELAY_PIN, 1);// changed ...
                powerReadData.MeasureType = VOLTAGE;
                gpio_set_level(SELECT_PIN, VOLTAGE);
                vTaskDelay(200 / portTICK_PERIOD_MS);

                powerReadData.VoltageTimeIndex = 3;powerReadData.CurrentTimeIndex = 3;powerReadData.PowerTimeIndex = 3;
                vTaskDelay(100 / portTICK_PERIOD_MS);
                powerReadData.MeasureType = CURRENT;
                gpio_set_level(SELECT_PIN, CURRENT);
                vTaskDelay(300 / portTICK_PERIOD_MS);
                break;
            }
            scanCount = scanCount + 1;
            if (scanCount > 5){
                timeoutLoop = 1;
                break;
            }
        }
        while (1){
            if (timeoutLoop == 1){
                break;
            }
            printf("CaliberationTask VOLTAGE\r\n");
            for (i = 0; i < 5; i++){
                printf("%05lu ", powerReadData.VoltageTimeDif[i]);
            }
            printf("\r\nCaliberationTask CURRENT \r\n");
            for (i = 0; i < 5; i++){
                printf("%05lu ", powerReadData.CurrentTimeDif[i]);
            }
            printf("\r\nCaliberationTask POWER\r\n");
            for (i = 0; i < 5; i++){
                printf("%05lu ", powerReadData.PowerTimeDif[i]);
            }
            printf("\r\n");

            for(i = 0;i < 5; i++){
                   AvgVoltage = AvgVoltage + powerReadData.VoltageTimeDif[i]; 
                   AvgCurrent= AvgCurrent + powerReadData.CurrentTimeDif[i]; 
                   AvgPower= AvgPower + powerReadData.PowerTimeDif[i];
            }
            AvgVoltage = AvgVoltage / 5.0;AvgCurrent= AvgCurrent / 5; AvgPower= AvgPower / 5;
            printf("TIME = AvgVoltage %f AvgCurrent %f AvgPower %f\r\n", AvgVoltage, AvgCurrent, AvgPower);
            AvgVoltage = 1000000.0 / AvgVoltage;AvgCurrent= 1000000.0 / AvgCurrent; AvgPower= 1000000.0 / AvgPower;
            printf("FREQ = AvgVoltage %f AvgCurrent %f AvgPower %f\r\n", AvgVoltage, AvgCurrent, AvgPower);

            // if(AvgPower > 30 && AvgPower < 150 &&  AvgVoltage > 1000){
            if(AvgPower > 30 && AvgPower < 175 &&  AvgVoltage > 1000){
                //FREQ = AvgVoltage 1306.631958 AvgCurrent 86.201538 AvgPower 167.853516
                // 0.515330 4.559083 0.168372
                // add measured value here
                fistTimeCaliberation.F_power = 86.5 / AvgPower;
                fistTimeCaliberation.F_current = 393 / AvgCurrent;
                fistTimeCaliberation.F_voltage = 220 / AvgVoltage;
                // fistTimeCaliberation.F_power = 52 / AvgPower;
                // fistTimeCaliberation.F_current = 250 / AvgCurrent;
                // fistTimeCaliberation.F_voltage = 220 / AvgVoltage;
                fistTimeCaliberation.flag = 0xAA55;
                for(i = 0;i < 5; i++){
                     vTaskDelay(100 / portTICK_PERIOD_MS);
                     AvgPower = (1000000.0 / (float)powerReadData.PowerTimeDif[0]) * fistTimeCaliberation.F_power;
                     printf("\ncalibration Avg Power => %f\n",AvgPower);
                     //if(AvgPower >  84  && AvgPower < 90){
                     if(AvgPower >  80  && AvgPower < 90){
                     // if(AvgPower >  48  && AvgPower < 56){
                            WriteFile(&fistTimeCaliberation, "caliberation", sizeof(fistTimeCaliberation));
                            printf(">> %f %f %f \r\n", fistTimeCaliberation.F_power, fistTimeCaliberation.F_current, fistTimeCaliberation.F_voltage);
                            gpio_set_level(LED_PIN, 0);
                            vTaskDelay(500 / portTICK_PERIOD_MS);
                            gpio_set_level(LED_PIN, 1);
                            vTaskDelay(500 / portTICK_PERIOD_MS);
                            gpio_set_level(LED_PIN, 0);
                        // ON -> OFF -> ON and Relay OFF
                        //    vTaskDelay(500 / portTICK_PERIOD_MS);
                        //    gpio_set_level(LED_PIN, 1);
                            gpio_set_level(RELAY_PIN, 0);
                            vTaskDelete(NULL);
                            break;        
                     }
                }
            }
            vTaskDelay(400 / portTICK_PERIOD_MS);
        }
        if (timeoutLoop == 1)
        {
            vTaskDelete(NULL);
        }
    }
    vTaskDelete(NULL);
}
void removeButtonTask();
static void VerficationTask(void *arg)
{

    unsigned char i = 0;

    float AvgVoltage = 0, AvgCurrent = 0, AvgPower = 0;


    gpio_set_level(RELAY_PIN, 1);
    powerReadData.MeasureType = VOLTAGE;
    gpio_set_level(SELECT_PIN, VOLTAGE);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    powerReadData.VoltageTimeIndex = 8;
    powerReadData.CurrentTimeIndex = 8;
    powerReadData.PowerTimeIndex = 8;

    vTaskDelay(100 / portTICK_PERIOD_MS);

    powerReadData.MeasureType = CURRENT;
    gpio_set_level(SELECT_PIN, CURRENT);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    printf("VOLTAGE\r\n");
    for (i = 0; i < 5; i++)
    {
        printf("%05lu ", powerReadData.VoltageTimeDif[i]);
    }
    printf("\r\nCURRENT \r\n");
    for (i = 0; i < 5; i++)
    {
        printf("%05lu ", powerReadData.CurrentTimeDif[i]);
    }
    printf("\r\nPOWER\r\n");
    for (i = 0; i < 5; i++)
    {
        printf("%05lu ", powerReadData.PowerTimeDif[i]);
    }
    printf("\r\n");

    for (i = 0; i < 5; i++)
    {
        AvgVoltage = AvgVoltage + powerReadData.VoltageTimeDif[i];
        AvgCurrent = AvgCurrent + powerReadData.CurrentTimeDif[i];
        AvgPower = AvgPower + powerReadData.PowerTimeDif[i];
    }
    AvgVoltage = AvgVoltage / 5.0;
    AvgCurrent = AvgCurrent / 5;
    AvgPower = AvgPower / 5;

    printf("TIME = AvgVoltage %f AvgCurrent %f AvgPower %f\r\n", AvgVoltage, AvgCurrent, AvgPower);

    AvgVoltage = (1000000.0 / AvgVoltage) * fistTimeCaliberation.F_voltage;
    AvgCurrent = (1000000.0 / AvgCurrent) * fistTimeCaliberation.F_current;
    AvgPower = (1000000.0 / AvgPower) * fistTimeCaliberation.F_power;

    printf("FREQ = AvgVoltage %f AvgCurrent %f AvgPower %f\r\n", AvgVoltage, AvgCurrent, AvgPower);

    if ((AvgPower > 80.0 && AvgPower < 95.0) && (AvgVoltage > 215.0 && AvgVoltage < 225.0) && (AvgCurrent > 350.0 && AvgCurrent < 450.0))
    {
        printf("VerficationTask OK ==================================================\r\n");  
        verification_status = 1;
        char res[2];
        verif_cnt = verif_cnt + 1;
        memset(res, 0, sizeof(res));
        sprintf(res,"%d",verif_cnt);
        writeNVData("verifycount",res);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        gpio_set_level(RELAY_PIN, 0);

        // only increase nvs verification count here
        vTaskDelay (5000 / portTICK_PERIOD_MS);
    }
    else
    {
        verification_status = 0;
        printf("VerficationTask NOT OK !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

        gpio_set_level(LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 1);
        gpio_set_level(RELAY_PIN, 0);

        //removeButtonTask();

    }
    verification_in_progress = 1;
    vTaskDelete(NULL);
}

volatile int isPowerMonitoring = 0;

int getPowerMonitoringStatus(){
    return isPowerMonitoring;
}

void init_verification(){
    xTaskCreate(init_verification_task, "init_verification_task", 2 * 2048, NULL, 5, NULL);
}



void PowerMonitorFunction(void)
{

    ReadFile(&deviceData, "deviceData", sizeof(deviceData));
    printf("Data retrieved from memory here!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

    ReadFile(&messageData, "messageData", sizeof(messageData));
    InitPins();
    //xTaskCreate(pert_task, "pert_task", 2048, NULL, 5, NULL);

    printf("===================>>>>>  Check storage Data here %X\r\n",deviceData.storeFlag);
    if(deviceData.storeFlag != 0xAA55){
         deviceData.storeFlag = 0xAA55;
         deviceData.smartConfigFlag = 1;
         WriteFile(&deviceData, "deviceData", sizeof(deviceData));
    }

    powerReadData.ActiveTemp = 0 ;
    messageData.ActivePower  = 0;
    powerReadData.currentTemp = 0;
    messageData.Current = 0;
    powerReadData.voltageTemp = 0;
    messageData.Voltage = 0;
    
    fluctuation.voltage_fluctuation_count = 4;
    fluctuation.current_fluctuation_count = 0;
    fluctuation.power_fluctuation_count = 0;

    ReadFile(&fistTimeCaliberation, "caliberation", sizeof(fistTimeCaliberation));
    printf("CHEKC  >> %f %f %f \r\n", fistTimeCaliberation.F_power, fistTimeCaliberation.F_current, fistTimeCaliberation.F_voltage);

    swver = getCurrentSWVersion();

    if (fistTimeCaliberation.flag == 0xAA55)
    {   

        char *verification_count = readNVData("verifycount");
        printf("\nNV Data total verification count =>   %d\n",verification_count);
        if(strlen(verification_count) > 0 /*&& isCommisioned()*/){//as we are not resetting NVS
            ESP_LOGI(TAG, "\nNV Data total verification count =>   %s\n",verification_count);
            sscanf(verification_count, "%d", &verif_cnt);
            ESP_LOGI(TAG, "\nNV Data  verification count->: %d\n",verif_cnt);
            printf("\nNV Data  verification count->: %d\n",verif_cnt);
        }

        if(verif_cnt < 1){
            printf("\ninitializing verification !!!!!! \n");
            gpio_set_level(LED_PIN, 1); 
            init_verification();
            verification_in_progress = 0;
        }
        else{
            verification_status = 1;
            verification_in_progress = 1;
            ESP_LOGI(TAG, "\none times successfully verified !!!!!! \n");
            printf("\nOne times successfully verified !!!!!! \n");
        }


        while(verification_in_progress != 1) vTaskDelay(500 / portTICK_PERIOD_MS);

        if(verif_cnt >=1 && verification_status == 1){
            isPowerMonitoring = 1;
        }
        if(isPowerMonitoring){
            xTaskCreate(PowerMonitorTask, "PowerMonitorTask", 2 * 2048, NULL, 10, NULL);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        // xTaskCreate(calculateUnitTask, "calculateUnitTask", 8*1024/*2048*/, NULL, 10, NULL);
    }
    else
    {
        //0.526872 4.541037 0.114268 <= 10amp
        //>> 1.206052 10.044059 0.119108  <=16amp
        fistTimeCaliberation.F_power = 0.526872;
        fistTimeCaliberation.F_current = 4.541037;
        fistTimeCaliberation.F_voltage = 0.114268;
        //isPowerMonitoring = 2;
        xTaskCreate(CaliberationTask, "CaliberationTask", 2 * 2048, NULL, 5, NULL);
        char res[2];
        memset(res, 0, sizeof(res));
        sprintf(res,"%d",verif_cnt);
        writeNVData("verifycount",res);
        // while(1);
        //isPowerMonitoring = 1; // in case if everything erased and we do recommisioning then with hardcoded value
        //while(1){vTaskDelay(10 / portTICK_PERIOD_MS);}
    }

    // Disaster recovery ????????????

}
