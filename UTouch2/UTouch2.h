/*
  UTouch2.h - Arduino/chipKit library support for Color TFT LCD Touch screens 
  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
  
  Basic functionality of this library are based on the demo-code provided by
  ITead studio.

  You can find the latest version of the library at 
  http://www.RinkyDinkElectronics.com/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/

#ifndef UTouch2_h
#define UTouch2_h

#define UTOUCH2_VERSION	124

// *** Hardwarespecific defines ***

#if defined(__PIC32MX__)
 #include "WProgram.h"

 #define cbi(reg, bitmask) (*(reg + 1)) = bitmask
 #define sbi(reg, bitmask) (*(reg + 2)) = bitmask
 #define rbi(reg, bitmask) (*(reg) & bitmask)

 #define pulse_high(reg, bitmask) sbi(reg, bitmask); cbi(reg, bitmask);
 #define pulse_low(reg, bitmask) cbi(reg, bitmask); sbi(reg, bitmask);

 #define regtype volatile uint32_t
 #define regsize uint16_t

#elif defined(__AVR__) || defined(__arm__)
 #include "Arduino.h"

 #define cbi(reg, bitmask) *reg &= ~bitmask
 #define sbi(reg, bitmask) *reg |= bitmask
 #define rbi(reg, bitmask) ((*reg) & bitmask)

 #define pulse_high(reg, bitmask) sbi(reg, bitmask); cbi(reg, bitmask);
 #define pulse_low(reg, bitmask) cbi(reg, bitmask); sbi(reg, bitmask);

 #if defined(__arm__) && !(defined(TEENSYDUINO) && TEENSYDUINO >= 117)
  #define regtype volatile uint32_t
  #define regsize uint32_t
 #else
  #define regtype volatile uint8_t
  #define regsize uint8_t
 #endif

#endif

#define PORTRAIT 0
#define LANDSCAPE 1

#define PREC_LOW 1
#define PREC_MEDIUM 2
#define PREC_HI 3
#define PREC_EXTREME 4

class UTouch2
{
 public:
  int16_t TP_X ,TP_Y;

  UTouch2(byte tclk, byte tcs, byte din, byte dout, byte irq, int default_spi_mode, bool hw_spi);
  UTouch2(byte tclk, byte tcs, byte din, byte dout, byte irq);

  void InitSwSpiPins();
  void InitTouch(byte orientation = LANDSCAPE);
  void read();
  bool dataAvailable();
  int16_t getX();
  int16_t getY();
  void setPrecision(byte precision);
  
  void calibrateRead();
  
 private:
  regtype *P_CLK, *P_CS, *P_DIN, *P_DOUT, *P_IRQ;
  regsize B_CLK, B_CS, B_DIN, B_DOUT, B_IRQ;
  byte T_CLK, T_CS, T_DIN, T_DOUT, T_IRQ;
  long _default_orientation;
  byte orient;
  byte prec;
  byte display_model;
  long disp_x_size, disp_y_size, default_orientation;
  long touch_x_left, touch_x_right, touch_y_top, touch_y_bottom;
  bool ut_sw_spi, ut_sharing_hw_spi_pins;
  int ut_default_spi_mode;
  
  void touch_WriteData(byte data);
  word touch_ReadData();
};

#endif
