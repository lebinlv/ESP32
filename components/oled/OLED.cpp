#include <iostream>
#include <string.h>
#include "OLED.h"

I2C_OLED::I2C_OLED()
{
    buffer.GRAM = new uint8_t [1024];  // 1024 = OLED_WIDTH*OLED_HEIGHT/8
    //buffer.GRAM = &temp[0];
    buffer.column_start = 0;
    buffer.column_end = 127;
    buffer.page_start = 0;
    buffer.page_end = 7;
    
    GRAM_bk = buffer.GRAM;
    fontData = ArialMT_Plain_10;
}
I2C_OLED::~I2C_OLED()
{
    buffer.GRAM = GRAM_bk;
    delete [] buffer.GRAM;
}

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
    gpio_conf.pin_bit_mask = ((1<<SCL_pin)|(1<<SDA_pin)|(1<<RST_pin));
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&gpio_conf);

    // Init SCL Pin:
    gpio_set_level(SCL_pin, 1);
    // Init SDA Pin:
    gpio_set_level(SDA_pin, 1);
    // Init RST Pin:
    gpio_set_level(RST_pin, 1);

    /******  Screen Init ******/
    sendCommand(OLED_DISPLAY_OFF);          // Turn off the screen, but still consume energy
    sendCommand(OLED_SET_DCLK_CLK);         // Set CLK frequence and the divide ratio to generate DCLK from CLK
    sendCommand(0x80);                      // Increase speed of the display max ~96Hz
    sendCommand(OLED_SET_MUX_Ratio);        // Set Multiplex Ratio (from 0 ~ 63)
    sendCommand(0x3F);
    sendCommand(OLED_SET_DISPLAY_OFFSET);   // Set display offset
    sendCommand(0x00);
    sendCommand(OLED_SET_START_LINE);       // Set start line address, 0x40(RESET)
    sendCommand(OLED_CHARGE_PUMP);          // Set charge pump
    sendCommand(0x14);                      // Enable charge pump
    sendCommand(OLED_MEMORY_MODE);          // Set memory addressing mode
    sendCommand(0x00);                      // Set to Horizontal addressing mode
    
    // 将引脚位于屏幕上方作为正方向：
    sendCommand(OLED_SEG_REMAP_A1);         // SEG_REMAP command only affects subsequent data input. Data already stored in GDDRAM will have no changes
                                            // 0xA0: column address 0 is mapped to SEG0(RESET);
                                            // 0xA1: column address 127 is mapped to SEG0

    sendCommand(OLED_COM_SCAN_C8);          // 0xC0: normal mode (RESET) Scan from COM0 to COM[N –1]
                                            // 0xC8: remapped mode. Scan from COM[N-1] to COM0
                                            
    sendCommand(OLED_SET_COMPINS);          // Set COM Pins Hardware Configuration
    sendCommand(0x12);
    sendCommand(OLED_SET_CONTRAST);         // Set Contrast
    sendCommand(0x0F);
    sendCommand(OLED_SET_PRE_CHARGE);       // Set Pre-charge Period
    sendCommand(0xF1);
    sendCommand(OLED_SET_VCOMH_DESELECT);   // Set Vcomh regular output
    sendCommand(0x30);
    sendCommand(OLED_DISPLAY_ALLON_RESUME); // 0xA4: enable display outputs according to the GDDRAM contents.
                                            // 0xA5: Entire display ON, Output ignores RAM content
    sendCommand(OLED_NORMAL_DISPLAY);       // 0xA6: normal display(RESET); 0xA7 inverse display
    sendCommand(OLED_DISPLAY_ON);           // Turn on the screen
    
    clear();

}
void I2C_OLED::Refresh()
{
    if ((buffer.column_start > buffer.column_end) || (buffer.page_start > buffer.page_end))
        return;
    setAddr(OLED_COLUMN_ADDR, buffer.column_start, buffer.column_end);
    setAddr(OLED_PAGE_ADDR, buffer.page_start, buffer.page_end);

    uint8_t x = buffer.column_start, y = buffer.page_start;

    I2C_Start();
    WriteByte(0x40); //连续性写入数据
    for (y=buffer.page_start; y <= buffer.page_end; y++)
        for (x=buffer.column_start; x <= buffer.column_end; x++)
            WriteByte(buffer.GRAM[x + y * OLED_WIDTH]);    
    I2C_Stop();
    
    buffer.column_start = 127;
    buffer.column_end = 0;
    buffer.page_start = 7;
    buffer.page_end = 0;
}
void I2C_OLED::Refresh(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end)
{
    uint8_t page_start = y_start/8, page_end = y_end/8;
    setAddr(OLED_COLUMN_ADDR, x_start, x_end);
    setAddr(OLED_PAGE_ADDR, page_start, page_end);

    uint8_t x = x_start, y = page_start;

    I2C_Start();
    WriteByte(0x40); //连续性写入数据
    for (y = page_start; y <= page_end; y++)
        for (x = x_start; x <= x_end; x++)
            WriteByte(buffer.GRAM[x + y * OLED_WIDTH]);
    I2C_Stop();
    
    buffer.column_start = 127;
    buffer.column_end = 0;
    buffer.page_start = 7;
    buffer.page_end = 0;
}
void I2C_OLED::clear(bool update)
{
    uint16_t i = 1024;
    for (uint8_t *t = buffer.GRAM; i > 0; i--, t++)
        *t = 0x00;
    buffer.column_start = 0;
    buffer.column_end = 127;
    buffer.page_start = 0;
    buffer.page_end = 7;
    if(update) Refresh();
}



/******  Basic Hardware Communication Function      ******/
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
void I2C_OLED::setBuffer(uint8_t *pBuffer) { buffer.GRAM = pBuffer; }
void I2C_OLED::resetBuffer() { buffer.GRAM = GRAM_bk; }
/*********************************************************/



/******      Screen Operation Function      ******/
void I2C_OLED::turnOnScreen() { sendCommand(OLED_DISPLAY_ON); }
void I2C_OLED::turnOffScreen() { sendCommand(OLED_DISPLAY_OFF); }
void I2C_OLED::invert_mode() { sendCommand(OLED_INVERT_DISPLAY); }
void I2C_OLED::normal_mode() { sendCommand(OLED_NORMAL_DISPLAY); }
void I2C_OLED::setContrast(uint8_t contrast)
{
    sendCommand(OLED_SET_CONTRAST);
    sendCommand(contrast);
}
void I2C_OLED::setScreen_UpsideDown()
{
    sendCommand(OLED_SEG_REMAP_A0);
    sendCommand(OLED_COM_SCAN_C0);
    Refresh(0, 127, 0, 63);
}
void I2C_OLED::resetScreen_Orientation()
{
    sendCommand(OLED_SEG_REMAP_A1);
    sendCommand(OLED_COM_SCAN_C8);
    Refresh(0, 127, 0, 63);
}
/************************************************/



/******      Basic Drawing Function      ******/
void I2C_OLED::setColoredPixel(uint8_t x, uint8_t y)
{
    if (x < OLED_WIDTH &&  y < OLED_HEIGHT)
        buffer.GRAM[x + (y >> 3) * OLED_WIDTH] |= (1 << (y & 0x07));
}
void I2C_OLED::setUncoloredPixel(uint8_t x, uint8_t y)
{
    if (x < OLED_WIDTH &&  y < OLED_HEIGHT)
        buffer.GRAM[x + (y >> 3) * OLED_WIDTH] &= ~(1 << (y & 0x07));
}
void I2C_OLED::setInversePixel(uint8_t x, uint8_t y)
{
    if (x < OLED_WIDTH &&  y < OLED_HEIGHT)
        buffer.GRAM[x + (y >> 3) * OLED_WIDTH] ^= (1 << (y & 0x07));
}

void I2C_OLED::drawHorizontalLine(int16_t x, int16_t y, uint8_t length, bool update, drawMode mode)
{
    if (x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    if (x < 0) { length += x; x = 0; }
    if ((x + length) > OLED_WIDTH) length = (OLED_WIDTH - x);
    if (length <= 0) return;

    uint8_t tempy = y >> 3;
    uint8_t tempx = x + length - 1;
    uint8_t *bufferPtr = buffer.GRAM;
    bufferPtr += (x + tempy * OLED_WIDTH);
    uint8_t drawBit = 1 << (y & 7);
   
    switch (mode) {
        case CLEAR:
            drawBit = ~drawBit;
            while (length--) { *bufferPtr++ &= drawBit; }
            break;
        case INVERSE:
            while (length--) { *bufferPtr++ ^= drawBit; }
            break;
        case NORMAL:
        default:
            while (length--) { *bufferPtr++ |= drawBit; }
            break;
    }

    if (update)
    {
        buffer.column_start = MIN(buffer.column_start, x);
        buffer.column_end = MAX(buffer.column_end, tempx);
        buffer.page_start = MIN(buffer.page_start, tempy);
        buffer.page_end = MAX(buffer.page_end, tempy);
        Refresh();
    }
}

void I2C_OLED::drawVerticalLine(int16_t x, int16_t y, uint8_t length, bool update, drawMode mode)
{
    if (x < 0 || x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (y < 0) { length += y; y = 0; }
    if ((y + length) > OLED_HEIGHT) { length = (OLED_HEIGHT - y); }
    if (length <= 0) return;

    uint8_t tempy = (y + length - 1) >> 3;
    uint8_t yOffset = y & 7;
    uint8_t drawBit;
    uint8_t *bufferPtr = buffer.GRAM;
    bufferPtr += (x + (y >> 3) * OLED_WIDTH);

    switch(mode) 
    {
    case CLEAR:
        if (yOffset) {
            yOffset = 8 - yOffset;
            drawBit = ~(0xFF >> (yOffset));
            if (length < yOffset) drawBit &= (0xFF >> (yOffset - length));
            *bufferPtr &= ~drawBit;
            if (length < yOffset) return;
            length -= yOffset;
            bufferPtr += OLED_WIDTH;
        }
        if (length >= 8) {
            drawBit = 0x00;
            do { *bufferPtr = drawBit; bufferPtr += OLED_WIDTH; length -= 8;
            } while (length >= 8);
        }
        if (length > 0) { drawBit = (1 << (length & 7)) - 1; *bufferPtr &= ~drawBit; }
        break;

    case INVERSE:
        if (yOffset) {
            yOffset = 8 - yOffset;
            drawBit = ~(0xFF >> (yOffset));
            if (length < yOffset) { drawBit &= (0xFF >> (yOffset - length)); }
            *bufferPtr ^= drawBit;
            if (length < yOffset) return;
            length -= yOffset;
            bufferPtr += OLED_WIDTH;
        }
        if (length >= 8) {
            do { *bufferPtr = ~(*bufferPtr); bufferPtr += OLED_WIDTH; length -= 8;
            } while (length >= 8);
        }
        if (length > 0) { drawBit = (1 << (length & 7)) - 1; *bufferPtr ^= drawBit; }
        break;

    case NORMAL:
    default:
        if (yOffset){
            yOffset = 8 - yOffset;
            drawBit = ~(0xFF >> (yOffset));
            if (length < yOffset) drawBit &= (0xFF >> (yOffset - length));
            *bufferPtr |= drawBit;
            if (length < yOffset) return;
            length -= yOffset;
            bufferPtr += OLED_WIDTH;
        }
        if (length >= 8) {
            drawBit =  0xFF;
            do { *bufferPtr = drawBit; bufferPtr += OLED_WIDTH; length -= 8;
            } while (length >= 8);
        }
        if (length > 0) { drawBit = (1 << (length & 7)) - 1; *bufferPtr |= drawBit; }
        break;
    }

    if (update)
    {
        buffer.column_start = MIN(buffer.column_start, x);
        buffer.column_end = MAX(buffer.column_end, x);
        buffer.page_start = MIN(buffer.page_start, y >> 3);
        buffer.page_end = MAX(buffer.page_end, tempy);
        Refresh();
    }
}

void I2C_OLED::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool update)
{
    if((x0|x1) >= OLED_WIDTH || (y0|y1) >= OLED_HEIGHT) return;
    
    buffer.column_start = MIN(buffer.column_start, MIN(x0, x1));
    buffer.column_end = MAX(buffer.column_end, MAX(x0, x1));
    buffer.page_start = MIN(buffer.page_start, (MIN(y0, y1)) >> 3);
    buffer.page_end = MAX(buffer.page_end, (MAX(y0, y1)) >> 3);

    uint8_t dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int8_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int8_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;

    while ((x0 != x1) && (y0 != y1))
    {
        setColoredPixel(x0, y0);
        if (2 * err >= dy) { err += dy; x0 += sx; }
        if (2 * err <= dx) { err += dx; y0 += sy; }
    }

    if(update) Refresh();
}

void I2C_OLED::drawRect(int16_t x, int16_t y, uint8_t width, uint8_t height, bool update)
{
    int16_t x_end = x + width - 1, y_end = y + height - 1;
    if (x_end < 0 || x > OLED_WIDTH || y_end < 0 || y > OLED_HEIGHT) return;

    drawHorizontalLine(x, y, width, 0);
    drawVerticalLine(x, y, height, 0);
    drawVerticalLine(x_end, y, height, 0);
    drawHorizontalLine(x, y_end, width, 0);

    buffer.column_start = MIN(buffer.column_start, MAX(x, 0));
    buffer.column_end = MAX(buffer.column_end, MIN(x_end, 127));
    buffer.page_start = MIN(buffer.page_start, (MAX(y, 0)) >> 3);
    buffer.page_end = MAX(buffer.page_end, (MIN(y_end, 63)) >> 3);
    if(update) Refresh();
}

void I2C_OLED::drawFilledRect(int16_t x, int16_t y, uint8_t width, uint8_t height, drawMode mode, bool update)
{
    int16_t x_end = x + width - 1, y_end = y + height - 1;
    if (x_end < 0 || x > OLED_WIDTH || y_end < 0 || y > OLED_HEIGHT) return;
    
    for (int16_t tempx = x; tempx < x + width; tempx++)
        drawVerticalLine(tempx, y, height, 0, mode);

    buffer.column_start = MIN(buffer.column_start, MAX(x, 0));
    buffer.column_end = MAX(buffer.column_end, MIN(x_end, 127));
    buffer.page_start = MIN(buffer.page_start, (MAX(y, 0)) >> 3);
    buffer.page_end = MAX(buffer.page_end, (MIN(y_end, 63)) >> 3);
    if(update) Refresh();
}

void I2C_OLED::drawCircle(int16_t x0, int16_t y0, uint8_t radius, bool update)
{
    int16_t x_start = x0 - radius, x_end = x0 + radius, y_start = y0 - radius, y_end = y0 + radius;
    if (x_end < 0 || x_start > OLED_WIDTH || y_end < 0 || y_start > OLED_HEIGHT) return;

    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do{
        setColoredPixel(x0 - x_pos, y0 + y_pos);
        setColoredPixel(x0 + x_pos, y0 + y_pos);
        setColoredPixel(x0 + x_pos, y0 - y_pos);
        setColoredPixel(x0 - x_pos, y0 - y_pos);
        e2 = err;
        if (e2 <= y_pos)
        {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) e2 = 0;
        }
        if (e2 > x_pos) err += ++x_pos * 2 + 1;
    } while (x_pos <= 0);

    buffer.column_start = MIN(buffer.column_start, MAX(x_start, 0));
    buffer.column_end = MAX(buffer.column_end, MIN(x_end, 127));
    buffer.page_start = MIN(buffer.page_start, (MAX(y_start, 0)) >> 3);
    buffer.page_end = MAX(buffer.page_end, (MIN(y_end, 63)) >> 3);
    if(update) Refresh();
}

void I2C_OLED::drawFilledCircle(int16_t x0, int16_t y0, uint8_t radius, drawMode mode, bool update)
{
    int16_t x_start = x0 - radius, x_end = x0 + radius, y_start = y0 - radius, y_end = y0 + radius;
    if (x_end < 0 || x_start > OLED_WIDTH || y_end < 0 || y_start > OLED_HEIGHT) return;

    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    drawHorizontalLine(x0 + x_pos, y0 + y_pos, 2 * (-x_pos) + 1, 0, mode);
    do {
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            
            drawHorizontalLine(x0 + x_pos, y0 + y_pos, 2 * (-x_pos) + 1, 0, mode);
            drawHorizontalLine(x0 + x_pos, y0 - y_pos, 2 * (-x_pos) + 1, 0, mode);
            
            if (-x_pos == y_pos && e2 <= x_pos) e2 = 0;
        }
        if (e2 > x_pos) err += ++x_pos * 2 + 1;
    } while (x_pos <= 0);

    buffer.column_start = MIN(buffer.column_start, MAX(x_start, 0));
    buffer.column_end = MAX(buffer.column_end, MIN(x_end, 127));
    buffer.page_start = MIN(buffer.page_start, (MAX(y_start, 0)) >> 3);
    buffer.page_end = MAX(buffer.page_end, (MIN(y_end, 63)) >> 3);
    if (update) Refresh();
}

void I2C_OLED::setFont(const uint8_t *fontData)
{
    this -> fontData = fontData;
}

uint16_t I2C_OLED::getStringWidth(const char *text, uint16_t length)
{
    uint16_t firstChar = *(fontData + 2);
    uint16_t stringWidth = 0;

    while (length--)
        stringWidth += *(fontData + 4 + (text[length] - firstChar) * 4 + 3);

    return stringWidth;
}

void inline I2C_OLED::drawInternal(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *data, uint16_t offset, uint16_t sizeOfChar)
{
    if (y + height < 0 || y > OLED_HEIGHT) return;
    if (x + width < 0 || x > OLED_WIDTH) return;

    uint8_t rasterHeight = 1 + ((height - 1) >> 3); // fast ceil(height / 8.0)
    int8_t yOffset = y & 7;

    sizeOfChar = sizeOfChar == 0 ? width * rasterHeight : sizeOfChar;

    int16_t initY = y;
    int8_t initYOffset = yOffset;

    for (uint16_t i = 0; i < sizeOfChar; i++)
    {
        // Reset if next horizontal drawing phase is started.
        if (i % rasterHeight == 0)
        {
            y = initY;
            yOffset = initYOffset;
        }

        uint8_t currentByte = *(data + offset + i);

        int16_t xPos = x + (i / rasterHeight);
        int16_t yPos = ((y >> 3) + (i % rasterHeight)) * OLED_WIDTH;

        //    int16_t yScreenPos = yMove + yOffset;
        int16_t dataPos = xPos + yPos;

        if (dataPos >= 0 && dataPos < 1024 && xPos >= 0 && xPos < OLED_WIDTH)
        {
            if (yOffset >= 0)
            {
                buffer.GRAM[dataPos] |= currentByte << yOffset;

                if (dataPos < (1024 - OLED_WIDTH))
                    buffer.GRAM[dataPos + OLED_WIDTH] |= currentByte >> (8 - yOffset);
            }
            else
            {
                // Make new offset position
                yOffset = -yOffset;
                buffer.GRAM[dataPos] |= currentByte >> yOffset;
                // Prepare for next iteration by moving one block up
                y -= 8;
                // and setting the new yOffset
                yOffset = 8 - yOffset;
            }
        }
    }
}

void I2C_OLED::drawStringInternal(int16_t x, int16_t y, const char *text, uint8_t textLength, uint8_t textWidth)
{
    uint8_t textHeight = *(fontData + 1);
    uint8_t firstChar = *(fontData + 2);
    uint16_t sizeOfJumpTable = *(fontData + 3) * 4;

    uint8_t cursorX = 0;
    uint8_t cursorY = 0;

    // Don't draw anything if it is not on the screen.
    if (x + textWidth < 0 || x > OLED_WIDTH) return;
    if (y + textHeight < 0 || y > OLED_HEIGHT) return;

    for (uint8_t j = 0; j < textLength; j++)
    {
        int16_t xPos = x + cursorX;
        int16_t yPos = y + cursorY;

        uint8_t code = text[j];
        uint8_t charCode = code - firstChar;

        // 4 Bytes per char code
        uint8_t msbJumpToChar = *(fontData + 4 + charCode * 4);                      // MSB  \ JumpAddress
        uint8_t lsbJumpToChar = *(fontData + 4 + charCode * 4 + 1);      // LSB /
        uint8_t charByteSize = *(fontData + 4 + charCode * 4 + 2);      // Size
        uint8_t currentCharWidth = *(fontData + 4 + charCode * 4 + 3); // Width

        // Test if the char is drawable
        if (!(msbJumpToChar == 255 && lsbJumpToChar == 255))
        {
            // Get the position of the char data
            uint16_t charDataPosition = 4 + sizeOfJumpTable + ((msbJumpToChar << 8) + lsbJumpToChar);
            drawInternal(xPos, yPos, currentCharWidth, textHeight, fontData, charDataPosition, charByteSize);
        }

        cursorX += currentCharWidth;

    }
}

void I2C_OLED::drawString(int16_t x, int16_t y, char usrStr[], bool update)
{
    uint8_t lineHeight = *(fontData + 1);
    uint16_t line = 0;
    uint16_t length = 0;
    uint16_t width = 0;
    uint16_t maxwidth = 0;

    buffer.column_start = MIN(buffer.column_start, MAX(x, 0));
    buffer.page_start = MIN(buffer.page_start, (MAX(y, 0)) >> 3);

    char *textPart = strtok(usrStr, "\n");
    while (textPart != NULL)
    {
        length = strlen(textPart);
        width = getStringWidth(textPart, length);
        maxwidth = MAX(width, maxwidth);
        
        drawStringInternal(x, y + (line++) * lineHeight, textPart, length, width);
        textPart = strtok(NULL, "\n");
    }

    buffer.column_end = MAX(buffer.column_end, MIN(x+maxwidth, 127));
    buffer.page_end = MAX(buffer.page_end, (MIN((y + line * lineHeight), 63)) >> 3);
    if(update)  Refresh();
}

void I2C_OLED::drawFastImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *image, bool update)
{
    drawInternal(x, y, width, height, image, 0, 0);

    buffer.column_start = MIN(buffer.column_start, MAX(x, 0));
    buffer.column_end = MAX(buffer.column_end, MIN(x + width, 127));
    buffer.page_start = MIN(buffer.page_start, (MAX(y, 0)) >> 3);
    buffer.page_end = MAX(buffer.page_end, (MIN((y + height), 63)) >> 3);
    if (update) Refresh();
}
