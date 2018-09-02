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
#include "fonts/DejaVu_Sans_10.h"

extern "C" {
    void app_main();
}

void app_main()
{
    I2C_OLED oled;
    printf("Start initing oled...\n");
    oled.Init(GPIO_NUM_15, GPIO_NUM_4, GPIO_NUM_16);


    /****** drawVerticalLine Function Test ******/
    ESP_LOGI("OLED_TEST", "drawVerticalLine Function Test");
    int j = 0;
    for (int y = 0; y < 8; y++)
        for (int i = 0; i < 128; i++)
        {
            j = j & 7;
            oled.drawVerticalLine(i, j + 8 * y, 8 - j, 1, oled.INVERSE);
            j++;
        }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** drawHorizontalLine Function Test ******/
    ESP_LOGI("OLED_TEST", "drawHorizontalLine Function Test");
    for (int y = 0; y < 64; y++)
    {
        oled.drawHorizontalLine(0, y, 128-2*y);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** drawRect Function Test ******/
    ESP_LOGI("OLED_TEST", "drawRect Function Test");
    for (int i = 1; 8 * i <= 128; i++)
    {
        oled.drawRect(64 - 4 * i, 32 - 2 * i, 8 * i, 4 * i);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** drawFilledRect Function Test ******/
    ESP_LOGI("OLED_TEST", "drawFilledRect Function Test");
    for(int i = 1; 8*i<=128; i++)
    {
        oled.drawFilledRect(64-4*i,32-2*i,8*i,4*i,oled.INVERSE);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** drawFilledCircle Function Test ******/
    ESP_LOGI("OLED_TEST", "drawFilledCircle Function Test");
    oled.drawFilledCircle(0, 0, 64);
    oled.drawFilledCircle(127, 63, 64);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** drawCircle Function Test ******/
    ESP_LOGI("OLED_TEST", "drawCircle Function Test");
    for(int i=0; i<=128; i+=16)
        oled.drawCircle(i,32,8);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** printf Function Test 1 ******/
    ESP_LOGI("OLED_TEST", "printf Function Test 1");
    oled.printf("draw function test done...\n1234567890");
    vTaskDelay(5000 / portTICK_PERIOD_MS);


    /****** printf Function Test 2 ******/
    ESP_LOGI("OLED_TEST", "printf Function Test 2");
    oled.setFont(DejaVu_Sans_10);
    oled.printf("I have changed font!\n%d",123);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    /****** drawString Function Test ******/
    ESP_LOGI("OLED_TEST", "drawString Function Test");
    oled.drawString(0, 0, "DrawString Test\nAll test OK!");
    oled.drawString(64, 20, "!@#$%^&*()_+\nCongratulations!!!\nAll Fine !");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    oled.clear();


    printf("Start drawing image...\n");
    oled.drawImage(0, 16, Mini_WiFi_Logo_width, Mini_WiFi_Logo_height, Mini_WiFi_Logo);
    oled.setPrintfArea(64,128,0,64);
    ESP_LOGI("OLED_TEST", "setPrintfArea Function Test OK");
    oled.printfClear();
    ESP_LOGI("OLED_TEST", "printfClear Function Test OK");
    oled.setFont(ArialMT_Plain_10);
    float idx = 1.1;
    while(1)
    {
        oled.printf("\nIt is %f line!",idx);
        printf("\nIt is %f line!",idx);
        vTaskDelay(2000/portTICK_PERIOD_MS);
        idx++;
    }
}

