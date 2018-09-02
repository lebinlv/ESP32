#include "wifi_sta.h"
#include "esp_wifi.h"
#include "esp_log.h"

bool WIFISTAClass::_smartConfigStarted = false;
bool WIFISTAClass::_smartConfigDone = false;

WIFISTAClass::WIFISTAClass() {}
WIFISTAClass::~WIFISTAClass() {}

bool WIFISTAClass::startSmartConfig(smartconfig_type_t type)
{
    if (_smartConfigStarted) return false;
    
    if(!setMode(WIFI_MODE_STA)) return false;

    esp_smartconfig_set_type(type);
    esp_err_t err = esp_smartconfig_start(WIFISTAClass::_smartConfigCallback);
    if (err == ESP_OK)
    {
        _smartConfigStarted = true;
        _smartConfigDone = false;
        return true;
    }
    return false;
}

bool WIFISTAClass::stopSmartConfig()
{
    if (!_smartConfigStarted)
        return true;

    if (esp_smartconfig_stop() == ESP_OK)
    {
        _smartConfigStarted = false;
        return true;
    }

    return false;
}

bool WIFISTAClass::isSmartConfigDone()
{
    return _smartConfigDone;
}

void WIFISTAClass::_smartConfigCallback(smartconfig_status_t status, void *pdata)
{
    switch (status)
    {
    case SC_STATUS_WAIT:
        ESP_LOGI("smartConfig", "SC_STATUS_WAIT");
        break;
    case SC_STATUS_FIND_CHANNEL:
        ESP_LOGI("smartConfig", "SC_STATUS_FINDING_CHANNEL");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        ESP_LOGI("smartConfig", "SC_STATUS_GETTING_SSID_PSWD");
        break;
    case SC_STATUS_LINK:{
        ESP_LOGI("smartConfig", "SC_STATUS_LINK");
        wifi_config_t *wifi_config = (wifi_config_t*)pdata;
        ESP_LOGI("smartConfig", "SSID:%s", wifi_config->sta.ssid);
        ESP_LOGI("smartConfig", "PASSWORD:%s", wifi_config->sta.password);
        esp_wifi_disconnect();
        esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
        esp_wifi_connect();
        } break;
    case SC_STATUS_LINK_OVER:{
        ESP_LOGI("smartConfig", "SC_STATUS_LINK_OVER");
        if (pdata) {
            uint8_t *phone_ip = (uint8_t*)pdata;
            ESP_LOGI("smartConfig", "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
        }
        _smartConfigDone = true;
        if(esp_smartconfig_stop()==ESP_OK)
            _smartConfigStarted = false;
        } break;
    default:
        break;
    }
}