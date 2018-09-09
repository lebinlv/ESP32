#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "I2C_OLED.h"
#include "bluetoothSerial.h"
#include "fonts/DejaVu_Sans_12.h"

extern "C"
{
    void app_main();
}

void app_main()
{
    nvs_flash_init();
    I2C_OLED oled;
    oled.Init(GPIO_NUM_15, GPIO_NUM_4, GPIO_NUM_16);

    std::string name("TEST");
    BluetoothSerial bt_spp;
    bt_spp.begin(name);

    int length = 0;
    char *buffer;
    const char stop_str[] = "stop\r\n";

    //oled.drawHorizontalLine(0,0,128);
    oled.setFont(DejaVu_Sans_12);
    oled.drawString(0,0," Bluetooth SPP Test");
    //oled.Refresh();
    oled.drawHorizontalLine(0,13,128);
    oled.setPrintfArea(24,104,15,64);
    oled.setFont(DejaVu_Sans_10);


    // for(int i=0;i<12;i++){
    //      oled.printf("%s","stop123");
    //      vTaskDelay(1000/portTICK_PERIOD_MS);
    // }
    // oled.printfClear();



    while(1)
    {
        length = bt_spp.available();
        //ESP_LOGI("while()","%d",length);
        if(length){
            //ESP_LOGW("while()","%d",length);
            buffer = new char [length+1];
            for(int i=0; i<length; i++){
                buffer[i] = bt_spp.read();
            }
            //bt_spp.write(buffer, length);
            buffer[length] = '\0';
            ESP_LOGW("READ","%s", buffer);
            
            oled.printf("%s", buffer);
            
            if(strcmp(stop_str, buffer) == 0) {
                bt_spp.end();
                vTaskDelay(5000/portTICK_PERIOD_MS);
                bt_spp.begin(name);
                vTaskDelay(1000/portTICK_PERIOD_MS);
            }
            delete [] buffer;
        }
        vTaskDelay(200/portTICK_PERIOD_MS);
    }  
}
