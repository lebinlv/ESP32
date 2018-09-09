#include "I2C_OLED.h"
#include "esp_log.h"
static const uint16_t DELAY_600_NS = 120;
static const uint16_t DELAY_1_3_US = 240;

void I2C_OLED::Init(gpio_num_t SCL_pinNum, gpio_num_t SDA_pinNum, gpio_num_t RST_pinNum, uint8_t address)
{
    SCL_pin = SCL_pinNum;
    SDA_pin = SDA_pinNum;
    RST_pin = RST_pinNum;
    device_address = address;

    /******  GPIO Init    ******/
    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = ((1 << SCL_pin) | (1 << SDA_pin) | (1 << RST_pin));
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&gpio_conf);

    gpio_set_level(SCL_pin, 1); // Init SCL Pin:
    gpio_set_level(SDA_pin, 1); // Init SDA Pin:
    gpio_set_level(RST_pin, 1); // Init RST Pin:

    screenInit();
    clear();
}

/******  Basic Hardware Communication Function      ******/
void I2C_OLED::I2C_Start()
{
    uint16_t i = 0;
    gpio_set_level(SDA_pin, 0);
    for (i=0; i < DELAY_600_NS; i++); // t_HSTART > 600ns
    gpio_set_level(SCL_pin, 0);
    for (i=0; i < DELAY_600_NS; i++); // t_HSTART > 600ns
    WriteByte(device_address);
}

void I2C_OLED::I2C_Stop()
{    
    uint16_t i = 0;
    for (i=0; i < DELAY_600_NS; i++);    // t_SSTOP > 600ns
    gpio_set_level(SCL_pin, 1);
    gpio_set_level(SDA_pin, 0);
    for (i=0; i < DELAY_600_NS; i++);    // t_SSTOP > 600ns
    gpio_set_level(SDA_pin, 1);
    for (i = 0; i < DELAY_1_3_US; i++);  // t_IDLE > 1.3us(两次通信之间的时间间隔)
}

void I2C_OLED::sendCommand(uint8_t command)
{
    I2C_Start();
    WriteByte(0x80); //单次性写入命令
    WriteByte(command);
    I2C_Stop();
}

void I2C_OLED::sendData(uint8_t data)
{
    I2C_Start();
    WriteByte(0xC0); //单次性写入数据
    WriteByte(data);
    I2C_Stop();
}

void I2C_OLED::WriteByte(uint8_t data)
{
    uint16_t j = 0;
    for (uint16_t i = 0x80; i > 0; i >>= 1)
    {
        if (data & i)
            gpio_set_level(SDA_pin, 1);
        else
            gpio_set_level(SDA_pin, 0);
        for (j = 0; j < DELAY_1_3_US; j++); //delay time > 1.25us

        gpio_set_level(SCL_pin, 1);
        for (j = 0; j < DELAY_1_3_US; j++); //delay time > 1.25us

        gpio_set_level(SCL_pin, 0);
    }
    // wait ACK
    for (j = 0; j < DELAY_1_3_US; j++); //delay time > 1.25us
    gpio_set_level(SCL_pin, 1);
    for (j = 0; j < DELAY_1_3_US; j++); //delay time > 1.25us
    gpio_set_level(SCL_pin, 0);
}

void I2C_OLED::setAddr(uint8_t seg_or_page, uint8_t startAddr, uint8_t endAddr)
{
    I2C_Start();
    WriteByte(0x00); //连续性写入命令
    WriteByte(seg_or_page);
    WriteByte(startAddr);
    WriteByte(endAddr);
    I2C_Stop();
}

void I2C_OLED::Refresh()
{
    buffer.column_end = buffer.column_end < OLED_WIDTH-1 ? buffer.column_end : OLED_WIDTH-1;
    buffer.row_end = buffer.row_end < OLED_HEIGHT - 1 ? buffer.row_end : OLED_HEIGHT - 1;
    if ((buffer.column_start > buffer.column_end) || (buffer.row_start > buffer.row_end)) return;

    uint8_t page_start = buffer.row_start >> 3, page_end = buffer.row_end >> 3;

    setAddr(OLED_COLUMN_ADDR, buffer.column_start, buffer.column_end);
    setAddr(OLED_PAGE_ADDR, page_start, page_end);

    uint8_t x = 0, y = 0;

    I2C_Start();
    WriteByte(0x40); //连续性写入数据
    for (y=page_start; y <= page_end; y++)
        for (x=buffer.column_start; x <= buffer.column_end; x++)
            WriteByte(buffer.GRAM[x + y * OLED_WIDTH]);
    I2C_Stop();

    buffer.column_start = 0;
    buffer.column_end = OLED_WIDTH - 1;
    buffer.row_start = 0;
    buffer.row_end = OLED_HEIGHT - 1;
}
