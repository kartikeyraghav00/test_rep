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

static const char *TAG = "example";

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
#define LEDC_TEST_DUTY         (5000)           //divides the duty cycle % 0-100% into 0 - 6000 means higher the test duty cycle more smoothly the transition is seen
#define LEDC_TEST_FADE_TIME    (3000)           //Fade time in ms makes the transition within the specified time

uint8_t pin;
static uint8_t s_led_state = 0;

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

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
static bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    int taskAwoken = 0;

    if (param->event == LEDC_FADE_END_EVT) {
        SemaphoreHandle_t counting_sem = (SemaphoreHandle_t) user_arg;
        xSemaphoreGiveFromISR(counting_sem, &taskAwoken);
    }

    return (taskAwoken == pdTRUE);
}

void app_main(void)
{
    int ch;
    configure_led();
    //blink_led();
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
    

    while (1) {

        //Green LED faded
        printf("1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        
            ledc_set_fade_with_time(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, LEDC_FADE_NO_WAIT);
        
        printf("2. LEDC fade down to duty = 0\n");
        
            ledc_set_fade_with_time(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[0].speed_mode,
                    ledc_channel[0].channel, LEDC_FADE_NO_WAIT);
        
        vTaskDelay(500);


        //Blue LED faded
        printf("1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        
            ledc_set_fade_with_time(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, LEDC_FADE_NO_WAIT);
        
        printf("2. LEDC fade down to duty = 0\n");
        
            ledc_set_fade_with_time(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, LEDC_FADE_NO_WAIT);
        
        vTaskDelay(500);


        //Red LED faded
        printf("1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
        
            ledc_set_fade_with_time(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, LEDC_FADE_NO_WAIT);
        
        printf("2. LEDC fade down to duty = 0\n");
        
            ledc_set_fade_with_time(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, 0, LEDC_TEST_FADE_TIME);
            ledc_fade_start(ledc_channel[2].speed_mode,
                    ledc_channel[2].channel, LEDC_FADE_NO_WAIT);
        
        vTaskDelay(500);

    }
}
