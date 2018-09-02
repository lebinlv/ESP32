#include "wifi_sta.h"
#include "esp_log.h"

extern "C" {
    void app_main();
}

void app_main()
{
    WIFISTAClass wifi;
    wifi.startSmartConfig();
    printf("waiting for smartconfig done!\n");
    while (!wifi.isSmartConfigDone())
    {}
    ESP_LOGI("SmartConfig TSET", "SmartConfig Done!");
    while(1)
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        printf("Waiting...");
    }
}

