/*
  UTouch2.cpp - Arduino/chipKit library support for Color TFT LCD Touch screens 
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

#include "UTouch2.h"
#include "UTouchCD.h"

#include <SPI.h>

UTouch2::UTouch2(byte tclk, byte tcs, byte din, byte dout, byte irq, int default_spi_mode, bool hw_spi)
{
  // need to use this version if sharing hw spi pins sck, mosi, miso for clk, din, dout

  T_CLK	= tclk;
  T_CS = tcs;
  T_DIN	= din;
  T_DOUT = dout;
  T_IRQ	= irq;

  // if default_spi_mode passed as < 0, then behaves as the "not sharing hw spi pins" version,
  // and the hw_spi parameter is ignored 

  ut_default_spi_mode = default_spi_mode;

  if (default_spi_mode < 0) {
    ut_sharing_hw_spi_pins = false;
    ut_sw_spi = true;
  }
  else {
    ut_sharing_hw_spi_pins = true;
    ut_sw_spi = !hw_spi;
  }
}

UTouch2::UTouch2(byte tclk, byte tcs, byte din, byte dout, byte irq)
{
  // use this version if not sharing hw spi pins
  UTouch2(tclk, tcs, din, dout, irq, -1, false);
}

void UTouch2::InitSwSpiPins()
{
  // init/reinit clk, din, dout pins for sw spi

  if (ut_sw_spi) {
    pinMode(T_CLK, OUTPUT);
    pinMode(T_DIN, OUTPUT);
    pinMode(T_DOUT, INPUT);
  }
}

void UTouch2::InitTouch(byte orientation)
{
  orient = orientation;
  _default_orientation = CAL_S >> 31;
  touch_x_left = (CAL_X >> 14) & 0x3FFF;
  touch_x_right = CAL_X & 0x3FFF;
  touch_y_top = (CAL_Y >> 14) & 0x3FFF;
  touch_y_bottom = CAL_Y & 0x3FFF;
  disp_x_size = (CAL_S >> 12) & 0x0FFF;
  disp_y_size = CAL_S & 0x0FFF;
  prec = 10;

  if (ut_sw_spi) {
    P_CLK = portOutputRegister(digitalPinToPort(T_CLK));
    B_CLK = digitalPinToBitMask(T_CLK);
    P_DIN = portOutputRegister(digitalPinToPort(T_DIN));
    B_DIN = digitalPinToBitMask(T_DIN);
    P_DOUT = portInputRegister(digitalPinToPort(T_DOUT));
    B_DOUT = digitalPinToBitMask(T_DOUT);

    this->InitSwSpiPins(); // init clk, din, dout pins for sw spi
  }

  P_CS	= portOutputRegister(digitalPinToPort(T_CS));
  B_CS	= digitalPinToBitMask(T_CS);
  P_IRQ	= portInputRegister(digitalPinToPort(T_IRQ));
  B_IRQ	= digitalPinToBitMask(T_IRQ);

  pinMode(T_CS, OUTPUT);
  pinMode(T_IRQ, OUTPUT);

  sbi(P_CS, B_CS);
  sbi(P_IRQ, B_IRQ);
}

void UTouch2::read()
{
  unsigned long tx = 0, temp_x = 0;
  unsigned long ty = 0, temp_y = 0;
  unsigned long minx = 99999, maxx = 0;
  unsigned long miny = 99999, maxy = 0;
  int datacount = 0;
  
  cbi(P_CS, B_CS);                    
  
  pinMode(T_IRQ, INPUT);
  for (int i = 0; i < prec; i++) {
    if (!rbi(P_IRQ, B_IRQ)) {
      touch_WriteData(0x90);        

      if (ut_sw_spi) pulse_high(P_CLK, B_CLK);
      
      temp_x = touch_ReadData();
       
      if (!rbi(P_IRQ, B_IRQ)) {
	touch_WriteData(0xD0);      

	if (ut_sw_spi) pulse_high(P_CLK, B_CLK);
	
	temp_y = touch_ReadData();
	
	if ((temp_x > 0) and (temp_x < 4096) and (temp_y > 0) and (temp_y < 4096)) {
	  tx += temp_x;
	  ty += temp_y;
	  if (prec > 5) {
	    if (temp_x < minx)
	      minx = temp_x;
	    if (temp_x > maxx)
	      maxx = temp_x;
	    if (temp_y < miny)
	      miny = temp_y;
	    if (temp_y > maxy)
	      maxy = temp_y;
	  }
	  datacount++;
	}
      }
    }
  }
  pinMode(T_IRQ, OUTPUT);
  
  if (prec > 5) {
    tx = tx-(minx+maxx);
    ty = ty-(miny+maxy);
    datacount -= 2;
  }

  sbi(P_CS, B_CS);                    
  if ((datacount == (prec-2)) or (datacount == PREC_LOW)) {
    if (orient == _default_orientation) {
      TP_X = ty/datacount;
      TP_Y = tx/datacount;
    }
    else {
      TP_X = tx/datacount;
      TP_Y = ty/datacount;
    }
  }
  else {
    TP_X=-1;
    TP_Y=-1;
  }
}

bool UTouch2::dataAvailable()
{
  bool avail;
  pinMode(T_IRQ, INPUT);
  avail = !(rbi(P_IRQ, B_IRQ));
  pinMode(T_IRQ, OUTPUT);
  return avail;
}

int16_t UTouch2::getX()
{
  long c;

  if ((TP_X == -1) or (TP_Y == -1))
    return -1;

  if (orient == _default_orientation) {
    c = long(long(TP_X - touch_x_left) * (disp_x_size)) / long(touch_x_right - touch_x_left);
    if (c < 0)
      c = 0;
    if (c > disp_x_size)
      c = disp_x_size;
  }
  else {
    if (_default_orientation == PORTRAIT)
      c = long(long(TP_X - touch_y_top) * (-disp_y_size)) / long(touch_y_bottom - touch_y_top) + long(disp_y_size);
    else
      c = long(long(TP_X - touch_y_top) * (disp_y_size)) / long(touch_y_bottom - touch_y_top);
    if (c < 0)
      c = 0;
    if (c > disp_y_size)
      c = disp_y_size;
  }
  return c;
}

int16_t UTouch2::getY()
{
  int c;

  if ((TP_X == -1) or (TP_Y == -1))
    return -1;
  if (orient == _default_orientation) {
    c = long(long(TP_Y - touch_y_top) * (disp_y_size)) / long(touch_y_bottom - touch_y_top);
    if (c < 0)
      c = 0;
    if (c > disp_y_size)
      c = disp_y_size;
  }
  else {
    if (_default_orientation == PORTRAIT)
      c = long(long(TP_Y - touch_x_left) * (disp_x_size)) / long(touch_x_right - touch_x_left);
    else
      c = long(long(TP_Y - touch_x_left) * (-disp_x_size)) / long(touch_x_right - touch_x_left) + long(disp_x_size);
    if (c < 0)
      c = 0;
    if (c > disp_x_size)
      c = disp_x_size;
  }
  return c;
}

void UTouch2::setPrecision(byte precision)
{
  switch (precision) {
  case PREC_LOW:
    prec = 1;		// DO NOT CHANGE!
    break;
  case PREC_HI:
    prec = 27;	// Iterations + 2
    break;
  case PREC_EXTREME:
    prec = 102;	// Iterations + 2
    break;
  default: // default is PREC_MEDIUM:
    prec = 12;	// Iterations + 2
    break;
  }
}

void UTouch2::calibrateRead()
{
  unsigned long tx = 0;
  unsigned long ty = 0;
  
  cbi(P_CS, B_CS);                    
  
  touch_WriteData(0x90);

  if (ut_sw_spi) pulse_high(P_CLK, B_CLK);
  
  tx = touch_ReadData();

  touch_WriteData(0xD0);      

  if (ut_sw_spi) pulse_high(P_CLK, B_CLK);

  ty = touch_ReadData();

  sbi(P_CS, B_CS);                    

  TP_X = ty;
  TP_Y = tx;
}

// low level SPI io routines

#if defined(__AVR__)
#define SET_HIGH(p) sbi(P_##p, B_##p)
#define SET_LOW(p) cbi(P_##p, B_##p)
#define READ_PIN(p) rbi(P_##p, B_##p)
#else
#define SET_HIGH(p) digitalWrite(T_##p, HIGH)
#define SET_LOW(p) digitalWrite(T_##p, LOW)
#define READ_PIN(p) digitalRead(T_##p)
#endif

#define UT_SPI_MODE SPI_MODE3

void UTouch2::touch_WriteData(byte data)
{
  if (ut_sw_spi) {
    if (ut_sharing_hw_spi_pins) {
      SPI.end();
      this->InitSwSpiPins();  
    }     	
    byte temp;
    
    temp = data;
    SET_LOW(CLK);                

    for(byte count = 0; count < 8; count++) {
      if(temp & 0x80)
	SET_HIGH(DIN);
      else
	SET_LOW(DIN);
      temp = temp << 1; 
      SET_LOW(CLK);                
      SET_HIGH(CLK);
    }
    if (ut_sharing_hw_spi_pins) {
      SPI.begin();
      SPI.setDataMode(ut_default_spi_mode);
    }
  }
  else {
    SPI.setDataMode(UT_SPI_MODE);
    SPI.transfer(data);
    SPI.setDataMode(ut_default_spi_mode);
  }
}

word UTouch2::touch_ReadData()
{
  word data = 0;
  
  if (ut_sw_spi) {	
    if (ut_sharing_hw_spi_pins) {
      SPI.end();
      this->InitSwSpiPins();  
    }     	
    for(byte count = 0; count < 12; count++) {
      data <<= 1;
      SET_HIGH(CLK);
      SET_LOW(CLK);                
      if (READ_PIN(DOUT))
	data++;
    }
    if (ut_sharing_hw_spi_pins) {
      SPI.begin();
      SPI.setDataMode(ut_default_spi_mode);
    }
  }
  else {
    int data1 = 0;
    int data2 = 0;
    
    SPI.setDataMode(UT_SPI_MODE);
    data1 = SPI.transfer(0);
    data2 = SPI.transfer(0);
    SPI.setDataMode(ut_default_spi_mode);
    
    data = data1;
    data <<= 4; // most significant 8 bits from data1
    data += (data2 >> 4); // add least significant 4 bits from data2
  }
  
  return(data);
}
