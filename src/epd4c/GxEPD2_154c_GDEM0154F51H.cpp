// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: https://www.good-display.com/
// Panel: GDEM0154F51H : 1.54inch 200x200px 4-color e-paper display
// Controller: Custom controller for 4-color display
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_154c_GDEM0154F51H.h"

GxEPD2_154c_GDEM0154F51H::GxEPD2_154c_GDEM0154F51H(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 4000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _paged = false;
}

void GxEPD2_154c_GDEM0154F51H::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_154c_GDEM0154F51H::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh();
}

void GxEPD2_154c_GDEM0154F51H::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_154c_GDEM0154F51H::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  if (!_init_display_done) _InitDisplay();
  _writeCommand(0x10);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 4; i++)
  {
    _transfer(0xFF == black_value ? 0x55 : 0x00);
  }
  _endTransfer();
  _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_154c_GDEM0154F51H::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (hasPartialUpdate)
  {
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 8; j++)
      {
        uint8_t data = 0xFF;
        // use wb, h of bitmap for index!
        uint32_t idx = mirror_y ? j + dx / 8 + uint32_t((h - 1 - (i + dy))) * wb : j + dx / 8 + uint32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  else
  {
    // full update without partial update support
    _writeCommand(0x10);
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 4; i++)
    {
      _transfer(0x55); // all white
    }
    _endTransfer();
  }
}

void GxEPD2_154c_GDEM0154F51H::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                               int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data = 0xFF;
      // use wb_bitmap, h_bitmap of bitmap for index!
      int16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + uint32_t(y_part + i + dy) * wb_bitmap;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                        (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
        data <<= 4;
        _transfer(data2);
      }
    }
  }
  _endTransfer();
}

void GxEPD2_154c_GDEM0154F51H::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154c_GDEM0154F51H::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                               int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154c_GDEM0154F51H::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
    if (!_init_display_done) _InitDisplay();
    if (_initial_write) writeScreenBuffer();
    int16_t wb = (w + 3) / 4; // width bytes 4 pixels per byte
    x -= x % 4; // 4 pixel boundary
    w = wb * 4; // 4 pixel boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 4; j++)
      {
        uint32_t idx = mirror_y ? j + dx / 4 + uint32_t((h - 1 - (i + dy))) * wb : j + dx / 4 + uint32_t(i + dy) * wb;
        uint8_t data = 0xFF;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        if (invert) data = ~data;
        _transfer(data);
      }
    }
    _endTransfer();
  }
}

void GxEPD2_154c_GDEM0154F51H::writeNativePart(const uint8_t* data1, const uint8_t* data2, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                                int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
    if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
    if ((x_part < 0) || (x_part >= w_bitmap)) return;
    if ((y_part < 0) || (y_part >= h_bitmap)) return;
    int16_t wb_bitmap = (w_bitmap + 3) / 4; // width bytes 4 pixels per byte
    x_part -= x_part % 4; // 4 pixel boundary
    w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
    h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
    x -= x % 4; // 4 pixel boundary
    w = 4 * ((w + 3) / 4); // 4 pixel boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    if (!_init_display_done) _InitDisplay();
    if (_initial_write) writeScreenBuffer();
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 4; j++)
      {
        uint32_t idx = mirror_y ? x_part / 4 + j + dx / 4 + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 4 + j + dx / 4 + uint32_t(y_part + i + dy) * wb_bitmap;
        uint8_t data = 0xFF;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        if (invert) data = ~data;
        _transfer(data);
      }
    }
    _endTransfer();
  }
}

void GxEPD2_154c_GDEM0154F51H::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_154c_GDEM0154F51H::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                              int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_154c_GDEM0154F51H::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_154c_GDEM0154F51H::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                              int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_154c_GDEM0154F51H::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_154c_GDEM0154F51H::refresh(bool partial_update_mode)
{
  _refresh(partial_update_mode);
}

void GxEPD2_154c_GDEM0154F51H::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (hasPartialUpdate)
  {
    _setPartialRamArea(x, y, w, h);
  }
  _refresh(false);
}

void GxEPD2_154c_GDEM0154F51H::_refresh(bool partial_update_mode)
{
  if (!partial_update_mode)
  {
    _writeCommand(0x12); // display refresh
    _writeData(0x00);
    _waitWhileBusy("_refresh", full_refresh_time);
  }
}

void GxEPD2_154c_GDEM0154F51H::powerOff()
{
  _PowerOff();
}

void GxEPD2_154c_GDEM0154F51H::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);
  }
}

void GxEPD2_154c_GDEM0154F51H::setPaged()
{
  _paged = true;
}

void GxEPD2_154c_GDEM0154F51H::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial_mode)
{
  w = (w + 7) & 0xFFF8; // byte boundary exclusive (round up)
  h = (h + 7) & 0xFFF8; // byte boundary exclusive (round up)
  if (partial_mode) // for partial mode always use full area
  {
    x = 0; y = 0; w = WIDTH; h = HEIGHT;
  }
  x = gx_uint16_min(x, WIDTH);
  y = gx_uint16_min(y, HEIGHT);
  w = gx_uint16_min(w, WIDTH - x);
  h = gx_uint16_min(h, HEIGHT - y);
}

void GxEPD2_154c_GDEM0154F51H::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04); // power on
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_154c_GDEM0154F51H::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02); // power off
    _writeData(0x00);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
}

void GxEPD2_154c_GDEM0154F51H::_InitDisplay()
{
  if (_hibernating) _reset();
  _writeCommand(0x4D); // FET Control
  _writeData(0x78);
  _writeCommand(0x00); // Panel Setting
  _writeData(0x0F); // 4-color mode
  _writeData(0x29);
  _writeCommand(0x06); // Booster Setting
  _writeData(0x0D);
  _writeData(0x12);
  _writeData(0x30);
  _writeData(0x20);
  _writeData(0x19);
  _writeData(0x2A);
  _writeData(0x22);
  _writeCommand(0x50); // VCOM and data interval setting
  _writeData(0x37);
  _writeCommand(0x61); // Resolution setting
  _writeData(WIDTH / 256);
  _writeData(WIDTH % 256);
  _writeData(HEIGHT / 256);
  _writeData(HEIGHT % 256);
  _writeCommand(0xE9); // Force temperature
  _writeData(0x01);
  _writeCommand(0x30); // PLL setting
  _writeData(0x08);
  _PowerOn();
  _waitWhileBusy("_InitDisplay", power_on_time);
  _init_display_done = true;
}