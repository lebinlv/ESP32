#ifndef _BLUETOOTH_SERIAL_H_
#define _BLUETOOTH_SERIAL_H_

#include "sdkconfig.h"
#include "esp_spp_api.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)

typedef void(*esp_spp_cb_fun_t)(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

class BluetoothSerial
{
  public:
    BluetoothSerial();
    ~BluetoothSerial();

    bool begin(const char local_name[] = "ESP32");
    bool end();

    int available();
    int peek();
    size_t hasClient();
    
    char read();
    size_t write(uint8_t c);
    size_t write(const char *buffer, size_t size);
    void flush();

    int printf(char *format, ...) __attribute__((format(printf, 2, 3)));

  private:
    esp_spp_cb_fun_t usr_spp_cb_fun;

};

#endif /* #if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED) */

#endif /* #ifndef _BLUETOOTH_SERIAL_H_ */