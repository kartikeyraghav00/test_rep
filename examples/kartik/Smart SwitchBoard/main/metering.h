#pragma once

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "driver/gpio.h"
#include <esp_timer.h>

#define DEFAULT_SCAN_LIST_SIZE 10






#define SELECT_PIN 5
#define LED_PIN 6
#define RELAY_PIN 4

#define GPIO_OUTPUT_PIN_SEL ((1ULL << SELECT_PIN) | (1ULL << LED_PIN) | (1ULL << RELAY_PIN))


#define VOLTAGE_CURRENT 3
#define POWER_PIN 7
#define GPIO_INPUT_PIN_SEL ((1ULL << VOLTAGE_CURRENT) | (1ULL << POWER_PIN))

#define VOLTAGE 1
#define CURRENT 0
#define MEASURE_NONE    2

enum METER{
    MTR_REACTIVE = 0,
    MTR_VOLTAGE,
    MTR_CURRENT,
    MTR_CONSUMED,
    MTR_ACTIVE,
    MTR_FACTOR,
    MTR_APPARENT
};


typedef struct
{
    unsigned char Router2Flag;
    unsigned char DEVICE_STATE;
    unsigned char CONFIG_STATE;
    unsigned char DEVICE_FLAG;
    unsigned int LED_COUNTER;
    unsigned char LED_FLAG;
    unsigned int smartConfigFlag;
}Utils;

// Utils utilsData;


void setRouter2Status(void);
static void VerficationTask(void *arg);



typedef struct
{
    unsigned char LED_STATE;
    unsigned char DEVICE_STATE;
    unsigned char CONFIG_STATE;
    unsigned char DEVICE_FLAG;
    unsigned int LED_COUNTER;
    unsigned char LED_FLAG;
    unsigned int smartConfigFlag;
    unsigned int storeFlag;
}DeviceData;

enum STATE{
    STATE_IDLE = 0,
    STATE_FACTORY_RESET,
    STATE_NETWORK_RESET,
    STATE_CONNECTED,
    STATE_DISCONNCTED,
    STATE_OTA,
    STATE_NONE

};

typedef struct
{
    float F_power;
    float F_current;
    float F_voltage;
    unsigned long time;
    unsigned long pulseCount;
    int flag;
    int plugStatus;
} PulseData;


typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;

// typedef struct {
//     unsigned char timer_flag;
// } example_timer_event_t;

typedef struct {
    uint64_t event_count;
} example_queue_element_t;

typedef struct {
    uint32_t PowerTimePres;
    uint32_t VoltageTimePres;
    uint32_t CurrentTimePres;
    
    uint32_t PowerTimePrev;
    uint32_t VoltageTimePrev;
    uint32_t CurrentTimePrev;

    uint32_t PowerTimeDif[5];
    uint32_t VoltageTimeDif[5];
    uint32_t CurrentTimeDif[5];

    uint8_t PowerTimeIndex;
    uint8_t VoltageTimeIndex;
    uint8_t CurrentTimeIndex;

    uint8_t MeasureType;
    uint8_t VoltageReadFlag;
    uint8_t CurrentReadFlag;
    uint8_t PowerReadFlag;

    uint16_t powerCount;
    uint8_t pushbuttonFlagLow;
     uint8_t pushbuttonFlagHigh;

    uint32_t PushButtonPres;
    uint32_t PushButtonPrev;
    uint32_t PushButtonDif;

    unsigned char timerCount;

    float voltageTemp;
    float currentTemp;
    float ActiveTemp;
    float ReactiveTemp;
    float ApparentTemp;
    float FactorTemp;
    
    unsigned char paramFlag;
   unsigned char power_timerCount;
} PowerMonitor;

typedef struct
{
    float ActivePower;
    float ReactivePower;
    float ApparentPower;
    float Current;
    float Voltage;
    float Factor;
    float Consumstion;
    float Consumstion_temp;
    bool flag;
    bool store_flag;
    unsigned char voltageCutoff;
    unsigned char currentCutoff;
    
} MessageData;

// PowerMonitor powerReadData;
// DeviceData deviceData;
// MessageData messageData;
// Utils utilsData;

typedef struct
{
    unsigned char voltage_fluctuation_count;
    unsigned char current_fluctuation_count;
    unsigned char power_fluctuation_count;
} Fluctuation;

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
// #define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER) / 1000  // convert counter value to seconds
#define TIMER_SCALE           ( APB_CLK_FREQ / TIMER_DIVIDER) / 1000  // convert counter value to seconds



void MqttPublishMetering(float curr, float voltage, float power, float unit, int totalTime );
void PowerMonitorFunction(void);
 int WriteFile(void *value, const char *key , size_t length);
 int ReadFile(void *value, const char *key, size_t length);
 void LedIndication();
 void CheckPowerMonitorData();
 void checkStoreCount();
void updatePowerData();
void updateAllChanges();
#ifdef MANUAL_SWITCHES
 void CheckManualInputStatus();
#endif