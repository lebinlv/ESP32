#ifndef _BLUETOOTH_SERIAL_H_
#define _BLUETOOTH_SERIAL_H_

#include "sdkconfig.h"
#include "esp_spp_api.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)

typedef void(*esp_spp_cb_fun_t)(esp_spp_cb_event_t event);

class BluetoothSerial
{
  public:
    BluetoothSerial();
    ~BluetoothSerial();

    bool begin(const char local_name[] = "ESP32");
    bool end();

    size_t available();
    size_t peek();
    size_t hasClient();
    
    char read();
    size_t write(uint8_t c);
    size_t write(const char *buffer, size_t size);
    size_t flush();

    int printf(char *format, ...) __attribute__((format(printf, 2, 3)));

    void addSppCbFun(esp_spp_cb_fun_t usr_fun);

};

#endif /* #if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED) */

#endif /* #ifndef _BLUETOOTH_SERIAL_H_ */