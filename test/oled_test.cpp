/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "OLED.h"
#include "images/images.h"

extern "C" {
    void app_main();
}

void app_main()
{
    I2C_OLED oled;
    printf("Start initing oled...\n");
    oled.Init(GPIO_NUM_15, GPIO_NUM_4, GPIO_NUM_16);
    int j = 0;
    for (int y = 0; y < 8; y++)
        for (int i = 0; i < 128; i++)
        {
            j = j & 7;
            oled.drawVerticalLine(i, j + 8 * y, 8 - j, 1, oled.INVERSE);
            j++;
        }
    oled.clear();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Start drawing image...\n");
    oled.drawFastImage(0, 0, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo);
    while(1)
    {
        vTaskDelay(5000/portTICK_PERIOD_MS);
        printf("Waiting...");
    }
    ESP_LOGI("test","test");
}

