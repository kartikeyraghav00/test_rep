/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
    */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/ledc.h"
#include "esp_err.h"

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/

#define redpin      5   // select the pin for the red LED
#define bluepin     6   // select the pin for the blue LED
#define greenpin    4   // select the pin for the green LED

#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE

#define LEDC_LS_CH0_GPIO       (4)                  //green LED Fade up / increase intensity
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_LS_CH1_GPIO       (6)                  //blue onchip LED Fade down / decrease intensity
#define LEDC_LS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_LS_CH2_GPIO       (5)                  // GPIO 7 Keep a stable intensity
#define LEDC_LS_CH2_CHANNEL    LEDC_CHANNEL_2   
#define LEDC_LS_CH3_GPIO       (9)
#define LEDC_LS_CH3_CHANNEL    LEDC_CHANNEL_3    

#define LEDC_TEST_CH_NUM       (4)
#define LEDC_TEST_DUTY         (8192)           //divides the duty cycle % 0-100% into 0 - 6000 means higher the test duty cycle more smoothly the transition is seen
#define LEDC_TEST_FADE_TIME    (20000)           //Fade time in ms makes the transition within the specified time



TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;

    SemaphoreHandle_t mutex_v;
int i = 0;
// void vATask( void * pvParameters )
// {
//    /* Create a mutex type semaphore. */
//    xSemaphore = xSemaphoreCreateMutex();

//    if( xSemaphore != NULL )
//    {
//        /* The semaphore was created successfully and
//        can be used. */
//    }
// }

void Task1(void *pvParameters) { 
    while(i<1000) 
    { 
        i++;
        xSemaphoreTake(mutex_v, portMAX_DELAY); 
        printf("Tk1:%d\n",i); 
        xSemaphoreGive(mutex_v); 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
}

void Task2(void *pvParameters) { 
    while(i<1000) 
    { 
        i++;
        xSemaphoreTake(mutex_v, portMAX_DELAY); 
        printf("Tk2:%d\n",i); 
        xSemaphoreGive(mutex_v); 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
}

uint8_t pin;
//static uint8_t s_led_state = 0;
    
static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    //gpio_set_level(greenpin, s_led_state);

    //RGB HW-478 operation
        gpio_set_level(greenpin, 1);
        vTaskDelay(100);
        gpio_set_level(greenpin, 0);
        vTaskDelay(100);

        gpio_set_level(bluepin, 1);
        vTaskDelay(100);
        gpio_set_level(bluepin, 0);
        vTaskDelay(100);

        gpio_set_level(redpin, 1);
        vTaskDelay(100);
        gpio_set_level(redpin, 0);
        vTaskDelay(100);
}

static void configure_led(void)
{
    //ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    //gpio_reset_pin(BLINK_GPIO);
    gpio_reset_pin(redpin);
    gpio_reset_pin(bluepin);
    gpio_reset_pin(greenpin);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(redpin, GPIO_MODE_OUTPUT);
    gpio_set_direction(bluepin, GPIO_MODE_OUTPUT);
    gpio_set_direction(greenpin, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    int ch;
#if 0 
    mutex_v = xSemaphoreCreateMutex(); 
    if (mutex_v == NULL) { 
    printf("Mutex can not be created"); 
    } 

    xTaskCreate(Task1, "Task 1", 1280, NULL, 1, NULL); 
    xTaskCreate(Task2, "Task 2", 1280, NULL, 1, NULL); 
    /* Configure the peripheral according to the LED type */
#endif

#if 1

    configure_led();
#endif

#if 0
    while(1)
    {
        gpio_set_level(greenpin, 1);
        gpio_set_level(redpin, 0);
        vTaskDelay(1000 );
        gpio_set_level(greenpin, 0);
        gpio_set_level(redpin, 0);
        vTaskDelay(300);
        gpio_set_level(greenpin, 0);
        gpio_set_level(redpin, 1);
        vTaskDelay(1000);
        gpio_set_level(greenpin, 0);
        gpio_set_level(redpin, 0);
        vTaskDelay(300);
    }
    // Green = GPIO 4
    // Red = GPIO 5
    blink_led();
#endif

#if 0
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,           // timer mode
        .timer_num = LEDC_LS_TIMER,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
    ledc_channel_config_t ledc_channel[3] = {
        {
            .channel    = LEDC_LS_CH0_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_LS_CH0_GPIO,
            .speed_mode = LEDC_LS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_LS_TIMER,
            .flags.output_invert = 0
        },
        {
            .channel    = LEDC_LS_CH1_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_LS_CH1_GPIO,
            .speed_mode = LEDC_LS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_LS_TIMER,
            .flags.output_invert = 0
        },
        {
            .channel    = LEDC_LS_CH2_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_LS_CH2_GPIO,
            .speed_mode = LEDC_LS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_LS_TIMER,
            .flags.output_invert = 0
        },
    };

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < 3; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    // Initialize fade service.
    ledc_fade_func_install(0);

#endif

#if 1
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (4) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (8100) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

// Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
while(1)
{
    for(int i = 25; i<40; i++)
    {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, i*500);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

        printf("Duty:%d\n",(i*200));
        vTaskDelay(1000);
    }
}




#endif

#if 0
    while (1) {
        //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");

        //Green LED faded
        printf("1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        
            ledc_set_fade_with_time(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, LEDC_FADE_NO_WAIT);
        
        vTaskDelay(500);
        printf("2. LEDC fade down to duty = 0\n");
        
            ledc_set_fade_with_time(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, LEDC_FADE_NO_WAIT);
        
        vTaskDelay(100);

        //Blue LED faded
        // printf("3. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        
        //     ledc_set_fade_with_time(ledc_channel[1].speed_mode,
        //             ledc_channel[1].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
        //     ledc_fade_start(ledc_channel[1].speed_mode,
        //             ledc_channel[1].channel, LEDC_FADE_NO_WAIT);
        
        // printf("4. LEDC fade down to duty = 0\n");
        
        //     ledc_set_fade_with_time(ledc_channel[1].speed_mode,
        //             ledc_channel[1].channel, 0, LEDC_TEST_FADE_TIME);
        //     ledc_fade_start(ledc_channel[1].speed_mode,
        //             ledc_channel[1].channel, LEDC_FADE_NO_WAIT);
        
        // vTaskDelay(500);

        //Red LED faded
        printf("5. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        
            ledc_set_fade_with_time(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, LEDC_FADE_NO_WAIT);
        
        vTaskDelay(500);
        
        printf("6. LEDC fade down to duty = 0\n");
        
            ledc_set_fade_with_time(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(100);        

    }
#endif
}


