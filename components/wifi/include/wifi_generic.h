#ifndef WIFI_GENERIC_H_
#define WIFI_GENERIC_H_

#include "esp_event_loop.h"

typedef void(*wifiEventCb)(system_event_t *event);

typedef uint8_t wifiEventCbFunc_id_t;

typedef enum
{
    WIFI_POWER_19_5dBm = 78,   // 19.5dBm
    WIFI_POWER_19dBm = 76,     // 19dBm
    WIFI_POWER_18_5dBm = 74,   // 18.5dBm
    WIFI_POWER_17dBm = 68,     // 17dBm
    WIFI_POWER_15dBm = 60,     // 15dBm
    WIFI_POWER_13dBm = 52,     // 13dBm
    WIFI_POWER_11dBm = 44,     // 11dBm
    WIFI_POWER_8_5dBm = 34,    // 8.5dBm
    WIFI_POWER_7dBm = 28,      // 7dBm
    WIFI_POWER_5dBm = 20,      // 5dBm
    WIFI_POWER_2dBm = 8,       // 2dBm
    WIFI_POWER_1dBm = -4 // -1dBm
} wifi_power_t;

static const int AP_STARTED_BIT     = BIT0;
static const int AP_HAS_IP6_BIT     = BIT1;
static const int AP_HAS_CLIENT_BIT  = BIT2;
static const int STA_STARTED_BIT    = BIT3;
static const int STA_CONNECTED_BIT  = BIT4;
static const int STA_HAS_IP_BIT     = BIT5;
static const int STA_HAS_IP6_BIT    = BIT6;
static const int ETH_STARTED_BIT    = BIT7;
static const int ETH_CONNECTED_BIT  = BIT8;
static const int ETH_HAS_IP_BIT     = BIT9;
static const int ETH_HAS_IP6_BIT    = BIT10;
static const int WIFI_SCANNING_BIT  = BIT11;
static const int WIFI_SCAN_DONE_BIT = BIT12;
static const int WIFI_DNS_IDLE_BIT  = BIT13;
static const int WIFI_DNS_DONE_BIT  = BIT14;

class WIFIGenericClass
{
  public:
    WIFIGenericClass();
    ~WIFIGenericClass();

    static esp_err_t _wifiEventCallback(void *arg, system_event_t *event);

    wifiEventCbFunc_id_t addWifiEventCbFunc(wifiEventCb newFunc, system_event_id_t event_id = SYSTEM_EVENT_MAX);
    void delWifiEventCbFunc(wifiEventCb funcToDel, system_event_id_t event_id);
    void delWifiEventCbFunc(wifiEventCbFunc_id_t idToDel);

    static uint32_t getStatusBits();
    static uint32_t waitStatusBits(uint32_t bitsToWaitFor, uint32_t timeout_ms);

    static bool setMode(wifi_mode_t newMode);
    static wifi_mode_t getMode();

    bool setTxPower(wifi_power_t power);
    wifi_power_t getTxPower();

  protected:
    static uint32_t setStatusBits(uint32_t bitsToSet);
    static uint32_t clearStatusBits(uint32_t bitsToSet);

  protected: 
    friend class WIFISTAClass;
};



#endif