#include "freertos/FreeRTOS.h"
#include "bluetooth_generic.h"
#include "esp_log.h"

#define TAG "Bluetooth"

uint8_t _bt_client = 0;

bool _bt_controller_initd = false;
bool _bt_controller_enabled = false;

bool initBtController()
{
    if(_bt_controller_initd) return true;

    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE("TAG", "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }
    _bt_controller_initd = true;
    return true;
}

bool enableBtController(esp_bt_mode_t btMode)
{
    if(_bt_controller_enabled) return true;
    esp_err_t ret;
    if ((ret = esp_bt_controller_enable(btMode)) != ESP_OK) {
        ESP_LOGE("TAG", "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }
    _bt_controller_enabled = true;
    return true;
}

bool stopBtController()
{
    esp_err_t ret;
    if(_bt_controller_enabled) {
        if((ret = esp_bt_controller_disable()) != ESP_OK) {
            ESP_LOGE(TAG, "%s(), bt controller disable failed: %s\n", __func__, esp_err_to_name(ret));
            return false;
        }
        _bt_controller_enabled = false;
    }

    // esp_bt_controller_init cannot called after this function.
    // if(_bt_controller_initd) {
    //     if((ret = esp_bt_controller_deinit()) != ESP_OK) {
    //         ESP_LOGE("TAG", "%s(), bt controller deinit failed: %s\n", __func__, esp_err_to_name(ret));
    //         return false;
    //     }
    //     _bt_controller_initd = false;
    // }
    return true;
}

bool startBluedroid()
{
    esp_bluedroid_status_t bluedroid_state = esp_bluedroid_get_status();

    if (bluedroid_state == ESP_BLUEDROID_STATUS_UNINITIALIZED){
        if (esp_bluedroid_init()) {
            ESP_LOGE("TAG", "%s initialize bluedroid failed\n", __func__);
            return false;
        }
    }
    
    if (bluedroid_state != ESP_BLUEDROID_STATUS_ENABLED){
        if (esp_bluedroid_enable()) {
            ESP_LOGE("TAG", "%s enable bluedroid failed\n", __func__);
            return false;
        }
    }

    return true;
}

bool stopBluedroid()
{
    esp_bluedroid_status_t bluedroid_state = esp_bluedroid_get_status();

    if (bluedroid_state == ESP_BLUEDROID_STATUS_ENABLED){
        if (esp_bluedroid_disable()) {
            ESP_LOGE("TAG", "%s disable bluedroid failed\n", __func__);
            return false;
        }
    }
    
    if (bluedroid_state != ESP_BLUEDROID_STATUS_UNINITIALIZED){
        if (esp_bluedroid_deinit()) {
            ESP_LOGE("TAG", "%s deinit bluedroid failed\n", __func__);
            return false;
        }
    }
    return true;
}