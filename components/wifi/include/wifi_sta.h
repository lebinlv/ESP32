#ifndef WIFISTA_H_
#define WIFISTA_H_
#include "wifi_generic.h"
#include "esp_smartconfig.h"

class WIFISTAClass : public WIFIGenericClass
{
  private:
    static bool _smartConfigStarted;
    static bool _smartConfigDone;
    static void _smartConfigCallback(smartconfig_status_t status, void *pdata);

  public:
    WIFISTAClass();
    ~WIFISTAClass();
    bool startSmartConfig(smartconfig_type_t type = SC_TYPE_ESPTOUCH_AIRKISS);
    bool stopSmartConfig();
    bool isSmartConfigDone();

};

#endif