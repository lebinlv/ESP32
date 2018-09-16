#ifndef _BLUETOOTH_SERIAL_H_
#define _BLUETOOTH_SERIAL_H_

#include "sdkconfig.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)

class BluetoothSerial
{
  public:
    BluetoothSerial(void);
    ~BluetoothSerial(void);

    bool begin(const char *deviceName = "ESP32");
    int available(void);
    int peek(void);
    bool hasClient(void);
    char read(void);
    size_t write(uint8_t c);
    size_t write(const char *buffer, size_t size);
    void flush();
    void end(void);

};

#endif

#endif