#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "bluetooth_generic.h"
#include "bluetoothSerial.h"

#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "esp_log.h"

#define QUEUE_SIZE 256

const char * _spp_server_name = "ESP32_SPP_SERVER";
static uint32_t _spp_client = 0;
static xQueueHandle _spp_queue = NULL;

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI("SPP", "ESP_SPP_INIT_EVT");
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, _spp_server_name);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT://discovery complete
        ESP_LOGI("SPP", "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT://Client connection open
        ESP_LOGI("SPP", "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT://Client connection closed
        _spp_client = 0;
        ESP_LOGI("SPP", "ESP_SPP_CLOSE_EVT");
        break;
    case ESP_SPP_START_EVT://server started
        ESP_LOGI("SPP", "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT://client initiated a connection
        ESP_LOGI("SPP", "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT://connection received data
        ESP_LOGI("SPP", "ESP_SPP_DATA_IND_EVT len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
        
        if (_spp_queue != NULL){
            for (int i = 0; i < param->data_ind.len; i++)
                xQueueSend(_spp_queue, param->data_ind.data + i, (TickType_t)0);
        } else {
            ESP_LOGE("SPP", "SerialQueueBT ERROR");
        }
        break;
    case ESP_SPP_CONG_EVT://connection congestion status changed
        ESP_LOGI("SPP", "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT://write operation completed
        ESP_LOGI("SPP", "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT://Server connection open
        _spp_client = param->open.handle;
        ESP_LOGI("SPP", "ESP_SPP_SRV_OPEN_EVT");
        break;
    default:
        break;
    }
}


static bool _init_bt_spp(const char *deviceName)
{

    if(!initBtController()) return false;

    if(!enableBtController(ESP_BT_MODE_CLASSIC_BT)) return false;

    if(!startBluedroid()) return false;

    if (esp_spp_register_callback(esp_spp_cb) != ESP_OK){
        ESP_LOGE("SPP", "%s spp register failed\n", __func__);
        return false;
    }

    if (esp_spp_init(ESP_SPP_MODE_CB) != ESP_OK){
        ESP_LOGE("SPP", "%s spp init failed\n", __func__);
        return false;
    }

    _spp_queue = xQueueCreate(QUEUE_SIZE, sizeof(uint8_t)); //initialize the queue
    if (_spp_queue == NULL){
        ESP_LOGE("SPP", "%s Queue creation error\n", __func__);
        return false;
    }
    esp_bt_dev_set_device_name(deviceName);

    // the default BTA_DM_COD_LOUDSPEAKER does not work with the macOS BT stack
    // esp_bt_cod_t cod;
    // cod.major = 0b00001;
    // cod.minor = 0b000100;
    // cod.service = 0b00000010110;
    // if (esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD) != ESP_OK) {
    //     ESP_LOGE("SPP", "%s set cod failed\n", __func__);
    //     return false;
    // }

    return true;
}

static bool _stop_bt_spp()
{
    if(_spp_client){
        if(esp_spp_disconnect(_spp_client)) return false;
        _spp_client = 0;
    }
    if(esp_spp_deinit()) return false;
    if(!stopBluedroid()) return false;
    if(!stopBtController()) return false;
    
    return true;
}

BluetoothSerial::BluetoothSerial()
{

}

BluetoothSerial::~BluetoothSerial(void)
{
    _stop_bt_spp();
    vQueueDelete(_spp_queue);
}

bool BluetoothSerial::begin(const char *localName)
{
    return _init_bt_spp(localName);
}

int BluetoothSerial::available(void)
{
    if (!_spp_client || _spp_queue == NULL){
        return 0;
    }
    return uxQueueMessagesWaiting(_spp_queue);
}

int BluetoothSerial::peek(void)
{
    uint8_t c;
    if (xQueuePeek(_spp_queue, &c, 0)){
        return c;
    }
    return -1;
}

bool BluetoothSerial::hasClient(void)
{
    if (_spp_client)
        return true;
	
    return false;
}

char BluetoothSerial::read(void)
{
    if (available()){
        if (!_spp_client || _spp_queue == NULL){
            return 0;
        }

        char c;
        if (xQueueReceive(_spp_queue, &c, 0)){
            return c;
        }
    }
    return 0;
}

size_t BluetoothSerial::write(uint8_t c)
{
    if (!_spp_client){
        return 0;
    }

    uint8_t buffer[1];
    buffer[0] = c;
    esp_err_t err = esp_spp_write(_spp_client, 1, buffer);
    return (err == ESP_OK) ? 1 : 0;
}

size_t BluetoothSerial::write(const char *buffer, size_t size)
{
    if (!_spp_client){
        return 0;
    }

    esp_err_t err = esp_spp_write(_spp_client, size, (uint8_t *)buffer);
    return (err == ESP_OK) ? size : 0;
}

void BluetoothSerial::flush()
{
    if (_spp_client){
        int qsize = available();
        uint8_t buffer[qsize];
        esp_spp_write(_spp_client, qsize, buffer);
    }
}

void BluetoothSerial::end()
{
    _stop_bt_spp();
    vQueueDelete(_spp_queue);
}

#endif