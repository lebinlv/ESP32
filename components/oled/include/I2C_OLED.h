#ifndef I2C_OLED_H_
#define I2C_OLED_H_

#include "OLED.h"

class I2C_OLED : public OLED
{
  private:
    gpio_num_t SCL_pin;
    gpio_num_t SDA_pin;
    gpio_num_t RST_pin;

    /******      HardWare Operate Function      ******/
    void I2C_Start();                   // send I2C start signal and address byte
    void I2C_Stop();                    // send I2C stop signal
    void WriteByte(uint8_t data);       // write a byte
    void sendCommand(uint8_t command);  // send a single command
    void sendData(uint8_t data);        // send a single data

    /**
     * @brief: Setup column or page start and end address, only for horizontal or vertical addressing mode;
     * @param: seg_or_page: it can be OLED_COLUMN_ADDR or OLED_PAGE_ADDR;
     * */
    void setAddr(uint8_t seg_or_page, uint8_t startAddr, uint8_t endAddr);
    
    /*************************************************/

  public:
    // hardware init function.
    void Init(gpio_num_t SCL_pinNum, gpio_num_t SDA_pinNum, gpio_num_t RST_pinNum, uint8_t address = 0x78);

    void Refresh();
};

#endif