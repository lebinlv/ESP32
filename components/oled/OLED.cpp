#include <stdlib.h>
#include <stdarg.h>
#include "OLED.h"

OLED::OLED()
{
    buffer.GRAM = new uint8_t [1024];  // 1024 = OLED_WIDTH*OLED_HEIGHT/8
    buffer.column_start = 0;
    buffer.column_end = OLED_WIDTH - 1;
    buffer.row_start = 0;
    buffer.row_end = OLED_HEIGHT - 1;

    printfStruct.x_start = 0;
    printfStruct.x_end = OLED_WIDTH;
    printfStruct.y_start = 0;
    printfStruct.y_end = OLED_HEIGHT;
    printfStruct.x_cursor = 0;
    printfStruct.y_cursor = 0;
    
    GRAM_bk = buffer.GRAM;
    fontData = DejaVu_Sans_10;
    textHeight = *(fontData + 1);
    firstChar = *(fontData + 2);
    charNum = *(fontData + 3);
}

OLED::~OLED()
{
    buffer.GRAM = GRAM_bk;
    delete [] buffer.GRAM;
}

void OLED::screenInit()
{
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
}

void OLED::clear()
{
    uint16_t i = 1024;
    for (uint8_t *t = buffer.GRAM; i > 0; i--, t++)
        *t = 0x00;
    buffer.column_start = 0;
    buffer.column_end = OLED_WIDTH - 1;
    buffer.row_start = 0;
    buffer.row_end = OLED_HEIGHT - 1;
    Refresh();
}

void OLED::clear(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end)
{
    if(x_start>=x_end || y_start>=y_end) return;
    drawFilledRect(x_start, y_start, x_end-x_start, y_end-y_start, CLEAR);
}


void OLED::setBuffer(uint8_t *pBuffer) { buffer.GRAM = pBuffer; }
void OLED::resetBuffer() { buffer.GRAM = GRAM_bk; }
/*********************************************************/



/******      Screen Operation Function      ******/
void OLED::turnOnScreen() { sendCommand(OLED_DISPLAY_ON); }
void OLED::turnOffScreen() { sendCommand(OLED_DISPLAY_OFF); }
void OLED::invert_mode() { sendCommand(OLED_INVERT_DISPLAY); }
void OLED::normal_mode() { sendCommand(OLED_NORMAL_DISPLAY); }
void OLED::setContrast(uint8_t contrast)
{
    sendCommand(OLED_SET_CONTRAST);
    sendCommand(contrast);
}
void OLED::setScreen_UpsideDown()
{
    sendCommand(OLED_SEG_REMAP_A0);
    sendCommand(OLED_COM_SCAN_C0);
    Refresh();
}
void OLED::resetScreen_Orientation()
{
    sendCommand(OLED_SEG_REMAP_A1);
    sendCommand(OLED_COM_SCAN_C8);
    Refresh();
}
/************************************************/



/******      Basic Drawing Function      ******/
void OLED::setColoredPixel(uint8_t x, uint8_t y)
{
    if (x < OLED_WIDTH &&  y < OLED_HEIGHT)
        buffer.GRAM[x + (y >> 3) * OLED_WIDTH] |= (1 << (y & 0x07));
}
void OLED::setUncoloredPixel(uint8_t x, uint8_t y)
{
    if (x < OLED_WIDTH &&  y < OLED_HEIGHT)
        buffer.GRAM[x + (y >> 3) * OLED_WIDTH] &= ~(1 << (y & 0x07));
}
void OLED::setInversePixel(uint8_t x, uint8_t y)
{
    if (x < OLED_WIDTH &&  y < OLED_HEIGHT)
        buffer.GRAM[x + (y >> 3) * OLED_WIDTH] ^= (1 << (y & 0x07));
}

void OLED::drawHorizontalLine(int16_t x, int16_t y, uint8_t length, drawMode mode, bool update)
{
    if (x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    if (x < 0) { length += x; x = 0; }
    if ((x + length) > OLED_WIDTH) length = (OLED_WIDTH - x);
    if (length <= 0) return;

    uint8_t *bufferPtr = buffer.GRAM;
    bufferPtr += (x + (y >> 3) * OLED_WIDTH);
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
        buffer.column_start = x;
        buffer.column_end = x + length - 1;
        buffer.row_start = y;
        buffer.row_end = y;
        Refresh();
    }
}

void OLED::drawVerticalLine(int16_t x, int16_t y, uint8_t length, drawMode mode, bool update)
{
    if (x < 0 || x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (y < 0) { length += y; y = 0; }
    if ((y + length) > OLED_HEIGHT) { length = (OLED_HEIGHT - y); }
    if (length <= 0) return;

    uint8_t y_end = y + length - 1;
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
        buffer.column_start = x;
        buffer.column_end = x;
        buffer.row_start = y;
        buffer.row_end = y_end;
        Refresh();
    }
}

void OLED::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if((x0|x1) >= OLED_WIDTH || (y0|y1) >= OLED_HEIGHT) return;
    
    if(x0 < x1){
        buffer.column_start = x0;
        buffer.column_end = x1;
    }else{
        buffer.column_start = x1;
        buffer.column_end = x0;
    }

    if(y0 < y1){
        buffer.row_start = y0;
        buffer.row_end = y1;
    }else{
        buffer.row_start = y1;
        buffer.row_end = y0;
    }

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

    Refresh();
}

void OLED::drawRect(int16_t x, int16_t y, uint8_t width, uint8_t height)
{
    int16_t x_end = x + width - 1, y_end = y + height - 1;
    if (x_end < 0 || x > OLED_WIDTH || y_end < 0 || y > OLED_HEIGHT) return;

    drawHorizontalLine(x, y, width, NORMAL ,0);
    drawVerticalLine(x, y, height, NORMAL, 0);
    drawVerticalLine(x_end, y, height, NORMAL, 0);
    drawHorizontalLine(x, y_end, width, NORMAL, 0);

    buffer.column_start = x > 0 ? x : 0;
    buffer.column_end = x_end;
    buffer.row_start = y > 0 ? y : 0;
    buffer.row_end = y_end;
    Refresh();
}

void OLED::drawFilledRect(int16_t x, int16_t y, uint8_t width, uint8_t height, drawMode mode)
{
    int16_t x_end = x + width - 1, y_end = y + height - 1;
    if (x_end < 0 || x > OLED_WIDTH || y_end < 0 || y > OLED_HEIGHT) return;
    
    for (int16_t tempx = x; tempx < x + width; tempx++)
        drawVerticalLine(tempx, y, height, mode, 0);

    buffer.column_start = x > 0 ? x : 0;
    buffer.column_end = x_end;
    buffer.row_start = y > 0 ? y : 0;
    buffer.row_end = y_end;
    Refresh();
}

void OLED::drawCircle(int16_t x0, int16_t y0, uint8_t radius)
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

    buffer.column_start = x_start > 0 ? x_start : 0;
    buffer.column_end = x_end;
    buffer.row_start = y_start > 0 ? y_start : 0;
    buffer.row_end = y_end;
    Refresh();
}

void OLED::drawFilledCircle(int16_t x0, int16_t y0, uint8_t radius, drawMode mode)
{
    int16_t x_start = x0 - radius, x_end = x0 + radius, y_start = y0 - radius, y_end = y0 + radius;
    if (x_end < 0 || x_start > OLED_WIDTH || y_end < 0 || y_start > OLED_HEIGHT) return;

    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    drawHorizontalLine(x0 + x_pos, y0 + y_pos, 2 * (-x_pos) + 1, mode, 0);
    do {
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            
            drawHorizontalLine(x0 + x_pos, y0 + y_pos, 2 * (-x_pos) + 1, mode, 0);
            drawHorizontalLine(x0 + x_pos, y0 - y_pos, 2 * (-x_pos) + 1, mode, 0);
            
            if (-x_pos == y_pos && e2 <= x_pos) e2 = 0;
        }
        if (e2 > x_pos) err += ++x_pos * 2 + 1;
    } while (x_pos <= 0);

    buffer.column_start = x_start > 0 ? x_start : 0;
    buffer.column_end = x_end;
    buffer.row_start = y_start > 0 ? y_start : 0;
    buffer.row_end = y_end;
    Refresh();
}

void OLED::setFont(const uint8_t *fontData)
{
    this -> fontData = fontData;
    textHeight = *(fontData + 1);
    firstChar = *(fontData + 2);
    charNum = *(fontData + 3);
}

void OLED::drawChar(uint8_t &x, uint8_t &y, char charToDraw, bool printfMode)
{
static uint16_t idx = 0;
    if (charToDraw < firstChar || charToDraw >= firstChar + charNum) {
        if (charToDraw == '\n' && printfMode) {
            x = printfStruct.x_start;
            y += textHeight;

            // draw a FilledRect in CLEAR mode to clear the new line
            drawFilledRect(printfStruct.x_start, y, printfStruct.x_end - printfStruct.x_start, textHeight, CLEAR);
        }return;
    }

    uint16_t charFontTableBase = 4 + (charToDraw - firstChar) * 4;
    uint8_t msbJumpToChar = *(fontData + charFontTableBase);        // MSB of JumpAddress
    uint8_t lsbJumpToChar = *(fontData + charFontTableBase + 1);    // LSB of JumpAddress
    uint8_t currentCharWidth = *(fontData + charFontTableBase + 3); // Width

    if(msbJumpToChar == 255 && lsbJumpToChar == 255) { x += currentCharWidth; return; }

    uint8_t sizeOfChar = *(fontData + charFontTableBase + 2);       // Size
    uint16_t charDataPosition = 4 + charNum * 4 + ((msbJumpToChar << 8) + lsbJumpToChar);

    uint8_t area_xend = printfStruct.x_end, area_yend = printfStruct.y_end;
    bool lineWraped = false;
    if (printfMode) {
        if ((x + (currentCharWidth >> 1)) > area_xend) {
            x = printfStruct.x_start;
            y += textHeight;
            lineWraped = true;
        }
        if ((y + (textHeight >> 1)) > area_yend) {
            y = printfStruct.y_start;
            lineWraped = true;
        }
        if(lineWraped)
            drawFilledRect(printfStruct.x_start, y, area_xend - printfStruct.x_start, textHeight, CLEAR);
    } else {
        area_xend = OLED_WIDTH;
        area_yend = OLED_HEIGHT;
        if ((x + currentCharWidth/2) > area_xend) {
            x += currentCharWidth;
            return;
        }
        if ((y + textHeight/2) > area_yend) {
            y += textHeight;
            return;
        }
    }

    uint8_t rasterHeight = 1 + ((textHeight - 1) >> 3);
    uint8_t yOffset = y & 7;

    uint8_t currentByte = 0, xPos = 0;
    uint16_t yPos = 0, dataPos = 0;

    for (uint8_t i = 0; i < sizeOfChar; i++)
    {
        currentByte = *(fontData + charDataPosition + i);
        xPos = x + (i / rasterHeight);
        yPos = ((y >> 3) + (i % rasterHeight));
        dataPos = xPos + yPos * OLED_WIDTH;

        if (dataPos < 1024 && xPos < area_xend)
        {
            buffer.GRAM[dataPos] |= currentByte << yOffset;
            if ((yPos * 8 + 8) < area_yend && (dataPos + OLED_WIDTH) < 1024)
                buffer.GRAM[dataPos + OLED_WIDTH] |= currentByte >> (8 - yOffset);
        }
    }
    x += currentCharWidth;
    idx++;
}

void OLED::drawString(uint8_t x, uint8_t y, const char *usrStr)
{
    if(x>=OLED_WIDTH || y>=OLED_HEIGHT) return;
    uint8_t max_x = 0, initX = x;

    buffer.column_start = x;
    buffer.row_start = y;

    while (*usrStr)
    {
        if(*usrStr == '\n') {max_x = max_x > initX ? max_x : initX; initX = x; y += textHeight;}
        else if(initX < OLED_WIDTH && y < OLED_HEIGHT)
            drawChar(initX, y, *usrStr, false);
        usrStr++;
    }

    buffer.column_end = max_x > initX ? max_x : initX;
    buffer.row_end = y+textHeight;

    Refresh();
}

void OLED::printf(const char *format, ...)
{
    char strBuffer[17];
    uint8_t initY = printfStruct.y_cursor;
    va_list ap;
    va_start(ap, format);
    while (*format)
    {
        if (*format != '%'){
            drawChar(printfStruct.x_cursor, printfStruct.y_cursor, *format, true);
            format++;
        }
        else {
            format++;
            switch (*format)
            {
            case 'c': {
                char val_ch = va_arg(ap, int);
                drawChar(printfStruct.x_cursor, printfStruct.y_cursor, val_ch, true);
                format++;
            } break;

            case 'd': {
                int val_int = va_arg(ap, int);
                itoa(val_int, strBuffer, 10);
                char *str = &strBuffer[0];
                while (*str) {
                    drawChar(printfStruct.x_cursor, printfStruct.y_cursor, *str, true);
                    str++;
                }
                format++;
            } break;

            case 's': {
                char *val_str = va_arg(ap, char *);
                while (*val_str) {
                    drawChar(printfStruct.x_cursor, printfStruct.y_cursor, *val_str, true);
                    val_str++;
                }
                format++;
            } break;

            case 'f': {
                float val_flt = va_arg(ap, double);
                gcvt(val_flt, 6, strBuffer);
                char *str = &strBuffer[0];
                while (*str) {
                    drawChar(printfStruct.x_cursor, printfStruct.y_cursor, *str, true);
                    str++;
                }
                format++;
            } break;

            default: {
                drawChar(printfStruct.x_cursor, printfStruct.y_cursor, *format, true);
                format++;
                } break;
            }
        }
    }
    va_end(ap);
    buffer.column_start = printfStruct.x_start;
    buffer.column_end = printfStruct.x_end;

    if(printfStruct.y_cursor > initY){
        buffer.row_start = initY;
        buffer.row_end = printfStruct.y_cursor+textHeight;
    }else{
        buffer.row_start = printfStruct.y_start;
        buffer.row_end = printfStruct.y_end;     
    }
    Refresh();
}

void OLED::printfClear()
{
    printfStruct.x_cursor = printfStruct.x_start;
    printfStruct.y_cursor = printfStruct.y_start;
    // draw a FilledRect in CLEAR mode to clear the new line
    drawFilledRect(printfStruct.x_start, printfStruct.y_start,
                   printfStruct.x_end - printfStruct.x_start,
                   printfStruct.y_end - printfStruct.y_start,
                   CLEAR);
}

void OLED::setPrintfArea(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end)
{
    if(x_start>=x_end || x_end > OLED_WIDTH || y_start>=y_end || y_end >OLED_HEIGHT) return;
    printfStruct.x_start = x_start;
    printfStruct.x_end = x_end;
    printfStruct.y_start = y_start;
    printfStruct.y_end = y_end;
    printfStruct.x_cursor = x_start;
    printfStruct.y_cursor = y_start;
    printfClear();
}

void OLED::drawImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *image)
{
    if (y + height < 0 || y > OLED_HEIGHT) return;
    if (x + width < 0 || x > OLED_WIDTH) return;

    uint8_t rasterHeight = 1 + ((height - 1) >> 3); // fast ceil(height / 8.0)
    uint16_t imageSize = width * rasterHeight;

    int16_t initY = y;
    int8_t yOffset = y & 7;
    int8_t initYOffset = yOffset;

    for (uint16_t i = 0; i < imageSize; i++)
    {
        // Reset if next horizontal drawing phase is started.
        if (i % rasterHeight == 0)
        {
            y = initY;
            yOffset = initYOffset;
        }

        uint8_t currentByte = *(image + i);
        int16_t xPos = x + (i / rasterHeight);
        int16_t yPos = ((y >> 3) + (i % rasterHeight)) * OLED_WIDTH;
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
                yOffset = -yOffset; // Make new offset position
                buffer.GRAM[dataPos] |= currentByte >> yOffset;
                y -= 8; // Prepare for next iteration by moving one block up
                yOffset = 8 - yOffset; // and setting the new yOffset
            }
        }
    }

    buffer.column_start = x;
    buffer.column_end = x + width;
    buffer.row_start = y;
    buffer.row_end = y + height;
    Refresh();
}
