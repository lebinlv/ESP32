#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_generic.h"

#include <vector>

#define TAG "WIFIGeneric"
#define NETWORK_EVENT_TASK_RUNNING_CORE 1

static bool _tcpip_initialized = false;
static bool _wifi_initialized = false;
static bool _esp_wifi_started = false;

static xQueueHandle _network_event_queue;
static TaskHandle_t _network_event_task_handle = NULL;
static EventGroupHandle_t _network_event_group = NULL;

static void _network_event_task(void *parm)
{
    system_event_t *event = NULL;
    for (;;)
    {
        if (xQueueReceive(_network_event_queue, &event, portMAX_DELAY) == pdTRUE)
            WIFIGenericClass::_wifiEventCallback(parm, event);
    }
    vTaskDelete(NULL);
    _network_event_task_handle = NULL;
}

static esp_err_t _network_event_cb(void *parm, system_event_t *event)
{
    if (xQueueSend(_network_event_queue, &event, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGW(TAG, "Network Event Queue Send Failed!");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static bool _start_network_event_task()
{
    if (!_network_event_group)
    {
        _network_event_group = xEventGroupCreate();
        if (!_network_event_group)
        {
            ESP_LOGE(TAG, "Network Event Group Create Failed!");
            return false;
        }
        xEventGroupSetBits(_network_event_group, WIFI_DNS_IDLE_BIT);
    }
    if (!_network_event_queue)
    {
        _network_event_queue = xQueueCreate(32, sizeof(system_event_t *));
        if (!_network_event_queue)
        {
            ESP_LOGE(TAG,"Network Event Queue Create Failed!");
            return false;
        }
    }
    if (!_network_event_task_handle)
    {
        xTaskCreatePinnedToCore(_network_event_task, "network_event", 4096, NULL, 2 | portPRIVILEGE_BIT,
                                &_network_event_task_handle, NETWORK_EVENT_TASK_RUNNING_CORE);
        if (!_network_event_task_handle)
        {
            ESP_LOGE(TAG, "Network Event Task Start Failed!");
            return false;
        }
    }
    return esp_event_loop_init(&_network_event_cb, NULL) == ESP_OK;
}

void tcpipInit()
{
    if (!_tcpip_initialized && _start_network_event_task())
    {
        _tcpip_initialized = true;
        tcpip_adapter_init();
    }
}

static bool wifiInit()
{
    if (!_wifi_initialized)
    {
        tcpipInit();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err = esp_wifi_init(&cfg);
        if (err)
        {
            ESP_LOGE(TAG, "esp_wifi_init %d", err);
            return false;
        }
        //if (!persistent) esp_wifi_set_storage(WIFI_STORAGE_RAM);
        _wifi_initialized = true;
    }
    return true;
}

static bool espWiFiStart()
{
    if (_esp_wifi_started) return true;
    if(nvs_flash_init() != ESP_OK) return false;
    if (!wifiInit()) return false;
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_start %d", err);
        return false;
    }
    _esp_wifi_started = true;
    system_event_t event;
    event.event_id = SYSTEM_EVENT_WIFI_READY;
    WIFIGenericClass::_wifiEventCallback(nullptr, &event);

    return true;
}

static bool espWiFiStop()
{   
    if (!_esp_wifi_started) return true;
    esp_err_t err = esp_wifi_stop();
    if (err)
    {
        ESP_LOGE(TAG, "Could not stop WiFi! %u", err);
        _esp_wifi_started = true;
        return false;
    }
    _esp_wifi_started = false;
    return true;
}

typedef struct wifiEventCbList{
    static wifiEventCbFunc_id_t totalFunc;
    wifiEventCbFunc_id_t id;
    wifiEventCb cbFunc;
    system_event_id_t event_id;

    wifiEventCbList(): id(totalFunc++) {}
} wifiEventCbList_t;
wifiEventCbFunc_id_t wifiEventCbList::totalFunc = 1;

static std::vector<wifiEventCbList_t> eventCbFuncList;

WIFIGenericClass::WIFIGenericClass() { }
WIFIGenericClass::~WIFIGenericClass() { }

uint32_t WIFIGenericClass::setStatusBits(uint32_t bitsToSet)
{
    if (!_network_event_group) return 0;

    return xEventGroupSetBits(_network_event_group, bitsToSet);
}

uint32_t WIFIGenericClass::clearStatusBits(uint32_t bitsToSet)
{
    if (!_network_event_group) return 0;

    return xEventGroupClearBits(_network_event_group, bitsToSet);
}

uint32_t WIFIGenericClass::getStatusBits()
{
    if (!_network_event_group) return 0;

    return xEventGroupGetBits(_network_event_group);
}

uint32_t WIFIGenericClass::waitStatusBits(uint32_t bitsToWaitFor, uint32_t timeout_ms)
{
    if (!_network_event_group) return 0;

    uint32_t uxBits = xEventGroupWaitBits(
        _network_event_group,             // The event group being tested.
        bitsToWaitFor,                    // The bits within the event group to wait for.
        pdFALSE,                          // bitsToWaitFor should be cleared before returning.
        pdTRUE,                           // Don't wait for both bits, either bit will do.
        timeout_ms / portTICK_PERIOD_MS); // Wait a maximum of timeout_ms for either bit to be set.

    return uxBits & bitsToWaitFor;
}

wifiEventCbFunc_id_t WIFIGenericClass::addWifiEventCbFunc(wifiEventCb newFunc, system_event_id_t event_id)
{
    if (!newFunc) return 0;

    wifiEventCbList_t newEventCb;

    newEventCb.cbFunc = newFunc;
    newEventCb.event_id= event_id;

    eventCbFuncList.push_back(newEventCb);
    return newEventCb.id;
}

void WIFIGenericClass::delWifiEventCbFunc(wifiEventCb funcToDel, system_event_id_t event_id)
{
    if (!funcToDel) return;

    for (uint32_t i = 0; i < eventCbFuncList.size(); i++)
    {
        wifiEventCbList_t temp = eventCbFuncList[i];
        if (temp.cbFunc == funcToDel && temp.event_id == event_id)
            eventCbFuncList.erase(eventCbFuncList.begin() + i);
    }
}

void WIFIGenericClass::delWifiEventCbFunc(wifiEventCbFunc_id_t idToDel)
{
    for (uint32_t i = 0; i < eventCbFuncList.size(); i++)
    {
        wifiEventCbList_t temp = eventCbFuncList[i];
        if (temp.id == idToDel)
            eventCbFuncList.erase(eventCbFuncList.begin() + i);
    }
}

const char *system_event_names[] = {
    "WIFI_READY", "SCAN_DONE", "STA_START", "STA_STOP", "STA_CONNECTED",
    "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_LOST_IP",
    "STA_WPS_ER_SUCCESS", "STA_WPS_ER_FAILED", "STA_WPS_ER_TIMEOUT",
    "STA_WPS_ER_PIN", "AP_START", "AP_STOP", "AP_STACONNECTED",
    "AP_STADISCONNECTED", "AP_STAIPASSIGNED", "AP_PROBEREQRECVED", "GOT_IP6", "ETH_START",
    "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "MAX"};
const char *system_event_reasons[] = {
    "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE",
    "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE",
    "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD",
    "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT",
    "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID",
    "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION",
    "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED",
    "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT"};
#define reason2str(r) ((r > 176) ? system_event_reasons[r - 176] : system_event_reasons[r - 1])

esp_err_t WIFIGenericClass::_wifiEventCallback(void *arg, system_event_t *event)
{
    ESP_LOGI(TAG, "Event: %d - %s", event->event_id, system_event_names[event->event_id]);
    /*
    switch(event->event_id)
    {
    case SYSTEM_EVENT_SCAN_DONE:
        WiFiScanClass::_scanDone();
        break;
    case SYSTEM_EVENT_STA_START:
        WiFiSTAClass::_setStatus(WL_DISCONNECTED);
        setStatusBits(STA_STARTED_BIT);
        break;
    case SYSTEM_EVENT_STA_STOP:
        WiFiSTAClass::_setStatus(WL_NO_SHIELD);
        clearStatusBits(STA_STARTED_BIT | STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
        setStatusBits(STA_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED: 
    {
        uint8_t reason = event->event_info.disconnected.reason;
        ESP_LOGW(TAG,"Reason: %u - %s", reason, reason2str(reason));
        switch(reason)
        {
        case WIFI_REASON_NO_AP_FOUND:
            WiFiSTAClass::_setStatus(WL_NO_SSID_AVAIL);
            break;
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
            WiFiSTAClass::_setStatus(WL_CONNECT_FAILED);
            break;
        case WIFI_REASON_BEACON_TIMEOUT:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            WiFiSTAClass::_setStatus(WL_CONNECTION_LOST);
            break;
        case WIFI_REASON_AUTH_EXPIRE:
            break;
        default:
            WiFiSTAClass::_setStatus(WL_DISCONNECTED);
            break;
        } //switch(reason)
        clearStatusBits(STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
        if (((reason == WIFI_REASON_AUTH_EXPIRE) ||
             (reason >= WIFI_REASON_BEACON_TIMEOUT && reason != WIFI_REASON_AUTH_FAIL)) &&
            WiFi.getAutoReconnect()) {
            WiFi.enableSTA(false);
            WiFi.enableSTA(true);
            WiFi.begin();
        }
    } break; //case SYSTEM_EVENT_STA_DISCONNECTED
    case SYSTEM_EVENT_STA_GOT_IP:
        WiFiSTAClass::_setStatus(WL_CONNECTED);
        setStatusBits(STA_HAS_IP_BIT | STA_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_LOST_IP:
        WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
        clearStatusBits(STA_HAS_IP_BIT);
        break;
    case SYSTEM_EVENT_AP_START:
        setStatusBits(AP_STARTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STOP:
        clearStatusBits(AP_STARTED_BIT | AP_HAS_CLIENT_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        setStatusBits(AP_HAS_CLIENT_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    {
        wifi_sta_list_t clients;
        if (esp_wifi_ap_get_sta_list(&clients) != ESP_OK || !clients.num)
            clearStatusBits(AP_HAS_CLIENT_BIT);
    }break;
    case SYSTEM_EVENT_ETH_START:
        setStatusBits(ETH_STARTED_BIT);
        break;
    case SYSTEM_EVENT_ETH_STOP:
        clearStatusBits(ETH_STARTED_BIT | ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
        break;
    case SYSTEM_EVENT_ETH_CONNECTED:
        setStatusBits(ETH_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        clearStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT);
        break;
    case SYSTEM_EVENT_GOT_IP6:
        switch (event->event_info.got_ip6.if_index)
        {
        case TCPIP_ADAPTER_IF_AP:
            setStatusBits(AP_HAS_IP6_BIT);
            break;
        case TCPIP_ADAPTER_IF_STA:
            setStatusBits(STA_CONNECTED_BIT | STA_HAS_IP6_BIT);
            break;
        case TCPIP_ADAPTER_IF_ETH:
            setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP6_BIT);
            break;
        }break; //case SYSTEM_EVENT_GOT_IP6
    } //switch(event->event_id)
    */
    for (uint32_t i = 0; i < eventCbFuncList.size(); i++)
    {
        wifiEventCbList_t entry = eventCbFuncList[i];
        if (entry.cbFunc)
        {
            if (entry.event_id == SYSTEM_EVENT_MAX || entry.event_id == event->event_id)
                entry.cbFunc(event);
        }
    }
    return ESP_OK;
}

bool WIFIGenericClass::setMode(wifi_mode_t newMode)
{
    wifi_mode_t currentMode = getMode();

    // If the new mode is the same as the current mode, do nothing
    if (currentMode == newMode) { return true; }
    // If current mode = WIFI_MODE_NULL and new mode != WIFI_MODE_NULL, start wifi firstly
    if (!currentMode && newMode)
    {
        if (!espWiFiStart()) { return false; }
    }
    // If current mode != WIFI_MODE_NULL and new mode = WIFI_MODE_NULL, we should stop wifi
    else if (currentMode && !newMode) { return espWiFiStop(); }

    esp_err_t err = esp_wifi_set_mode(newMode);
    if (err)
    {
        ESP_LOGE("Change WiFi Mode", "Could not set mode! %u", err);
        return false;
    }
    return true;
}

wifi_mode_t WIFIGenericClass::getMode()
{
    if (!_esp_wifi_started) return WIFI_MODE_NULL;

    wifi_mode_t currentMode;
    if (esp_wifi_get_mode(&currentMode) == ESP_ERR_WIFI_NOT_INIT)
    {
        ESP_LOGW(TAG, "WiFi not started");
        return WIFI_MODE_NULL;
    }
    return currentMode;
}

bool WIFIGenericClass::setTxPower(wifi_power_t power)
{
    if ((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0)
    {
        ESP_LOGW(TAG, "Neither AP or STA has been started");
        return false;
    }
    return esp_wifi_set_max_tx_power((int8_t)power) == ESP_OK;
}

wifi_power_t WIFIGenericClass::getTxPower()
{
    int8_t power;
    if ((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0)
    {
        ESP_LOGW(TAG, "Neither AP or STA has been started");
        return WIFI_POWER_19_5dBm;
    }
    if (esp_wifi_get_max_tx_power(&power))
    {
        return WIFI_POWER_19_5dBm;
    }
    return (wifi_power_t)power;
}