# OLED(SSD1306) Library

> test passed with 0.96' oled on esp32

## Source File

- [header file](https://github.com/lebinlv/ESP32/blob/master/components/oled/include/OLED.h)
- [source file](https://github.com/lebinlv/ESP32/blob/master/components/oled/OLED.cpp)

## Basic

```c++
/**
 * @brief: drawMode Enumeration type:
 *        NORMAL: draw the element in normal mode;
 *         CLEAR: use the element to clear a area;
 *       INVERSE: use the element to inverse a area;
 * */
enum drawMode { NORMAL, CLEAR, INVERSE };

/**
 * @brief default constructor function, \
 *        initlized GrAM(display buffer), \
 *                  printfStruct(which specified the screen area that printf function used)
 *                  and some varible about font.
 * */
I2C_OLED();

~I2C_OLED();

// hardware init function.
void Init(gpio_num_t SCL_pinNum, gpio_num_t SDA_pinNum, gpio_num_t RST_pinNum, uint8_t address = 0x78);

/**
 * @brief clear screen;
 * @param [update]: true(default): clear GRAM and refresh screen right now;
 *                  false: clear GRAM only.
 * */
void clear(bool update = true);
void clear(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end, bool update = true);

//default screen refresh function. 
void Refresh();

/**
 * @brief  overloaded refresh function, 
 *         refresh the specified area from (x_start, y_start) to (x_end, y_end);
 * @param   x_start: >=0 and <=127;
 * @param     x_end: >=x_start and <=127;
 * @param   y_start: >=0 and <=63;
 * @param     y_end: >= y_start and <=63;
 **/
void Refresh(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);

// you can use this function to change GRAM
void setBuffer(uint8_t * pBuffer);

// reset the GRAM to default
void resetBuffer();
```


## Basic Display Control Function

```c++
/******      Basic Display Control Function      ******/
void turnOnScreen();    // turn on the screen;
void turnOffScreen();   // turn off the screen, just like "power save mode";
void invert_mode();     // set the OLED to invert display mode: bit 0 in GRAM indicates an "ON" pixel;
void normal_mode();     // (default mode)set the OLED to normal display mode: bit 1 in GRAM indivates an "ON" pixel;
void setContrast(uint8_t contrast); // adjust the contrast, 0<=contrast<=0xff;
void setScreen_UpsideDown();        // Turn the display upside down
void resetScreen_Orientation();     // (default mode) take the position of pins as screen's top
```

## Basic Drawing Function

```c++
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
void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool update = true);

/**
 * @brief: Draw a horizontal line.
 * @param:    x,y: start point (x,y);
 * @param: length: length of the line;
 * @param: update: it must be "true" when you use it to draw a HorizontalLine;
 * @param:   mode: it can be NORMAL, CLEAR, INVERSE;
 * */
void drawHorizontalLine(int16_t x, int16_t y, uint8_t length, bool update = true, drawMode mode = NORMAL);

// Draw a vertical line.
void drawVerticalLine(int16_t x, int16_t y, uint8_t length, bool update = true, drawMode mode = NORMAL);

// Draw the border of a rectangle at the given location
void drawRect(int16_t x, int16_t y, uint8_t width, uint8_t height, bool update = true);

/**
 * @brief: draw a filled rectangle or use this rectangle to realise clear or inverse a specified area.
 * @param    mode: it can be NORMAL, CLEAR, INVERSE:
 *                 NORMAL: normal display mode, draw a filled rectangle;
 *                  CLEAR: use this rectangle to clear a area;
 *                INVERSE: use this rectangle to inverse a area;
 * */
void drawFilledRect(int16_t x, int16_t y, uint8_t width, uint8_t height, drawMode mode = NORMAL, bool update = true);

// Draw the border of a circle
void drawCircle(int16_t x0, int16_t y0, uint8_t radius, bool update = true);

// draw a filled circle or use this circle to realise clear or inverse a specified area.
void drawFilledCircle(int16_t x0, int16_t y0, uint8_t radius, drawMode mode = NORMAL, bool update = true);
```

## Basic text operations

```c++
/******       Basic test operations       ******/
void setFont(const uint8_t *fontData);

/**
    * @brief  draw srting from (x, y)
    * @attention  this function can not auto wrap, but you can use '\n' to wrap the line.
    * */ 
void drawString(uint8_t x, uint8_t y, const char *usrStr, bool update = true);

/**
    * @brief set the screen area that printf function used.
    * */
void setPrintfArea(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);

// this function realizd basic "printf", you can "%d", "%f", "%c", "%s"
void printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

// clear printf area and reset cursor
void printfClear();
```

## Draw image

```c++
/******      Draw image      ******/
void drawImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *image, bool update = true);
```
