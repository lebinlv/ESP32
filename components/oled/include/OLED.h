#ifndef OLED_H_
#define OLED_H_

#include "driver/gpio.h"
#include "fonts/DejaVu_Sans_10.h"


// Display commands
#define OLED_CHARGE_PUMP            0x8D
#define OLED_COLUMN_ADDR            0x21
#define OLED_COM_SCAN_C0            0xC0
#define OLED_COM_SCAN_C8            0xC8
#define OLED_DISPLAY_ALL_ON         0xA5
#define OLED_DISPLAY_ALLON_RESUME   0xA4
#define OLED_DISPLAY_OFF            0xAE
#define OLED_DISPLAY_ON             0xAF
//#define OLED_EXTERNAL_VCC           0x01
#define OLED_INVERT_DISPLAY         0xA7
#define OLED_MEMORY_MODE            0x20
#define OLED_NORMAL_DISPLAY         0xA6
#define OLED_PAGE_ADDR              0x22
#define OLED_SEG_REMAP_A0           0xA0
#define OLED_SEG_REMAP_A1           0xA1
#define OLED_SET_COMPINS            0xDA
#define OLED_SET_CONTRAST           0x81
#define OLED_SET_DCLK_CLK           0xD5
#define OLED_SET_DISPLAY_OFFSET     0xD3
//#define OLED_SET_HIGH_COLUMN        0x10
//#define OLED_SET_LOW_COLUMN         0x00
#define OLED_SET_MUX_Ratio          0xA8
#define OLED_SET_PRE_CHARGE         0xD9
#define OLED_SET_START_LINE         0x40
#define OLED_SET_VCOMH_DESELECT     0xDB
//#define OLED_SWITCH_CAP_VCC         0x02


struct Display_Buffer_t
{
    uint8_t *GRAM;         // pointer, point to display buffer
    uint8_t column_start;  // the start column of modified data, 0<=column_start<128
    uint8_t column_end;    // the end column of modified data, column_start<=column_end<128
    uint8_t row_start;    // the start page of modified data, 0<=page_start<8
    uint8_t row_end;      // the end page of modified data, page_start<=page_end<8
};

struct OLED_printf_t
{
    uint8_t x_start;
    uint8_t x_end;
    uint8_t y_start;
    uint8_t y_end;
    uint8_t x_cursor;
    uint8_t y_cursor;
};

class OLED
{
  protected:
    static const uint8_t OLED_WIDTH = 128;              // screen width
    static const uint8_t OLED_HEIGHT = 64;              // screen height

    uint8_t device_address; // device address, 0x78 in default;

    Display_Buffer_t buffer;
    uint8_t *GRAM_bk;
    OLED_printf_t printfStruct;

    // varibles about font
    const uint8_t *fontData;
    uint8_t textHeight;
    uint8_t firstChar;
    uint16_t charNum;

    virtual void sendCommand(uint8_t command) = 0;

    void screenInit();

    void drawChar(uint8_t &x, uint8_t &y, char charToDraw, bool printfMode = false);

  public:

    /**
     * @brief: drawMode Enumeration type:
     *        NORMAL: draw the element in normal mode;
     *         CLEAR: use the element to clear a area;
     *       INVERSE: use the element to inverse a area;
     * */
    enum drawMode { NORMAL=0, CLEAR, INVERSE };
    
    /**
     * @brief default constructor function, \
     *        initlized GrAM(display buffer), \
     *                  printfStruct(which specified the screen area that printf function used)
     *                  and some varible about font.
     * */
    OLED();

    virtual ~OLED();

    /**
     * @brief clear screen;
     * @param [update]: true(default): clear GRAM and refresh screen right now;
     *                  false: clear GRAM only.
     * */
    void clear();
    void clear(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);

    //default screen refresh function. 
    virtual void Refresh() = 0;

    // you can use this function to change GRAM
    void setBuffer(uint8_t * pBuffer);

    // reset the GRAM to default
    void resetBuffer();


    /******      Basic Display Control Function      ******/
    void turnOnScreen();    // turn on the screen;
    void turnOffScreen();   // turn off the screen, just like "power save mode";
    void invert_mode();     // set the OLED to invert display mode: bit 0 in GRAM indicates an "ON" pixel;
    void normal_mode();     // (default mode)set the OLED to normal display mode: bit 1 in GRAM indivates an "ON" pixel;
    void setContrast(uint8_t contrast); // adjust the contrast, 0<=contrast<=0xff;
    void setScreen_UpsideDown();        // Turn the display upside down
    void resetScreen_Orientation();     // (default mode) take the position of pins as screen's top


    /******      Basic Drawing Function      ******/
    // Draw a pixel. just changes the data stored in GRAM, you should refresh the screen manually.
    void setColoredPixel(uint8_t x, uint8_t y);     // turn on a pixel;
    void setUncoloredPixel(uint8_t x, uint8_t y);   // turn off a pixal;
    void setInversePixel(uint8_t x, uint8_t y);     // inverse a pixel;

    /**
     * @brief: Draw a line from position(x0,y0) to position (x1,y1);
     * @param: update: 
     *              true(default): draw a line in GRAM and refresh screeen;
     *                      false: draw a line in GRAM and change modified area information that stored in buffer structure,
     *                        you can use refresh() function to refrsh the screen manually.
     * */
    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

    /**
     * @brief: Draw a horizontal line.
     * @param:    x,y: start point (x,y);
     * @param: length: length of the line;
     * @param: update: it must be "true" when you use it to draw a HorizontalLine;
     * @param:   mode: it can be NORMAL, CLEAR, INVERSE;
     * */
    void drawHorizontalLine(int16_t x, int16_t y, uint8_t length, drawMode mode = NORMAL, bool update = true);

    // Draw a vertical line.
    void drawVerticalLine(int16_t x, int16_t y, uint8_t length, drawMode mode = NORMAL, bool update = true);

    // Draw the border of a rectangle at the given location
    void drawRect(int16_t x, int16_t y, uint8_t width, uint8_t height);

    /**
     * @brief: draw a filled rectangle or use this rectangle to realise clear or inverse a specified area.
     * @param    mode: it can be NORMAL, CLEAR, INVERSE:
     *                 NORMAL: normal display mode, draw a filled rectangle;
     *                  CLEAR: use this rectangle to clear a area;
     *                INVERSE: use this rectangle to inverse a area;
     * */
    void drawFilledRect(int16_t x, int16_t y, uint8_t width, uint8_t height, drawMode mode = NORMAL);
    
    // Draw the border of a circle
    void drawCircle(int16_t x0, int16_t y0, uint8_t radius);

    // draw a filled circle or use this circle to realise clear or inverse a specified area.
    void drawFilledCircle(int16_t x0, int16_t y0, uint8_t radius, drawMode mode = NORMAL);


    /******       Basic test operations       ******/
    void setFont(const uint8_t *fontData);

    /**
     * @brief  draw srting from (x, y)
     * @attention  this function can not auto wrap, but you can use '\n' to wrap the line.
     * */ 
    void drawString(uint8_t x, uint8_t y, const char *usrStr);
    
    /**
     * @brief set the screen area that printf function used.
     * */
    void setPrintfArea(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);

    // this function realizd basic "printf", you can use "%d", "%f", "%c", "%s"
    void printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

    // clear printfClear and reset cursor
    void printfClear();

    /******      Draw image      ******/
    void drawImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *image);
};

#endif
