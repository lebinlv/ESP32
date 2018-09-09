#ifndef BLUETOOTH_GENERIC_H_
#define BLUETOOTH_GENERIC_H_

#include "esp_bt.h"
#include "esp_bt_main.h"

extern uint8_t _bt_client;

// The function esp_bt_controller_init should be called only once;
// The function esp_bt_controller_enable should be called only once;
// go to "esp_bt.h" for detail;
// so we use _bt_controller_initd and _bt_controller_enabled to control the use of the two function
extern bool _bt_controller_initd;
extern bool _bt_controller_enabled;

bool initBtController();
bool enableBtController(esp_bt_mode_t btMode);
bool stopBtController();

bool startBluedroid();
bool stopBluedroid();

#endif
