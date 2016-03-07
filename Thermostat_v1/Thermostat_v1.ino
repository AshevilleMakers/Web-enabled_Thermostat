
// tstat sketch, adapted from roof_fans_stage_2a.ino
// Mark Pendrith, Oct, Nov 2014

// modifications and user interface added
// Steve Altemeier, 2015

#include <Arduino.h>
#include "Rfx_config.h"
#include <Rfx_r3.h>
#include <SPI.h>
#include <EEPROM.h>

//*****TFT LCD Definitions**************
#include <UTouch2.h> // modified version that supports hw spi
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_HW_SPI 1
#define UT_HW_SPI 1  // 0 = no, 1 = sw spi on hw spi pins, 2 = full hw spi
#define UT_SPI_SWITCH (UT_HW_SPI == 1 && TFT_HW_SPI)
#define UT_SPI_MODE SPI_MODE3
#define TFT_SPI_MODE SPI_MODE0
#define TOUCH_ORIENTATION  LANDSCAPE
#define ROTATION 1

#define TFT_CS 10
#define TFT_RST -1 //Connected to MCU reset
#define TFT_DC 4 
#define TFT_MOSI 11
#define TFT_SCK 13
#define LED 7
#define TFT_MISO 12  //Not connected

#define T_SCK 13
#define T_CS 5 
#define T_DIN 11
#define T_DOUT 12
#define T_IRQ 8 // weirdness here. (PA2), 8 (PD5) works. 6(PB2), 7 (PB3) doesn't.
  
#if UT_HW_SPI
UTouch2  myTouch( T_SCK, T_CS, T_DIN, T_DOUT, T_IRQ, TFT_SPI_MODE, (UT_HW_SPI == 2));
#else
UTouch2  myTouch( T_SCK, T_CS, T_DIN, T_DOUT, T_IRQ);
#endif

#if TFT_HW_SPI
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#else
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);
#endif

#define TOUCH_TIMEOUT 10000UL //Timeout for touchscreen input (in millis) after which unsaved changes will be discarded
#define TURN_OFF_INTERVAL  60000UL  //interval of inactivity after which LED turns off (in millis)

int iButtonX;
int iButtonY;

//**************************************

//*****logging parameters**************************************************
#define LOG_IP_ADDR "localhost" // ip address string and
#define LOG_PORT 1700 // port for log recording
#define MASTER_PORT 1800 // port for control information

#define LOG_CMDS 1 // send commands recieved such as A, 0, 1 etc. to log record
#define LOG_SETTINGS 1 // send parameter settings commands such as s=250, h=0 etc. to log record
#define LOG_RELAYS 0 // send change in a relay state to log record
#define LOG_TEMP 0 // send change in temperature reading to log record

#define DBGCHAT 0 // enable or disable the "chat" to serial (used to aid debugging) 
//*************************************************************************

#define WDT 1 // enable or disable watch dog timer (might be better to disable for debugging) 



Rfx rfx;

//*****temp sensor parameters**********************************
#include <OneWire.h>  // for DS18b20 temperature sensor

#define FAHRENHEIT 1  //0 if celsius

#define TEMP_SENSOR_PIN 21 // analog pin A7 aka digital pin 21
int curr_Temp;
//*************************************************************************


//*****relay and control parameters***********************************************
#define RELAYS_ACTIVATE_HIGH 1  //set to 1 if relays turn ON when pin is HIGH

#define RELAYS 4 // no. of relays

#define RESISTANCE_HEAT_RELAY_PIN 14
#define HEAT_PUMP_ON_RELAY_PIN 15
#define HEAT_PUMP_REVERSE_RELAY_PIN 16
//This is fan pin on v2 boards
#define FAN_ON_RELAY_PIN 17
//This is fan pin on v1 boards
//#define FAN_ON_RELAY_PIN 18

uint8_t relay_pins[RELAYS]; // array mapping relays to digital pins, will be initialized by setup()
char *relay_labels[RELAYS]; // array of text string labels for relays, will be initialized by setup()
bool relay_displays[RELAYS];  //array of the state that is displayed on monitor for each relay

enum relays_ndx_t {RESISTANCE_HEAT_RELAY_NDX, HEAT_PUMP_ON_RELAY_NDX, HEAT_PUMP_REVERSE_RELAY_NDX, FAN_ON_RELAY_NDX}; 
     // automatically enumerated indices to consistently access above relay arrays

#define HVAC_COOLING 0 // (cooling) 
#define HVAC_HEAT_PUMP_ONLY 1 // (heat pump only) 
#define HVAC_RESISTANCE_HEAT_ONLY 2 // (resistance heat only) 
#define HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT 3 // (both heat pump and resistance heat).

uint8_t relay_state = 0; // initially all relays are off (no type)
uint8_t manual_override = 0xFF; // start as automatic mode (0=OFF, 1=ON, 0xff=AUTO)
uint8_t heating_mode = HVAC_HEAT_PUMP_ONLY; // start as heating (heat pump only) mode
uint8_t iSystem;  //function of manual_override and heating_mode - (0=OFF, 1=COOL, 2=HEAT, 3=AUX HEAT, 4=EM HEAT)
uint8_t iSystem_temp;  //temporary value used in LCD display

bool fan_on = false; // start as automatic fan mode
bool fan_on_temp = fan_on;  //temporary value used in LCD display
uint8_t pump_on_timeout = 5; // 5 mins
uint8_t pump_off_timeout = 5; // 5 mins
uint16_t deadman_timeout = 300; // 5 hours
uint8_t iHold = 0;  //Values are 0=Program, 1=Manual (temporary hold), 2=Hold (indefinite hold)
uint8_t iHold_temp = iHold;  //temporary value used in LCD display

#if FAHRENHEIT
  uint8_t turn_on_delta = 8; // 8 tenths of 1 degree from setpoint to turn on
  uint8_t turn_off_delta = 8; // 8 tenths of 1 degree from setpoint to turn on
#else
  uint8_t turn_on_delta = 5; // 5 tenths of 1 degree from setpoint to turn on
  uint8_t turn_off_delta = 5; // 5 tenths of 1 degree from setpoint to turn on
#endif

#if FAHRENHEIT
  uint16_t setpoint = 700; // 70.0 degrees 
#else
  uint16_t setpoint = 250; // 25.0 degrees 
#endif
uint16_t setpoint_temp = setpoint;  //temporary value used in LCD display
uint16_t displayTemp;

uint8_t reset_cmd = 0;

#define CONTROL_PERIOD 5000 // 5s period for sense/control loop
  
#define HVAC_NO_CONTROL 0 // control decision values
#define HVAC_TURN_ON 1
#define HVAC_TURN_OFF 2

static int control_state = HVAC_TURN_OFF;  // initial control state

//*************************************************************************

//*************************************************************************
//Variables defined for the automatic auxiliary heat control
unsigned long start_aux_check = 6*60000UL;//0;   //The start time for checking the aux heat; reset if the setpoint is increased or the heat pump is turned on; initial value is 6 minutes from start time
unsigned long last_aux_check;        //This is the last time (in millis) aux heat was checked
uint8_t aux_heat_freq_minutes = 5;    //Frequency, in minutes, to check to see if aux het needs to come one or off; default to 5
uint16_t t_last_aux_check;            //Romm temperature at the time of the last aux heat check

bool aux_temp_on = false;             //Boolean indicating if the thermostat has automaticaly turned the aux heat on temporarily
uint8_t max_heat_window_minutes = 60; //Allowed window for heat pump to get to setpoint, else aux heat comes on (note: code uses a projecton
                                      //rather than wait until the end of this window)

uint8_t aux_problem_timeout_min = 180; //Time-out for auxiliary heat if a potential problem is detected
uint8_t aux_heat_window_minutes = 10; //Potential problem is aux heat on and room temp has not increased at all in this amount of time
uint8_t max_cont_aux_heat_minutes = 30;  //Longest auxiliary heat can run continuously before being shut off due to potential problem
uint8_t min_increase = 3;             //The required temperature increase in tenths of a degree since last auxilliary heat check else auxilliary heat is turned on
//*************************************************************************

/** prototypes **/
//main tab
void user_if(bool sense_ctrl_time);  //user interface via serial input (i.e. remote access)
bool handle_serial_input();  //handle serial input sources. returns true if any serial input processed
void send_log_msg(int port, char *msg);  //send messages wia the rfx network to a specified port
void process_cmd(uint8_t c);  //Process received command; execute if a single character command else wait for additional characters
void var_assign(char cmd, char *val);  //Update system values based on commands received
void var_query(char *val);  //Reply to a query for a system value with the appropriate result
void read_eeprom_values();  //Read saved parameters from eeprom
void write_eeprom_values();  //Write saved parameters to eeprom
void eeprom_write(uint8_t addr, uint8_t val);  //Write function called for each parameter to be saved
void force_reset();  //Forces a reset of the thermostat
bool sense_ctrl();  //Short routine to control the temp reading and control routines
int16_t read_Temp_Sensor();  //Read the ds18b20 digital temperature sensor
bool get_relay(uint8_t relay);  //Get the current state of a relay
void set_relay(uint8_t relay, bool state);  //Set the state of a relay
void set_heat_pump_delay(bool heat_pump_on);  //Switch heat pump on/off but only if pump_on_timeout and pump_of_timeout have been satisfied
int do_control3a();  //This is the primary control function that determines whether relays need to be switched on or off
void print_state();  //This prints the current state of the thermostat to the rfx port
void print_relay_state();  //This prints the current state of the relays to the rfx port
void get_new_setpoint();  //This sends a message to server to return the new setpoint if user saves a change to the system mode via the touchscreen
void get_new_setpoint_temp(); //This sends a message to server to return the new temporary setpoint if user changes but has not saved the system mode via the touchscreen
void aux_heat_cntrl();  //This controls the automatic switching of the auxilliay heat based on the heat pumps ability to keep up with heating

//Touchscreen tab
void checkTouchscreenInput();  //Checks for touchscreen input and controls response if input detected
void getTouchCoords();  //Gets the X and Y coordinates for the user touch
void bubsort(int v[], size_t n);  //used to sort X and Y coordinates from the touchscreen
void changeSetTemp(int iMove);  //Changes the temporary value of setpoint if user manually increases or decreases
void changeFan();  //Changes the temporary value of fan_on
void changeHold();  //Changes the temporary value of the Hold parameter (Program, Temporary, or Hold)
void changeSystem();  //Changes the temporary value of the system parameter (Off, Heat, Cool ,etc.)
void resetTempVariables();  //Resets temporary values back to saved values if user does not save changes
void setSystemVariable();  //Sets the system parameter based on the manual_override and heating_mode parameters (the actual control parameters)
void saveChanges();  //Sets the control values of parameters to the temp values, writes values to eeprom, and saves values to database on server
void mainDisplay();  //Controls the main display screen
void displayRoomTemp(uint16_t iColor);  //displays the current room temperature
void displaySetTemp(uint16_t iColor);  //displays the temporary value of setpoint (which may or may not equal the actual value)
void displayLabels();  //Displays the System: and Fan: labels on the touchscreen
void displayHold(uint16_t iColor);  //Displays the temporary value of the Hold parameters
void displaySystem(uint16_t iColor);  //Displays the temporary value of the system parameter
void updateSystemDisplay();  //Updates the display of the system parameter if it is changed remotely rather than through the touchscreen
void displayFan(uint16_t iColor);  //Updates the temporary value of the fan_on parameter
void displaySave(uint16_t iColor);  //Displays the SAVE option if there are unsaved changes on the touchscreen
void displayRelayState();  //Displays the state of the relays on the touchscreen
void systemDisplay();  //displays the menu to select updates to the system parameter
void drawrectndx(int menundx, int itemndx, int col);  //draws a rectangle around the selected System choice

void setup();
void loop();

/** functions **/

// setup() and loop()

void setup()
{
  wdt_start();
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);  //With a high side transistor switch, LOW is ON and HIGH is OFF

  relay_labels[RESISTANCE_HEAT_RELAY_NDX] = PSTR("RESISTANCE_HEAT_RELAY");
  relay_labels[HEAT_PUMP_ON_RELAY_NDX] = PSTR("HEAT_PUMP_ON_RELAY");
  relay_labels[HEAT_PUMP_REVERSE_RELAY_NDX] = PSTR("HEAT_PUMP_REVERSE_RELAY");
  relay_labels[FAN_ON_RELAY_NDX] = PSTR("FAN_ON_RELAY");
  
  relay_pins[RESISTANCE_HEAT_RELAY_NDX] = RESISTANCE_HEAT_RELAY_PIN;
  relay_pins[HEAT_PUMP_ON_RELAY_NDX] = HEAT_PUMP_ON_RELAY_PIN;
  relay_pins[HEAT_PUMP_REVERSE_RELAY_NDX] = HEAT_PUMP_REVERSE_RELAY_PIN;
  relay_pins[FAN_ON_RELAY_NDX] = FAN_ON_RELAY_PIN;
  
  relay_state = 0; // all relays initially off (no)

  for (uint8_t i = 0; i < RELAYS; i++) {
    pinMode(relay_pins[i], OUTPUT);
    set_relay(i, !RELAYS_ACTIVATE_HIGH); // initially relays are all switched off
  }

  read_eeprom_values(); // load config settings from NV memory
  start_aux_check = (pump_on_timeout+1)*60000UL;  //This means the aux heat check will only begin 1 minute after the pump_on_timeout is over

  //read the temp sensor 10 times (seem to get bad values first few readings)
  for (int i =0; i<10; i++) curr_Temp = read_Temp_Sensor();
  
  rfx.begin_id(BOARD_ID,ID_STRING);
  
  //get the correct program temperature if iHold = 0 (Program mode)
  if (iHold==0) {
    get_new_setpoint();
  }

//TFT LCD setup
  tft.begin();
  tft.setRotation(ROTATION);

  myTouch.InitTouch(TOUCH_ORIENTATION);

  tft.fillScreen(ILI9341_BLACK);

  setSystemVariable();
  
  //Set temp variable values now that EEPROM has been read
  resetTempVariables();
  
  mainDisplay();  //Always displays values of temp variables
}

void loop()
{
  wdt_pat(); // pat the watchdog
  
  bool sense_ctrl_time = sense_ctrl();

  user_if(sense_ctrl_time);
  
  checkTouchscreenInput();
  
//Redraw display if room temperature has changed
  //Change this later to just redraw the room temperature
  if ((int)((float)curr_Temp/10.0+0.5) != displayTemp) {
    displayRoomTemp(displayTemp, true);
    displayRoomTemp((int)((float)curr_Temp/10.0+0.5), false);
  }

  rfx.sync(); // regular diagnostics keep nRF24L01+ link sane and healthy
}

// user interface routines

void user_if(bool sense_ctrl_time)
{
  bool sip = handle_serial_input(); //handle serial input sources. returns true if any serial input processed

#if DBGCHAT
  if (sip || sense_ctrl_time) 
    print_state(); // periodically report system status
#endif

}

bool handle_serial_input()
{
  bool sa = false;
  while (Serial.available()) {
    sa = true;
    int c = Serial.read();
    process_cmd(c); // may set manual_override or reset_cmd vars 
  }
  
  while (rfx.available()) {
    sa = true;
    int c = rfx.read();
    if (c >= ' ' || c == '\r' || c == '\n') {
      process_cmd(c); // may set manual_override or reset_cmd vars 
    }
  }
  
  if (reset_cmd) { 
    if (reset_cmd == 4) {
      rfx.fp("\n*** ");
      force_reset();
    }
    reset_cmd = 0;
  }
  
  wdt_pat();

  return sa;
}

//void send_log_msg( char *msg)
void send_log_msg(int port, char *msg)
{
  const int timeout = 7000;
  wdt_pat();
  bool ack = rfx.exec_start_request(timeout); 
  wdt_pat();
  if (ack) {
#if !DBGCHAT
    uint8_t devs = rfx.get_output_devs();
    rfx.dev_serial_off();
#endif
    rfx.fpf("echo \"%s\"| nc -q 1 -w 5 %s %u\n", msg, LOG_IP_ADDR, port);
#if !DBGCHAT
    rfx.set_output_devs(devs);
#endif
    ack = rfx.exec_recv_request(timeout);
    wdt_pat();
    if (ack) {
      ack = rfx.exec_wait_until_complete(timeout);
      wdt_pat();
    }
  }
  rfx.exec_stop_request(timeout); 
  wdt_pat();
}

//***************************************************************
#define SEND_FMT_LOG_MSG(port,fmt, ...) { char msg[100]; sfpf(msg, fmt, ##__VA_ARGS__); send_log_msg(port, msg); }
//***************************************************************

void process_cmd(uint8_t c)
{
  static int state = 0, ndx = 0;
  static char val[20];
  static char cmd;

  switch (state) { // parsing state
  case 0:
    c = toupper(c);

    switch (c) {
    case '0':
      manual_override = 0; // off
      setSystemVariable();
      updateSystemDisplay();
      displayRoomTemp(displayTemp, true);
      displayRoomTemp((int)((float)curr_Temp/10.0+0.5), false);
      break;
    case '1':
      manual_override = 1; // on
      setSystemVariable();
      updateSystemDisplay();
      break;
    case 'A':
      manual_override = 0xFF; // automatic mode
      setSystemVariable();
      updateSystemDisplay();
      break;
    case 'R':
      reset_cmd = 4; // force reset
      break;
    case 'W':
      write_eeprom_values();
#if DBGCHAT
      rfx.fpln("\n*** wrote eeprom");
#endif
      break;

    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'H':
    case 'P':
    case 'S':
    case 'X':
    case 'U':
    case 'Y':
    case '?':
      cmd = c;
      state = 1;
      break;
    default:
      break;
    }
#if LOG_CMDS
    if (state == 0 && c >= '0' && c <= 'W') {
      SEND_FMT_LOG_MSG(LOG_PORT,"!MSG/4321/%u/%c#",BOARD_ID,c);
    }
#endif
    break;
  case 1:
    if (c == ' ' || c == '=') state = 2;
    else state = 0;
    break;
  case 2:
    if (c == '\r' || c == '\n') {
      val[ndx] = '\0'; // delimit
      if (cmd == '?') var_query(val);
      else {
	var_assign(cmd,val);
#if LOG_SETTINGS
	SEND_FMT_LOG_MSG(LOG_PORT,"!MSG/4321/%u/%c=%s#",BOARD_ID,cmd, val);
#endif
      }
      state = 0;
      ndx = 0;
    }
    else if (ndx < (sizeof(val)-1)) {
      val[ndx++] = c;
    }
    else {
      state = 0;
      ndx = 0;
    }
    break;
  default:
    break;
  }
}

void var_assign(char cmd, char *val)
{
  int ival = atoi(val);

  switch (cmd) {
  case 'B':
    pump_on_timeout = ival;
    break;
  case 'C':
    pump_off_timeout = ival;
    break;
  case 'D':
    deadman_timeout = ival;
    break;
  case 'F':
    fan_on = ival;
    displayFan(ILI9341_BLACK);
    fan_on_temp = fan_on;
    displayFan(ILI9341_WHITE);
    break;
  case 'H':
    heating_mode = ival;
    setSystemVariable();
    updateSystemDisplay();
    displayRoomTemp(displayTemp, true);
    displayRoomTemp((int)((float)curr_Temp/10.0+0.5), false);
    break;
  case 'P':
    iHold = ival;
    displayHold(ILI9341_BLACK);  //Erase the current hold display
    iHold_temp = iHold;
    if (iSystem_temp) displayHold(ILI9341_WHITE);  //Write the new hold display
    break;
  case 'S':
    if (ival > setpoint) {
      start_aux_check = millis();
      last_aux_check = start_aux_check;
      t_last_aux_check = curr_Temp;
    }
    setpoint = ival;
    displaySetTemp(ILI9341_BLACK);
    setpoint_temp = setpoint;
    if (iSystem_temp == 1) displaySetTemp(ILI9341_BLUE);
    else if (iSystem_temp > 1) displaySetTemp(ILI9341_RED);
    break;
  case 'U':  //This is getting the program temp from the server, but only changing the display variable (until SAVE is pressed)
    displaySetTemp(ILI9341_BLACK);
    setpoint_temp = ival;
    if (iSystem_temp == 1) displaySetTemp(ILI9341_BLUE);
    else if (iSystem_temp > 1) displaySetTemp(ILI9341_RED);
    break;
  case 'X':
    turn_on_delta = ival;
    break;
  case 'Y':
    turn_off_delta = ival;
    break;
  default:
#if DBGCHAT
    rfx.fpln("\n*** unrecognised cmd\n");
#endif
    break;
  }
}

void var_query(char *val)
{
  int ival = -1, cmd = toupper(val[0]);

  switch (cmd) {
  case 'A':
    ival = manual_override; // additional query for manual_override/automatic mode
    break;
  case 'B':
    ival = pump_on_timeout;
    break;
  case 'C':
   ival =  pump_off_timeout;
    break;
  case 'D':
    ival = deadman_timeout;
    break;
  case 'F':
    ival = fan_on;
    break;
  case 'H':
    ival = heating_mode;
    break;
  case 'P':
    ival = iHold;
    break;
  case 'R':
    ival = relay_state; // additional query for relay state
    break;
  case 'S':
    ival = setpoint;
    break;
  case 'T':
    ival = curr_Temp; // additional query for current temp
    break;
  case 'U':
    ival = setpoint_temp;
  case 'X':
    ival = turn_on_delta;
    break;
  case 'Y':
    ival = turn_off_delta;
    break;
  default:
#if DBGCHAT
    rfx.fpln("\n*** unrecognised cmd\n");
#endif
    break;
  }
  if (ival >= 0) rfx.fpf("%c=%d\r\n",cmd,ival);
  else rfx.fpf("%c=?\r\n",cmd);
}

void read_eeprom_values()
{
  manual_override = EEPROM.read(0); // read saved mode from eeprom
  if ((heating_mode = EEPROM.read(1)) == 0xFF) heating_mode = HVAC_HEAT_PUMP_ONLY; // default
  if ((fan_on = EEPROM.read(2)) == 0xFF) fan_on = 0; // default
  if ((pump_on_timeout = EEPROM.read(3)) == 0xFF) pump_on_timeout = 9; // default
  if ((pump_off_timeout = EEPROM.read(4)) == 0xFF) pump_off_timeout = 5; // default
#if FAHRENHEIT
  if ((turn_off_delta = EEPROM.read(5)) == 0xFF) turn_off_delta = 8; // default
  if ((turn_on_delta = EEPROM.read(6)) == 0xFF) turn_on_delta = 8; // default
#else
  if ((turn_off_delta = EEPROM.read(5)) == 0xFF) turn_off_delta = 5; // default
  if ((turn_on_delta = EEPROM.read(6)) == 0xFF) turn_on_delta = 5; // default
#endif
#if FAHRENHEIT
  if ((setpoint = EEPROM.read(9)) == 0xFF) setpoint = 720; // default
#else
  if ((setpoint = EEPROM.read(9)) == 0xFF) setpoint = 250; // default
#endif 
  else {
    setpoint <<= 8;
    setpoint += EEPROM.read(10); // read low byte of setpoint value
  }
  if ((deadman_timeout = EEPROM.read(11)) == 0xFF) deadman_timeout = 300; // default
  else {
    deadman_timeout <<= 8;
    deadman_timeout += EEPROM.read(12); // read low byte of deadman_timeout value
  }
  if ((iHold = EEPROM.read(13)) == 0xFF) iHold = 0; // default
}

void write_eeprom_values()
{
  eeprom_write(0,manual_override); // eeprom write 
  eeprom_write(1,heating_mode); // eeprom write 
  eeprom_write(2,fan_on); // eeprom write 
  eeprom_write(3,pump_on_timeout); // eeprom write 
  eeprom_write(4,pump_off_timeout); // eeprom write 
  eeprom_write(5,turn_on_delta); // eeprom write 
  eeprom_write(6,turn_off_delta); // eeprom write 
  eeprom_write(9,(setpoint >> 8)); // eeprom write high byte 
  eeprom_write(10,(setpoint & 0xFF)); // eeprom write low byte
  eeprom_write(11,deadman_timeout >> 8); // eeprom write high byte
  eeprom_write(12,deadman_timeout & 0xFF); // eeprom write low byte
  eeprom_write(13,iHold); //eeprom write
}

void eeprom_write(uint8_t addr, uint8_t val)
{
  uint8_t b = EEPROM.read(addr);
  if (b != val) EEPROM.write(addr,val);
}

void force_reset()
{
  // force AVR reset via WDT
#if WDT
  //wdt_start();
  wdt_pat(); 
#if DBGCHAT
  rfx.fpln("forced reset in 8s...");
#endif
  while(1);
#endif
}

// sense and control routines
  
bool sense_ctrl()
{
  static unsigned long lastctrl = 0;
  unsigned long ms = millis();
  bool sense_ctrl_time = ((ms  - lastctrl) > CONTROL_PERIOD);

  if (sense_ctrl_time) {
    curr_Temp = read_Temp_Sensor(); // read current temperature
    control_state = do_control3a(); // actuate control, report status
    lastctrl = ms;
  }
  return sense_ctrl_time;
}

int16_t read_Temp_Sensor() {

//Use the OneWire library to read a single ds18b20 temperature sensor
  OneWire ds(TEMP_SENSOR_PIN);// = ds_array[0];  //Index = # of sensors to read minus 1, so 0 for a single sensor
  const bool type_s = false; // false for ds18B20, chsnge to true for ds18S20 or old DS1820
  uint8_t data[12];

  ds.reset();
  ds.skip();

  ds.write(0x44, 0);        // start conversion, with parasite power off at the end
  
  ds.reset();
  ds.skip();

  ds.write(0xBE);         // Read Scratchpad

  for (uint8_t j = 0; j < 9; j++) {           // we need 9 bytes
    data[j] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

//Convert the raw reading to a temperature in either Celsius or Fahrenheit times 10 (e.g. 72.4 => 724)
#if FAHRENHEIT
  return (raw * 9) / 8 + 320; // degrees F * 10
#else
  return (raw * 10) / 16; // degrees C * 10
#endif
}

bool get_relay(uint8_t relay) // get current relay state
{
  bool state = false;
  if (relay < RELAYS) {
    uint8_t mask = 1<<relay;
    state = ((relay_state & mask) != 0);
  }  
  return state;
}

void set_relay(uint8_t relay, bool state)
{
  if (relay < RELAYS) {
    int pin = relay_pins[relay];
#if RELAYS_ACTIVATE_HIGH
    digitalWrite(pin, state); // relays activate HIGH
#else
    digitalWrite(pin, !state); // relays activate LOW
#endif
    bool old_state = get_relay(relay);
#if DBGCHAT
    rfx.fpf("set_relay(): relay %d (%P) state %d",relay, relay_labels[relay], state);
#endif

    if (state != old_state) {
      uint8_t mask = 1<<relay;
      if (state) relay_state |= mask;
      else relay_state &= ~(mask);
      displayRelayState();
#if DBGCHAT
      rfx.fpf(" -- set relay %d to %d\n",relay, state);
#endif
#if LOG_RELAYS
      SEND_FMT_LOG_MSG(LOG_PORT,"!MSG/4321/%u/%d %d %P#", BOARD_ID, relay, state, relay_labels[relay]);
#endif
    }
#if DBGCHAT
    else rfx.fpf(" -- relay %d is %d (no change)\n",relay, state);
#endif
  }
}

inline void set_resistance_heat(bool resistance_heat_on)
{
  set_relay(RESISTANCE_HEAT_RELAY_NDX, resistance_heat_on);
}

inline void set_heat_pump_on(bool heat_pump_on)
{
  set_relay(HEAT_PUMP_ON_RELAY_NDX, heat_pump_on);
}

inline void set_heat_pump_reverse(bool cooling_mode)
{
  set_relay(HEAT_PUMP_REVERSE_RELAY_NDX, cooling_mode);
}

inline void set_fan_on(bool fan_on)
{
  set_relay(FAN_ON_RELAY_NDX, fan_on);
}

void set_heat_pump_delay(bool heat_pump_on)
{
  static bool current_state = false;

#if DBGCHAT
  rfx.fpf("heat_pump_on state %d\n",heat_pump_on);
#endif

  if (current_state != heat_pump_on) {
    static unsigned long last_switch = 0;
    unsigned long ms = millis();
    unsigned long elapsed = ms - last_switch;
    unsigned long timeout = (heat_pump_on) ? pump_on_timeout : pump_off_timeout;
    timeout *= 60000UL; // convert mins to ms

#if DBGCHAT
    rfx.fpf("elapsed %ld timeout %ld\n",elapsed,timeout);
#endif

    if (elapsed >= timeout) {
      set_heat_pump_on(heat_pump_on);
      current_state = heat_pump_on;
      last_switch = ms;
      if ((heating_mode == HVAC_HEAT_PUMP_ONLY) && (heat_pump_on)) {
        //Turning heat pump on in mode HVAC_HEAT_PUMP_ONLY
        start_aux_check = ms;
        last_aux_check = ms;
        t_last_aux_check = curr_Temp;
      }
    }
#if DBGCHAT
    else rfx.fpln(" -- switching delayed");
#endif
  }
}

int do_control3a() 
{
  /*
    this function assumes acces to some global parms: 
    pump_on_timeout, pump_off_timeout
    manual_override, heating_mode, fan_on

    heating_mode can be one of four values: 

    0 (cooling) 
    1 (heat pump only) 
    2 (resistance heat only) 
    3 (both heat pump and resistance heat).

    manual_override has three values:

    0 hvac control override off
    1 hvac control override on
    0xff automatic (thermostat control) mode. 

    setting 1 also has a deadman timer associated with it, so that if no new control imputs have been received it 
    will revert to the previous control state before it was switched to on (off or automatic mode). 

    fan_on has two values (bool):

    true (hvac fan manual on)
    false (hvac fan automatic)
    
    pump_on_timeout is the time (mins) the pump must be off before restarting.
    pump_off_timeout is the time the pump must be on before it can be turned off.
  */

  // actuate control
  bool heat_pump_on = false;
  bool resistance_heat_on = false; 
  bool cooling_mode = (heating_mode == HVAC_COOLING);
  uint8_t hvac_ctrl = HVAC_NO_CONTROL;
  static uint8_t prev_manual_override, last_manual_override = 0;
  static int last_temp = -32000;
  int current_temp = curr_Temp;

  // first, check on deadman timeout for manual override ON setting

  if (manual_override == 1) {
    unsigned long ms = millis();
    static unsigned long manual_on_time;
    if (last_manual_override != 1) { // just switched to manual ON setting
      manual_on_time = ms; // record time
      prev_manual_override = last_manual_override; // record previous setting
    }
    if ((ms - manual_on_time) > (deadman_timeout*60000UL)) { // deadman timeout for ON setting has expired
      manual_override = prev_manual_override; // revert to previous setting
    }
  }

  last_manual_override = manual_override;

  //  next decide if resistance heat should be turned on
  aux_heat_cntrl();

  // next, decide if HVAC needs turning on, off, or left as is

  if (manual_override == 0) {
    // manual control off setting
    hvac_ctrl = HVAC_TURN_OFF;
  }
  else if (manual_override == 1) {
    // manual control on setting
    hvac_ctrl = HVAC_TURN_ON;
  }
  else {
    manual_override = 0xFF; // automatic control setting
    int t_delta = (current_temp - setpoint); 
    // t_delta +ve means temp higher than sepoint, -ve means temp lower than setpoint
    if (cooling_mode) {
#if DBGCHAT
      rfx.fpf("cooling (heating_mode = %d), t_delta %d\n", heating_mode, t_delta);
#endif
      if (t_delta >= turn_on_delta) hvac_ctrl = HVAC_TURN_ON;
      else if (-t_delta >= turn_off_delta) hvac_ctrl = HVAC_TURN_OFF;
    }
    else {
#if DBGCHAT
      rfx.fpf("heating (heating_mode = %d), t_delta %d\n", heating_mode, t_delta);
#endif
      if (-t_delta >= turn_on_delta) hvac_ctrl = HVAC_TURN_ON;
      else if (t_delta >= turn_off_delta) {
        hvac_ctrl = HVAC_TURN_OFF;
        if ((heating_mode == HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT)  && (aux_temp_on)) {
          heating_mode = HVAC_HEAT_PUMP_ONLY;
          aux_temp_on = false;
        }
      }
    }
  }

  // next, decide what relays (if any) need to be switched on or off

  if (hvac_ctrl == HVAC_TURN_OFF) {
#if DBGCHAT
    rfx.fpln("HVAC_TURN_OFF");
#endif
    heat_pump_on = false;
    resistance_heat_on = false;
  }
  else if (hvac_ctrl == HVAC_TURN_ON) {
#if DBGCHAT
    rfx.fpln("HVAC_TURN_ON");
#endif
    if (cooling_mode) {
      heat_pump_on = true;
      resistance_heat_on = false;
    } 
    else if (heating_mode == HVAC_HEAT_PUMP_ONLY) {
      // heat pump only
      heat_pump_on = true;
      resistance_heat_on = false;
    }
    else if (heating_mode == HVAC_RESISTANCE_HEAT_ONLY) {
      // resistance heat only
      heat_pump_on = false;
      resistance_heat_on = true;
    }
    else if (heating_mode == HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT) {
      // both heat pump and resistance heat
      heat_pump_on = true;
      resistance_heat_on = true;
    }
  }

  // heat pump reverse mode relay and resistance heat relay

  if (hvac_ctrl != HVAC_NO_CONTROL) { // "HVAC_NO_CONTROL means it's in hysteresis/deadband range
#if DBGCHAT
    rfx.fpln("switching relays.");
#endif

    bool toggling_reverse_mode = (cooling_mode != get_relay(HEAT_PUMP_REVERSE_RELAY_NDX));

    if (toggling_reverse_mode) set_heat_pump_delay(false); // switch off heat pump after reverse relay change
    else set_heat_pump_delay(heat_pump_on);

    bool heat_pump_relay_off = (get_relay(HEAT_PUMP_ON_RELAY_NDX) == false);

    if (toggling_reverse_mode && heat_pump_relay_off) set_heat_pump_reverse(cooling_mode);

    set_resistance_heat(resistance_heat_on);
  }

  bool fan_state = (fan_on || get_relay(HEAT_PUMP_ON_RELAY_NDX) || get_relay(RESISTANCE_HEAT_RELAY_NDX)); 

#if DBGCHAT
  rfx.fpf("fan_state = %d\n",fan_state);
#endif

  set_fan_on(fan_state); // fan control not affected by hysteresis/deadband range

  // perhaps need to consider whether switching other control parameters (e.g. resistance heat)
  // should override deadband inaction. I don't think so.

  // finally, report if temperature has changed
#if LOG_TEMP
  if (current_temp != last_temp) {
    SEND_FMT_LOG_MSG(LOG_PORT,"!MSG/4321/%u/t %d#",BOARD_ID,current_temp);
    last_temp = current_temp;
  }
#endif

  return hvac_ctrl;
}

void print_relay_state()
{
  for (uint8_t i = 0; i < RELAYS; i++) {
    rfx.fpf("relay %d (%P - pin %d) is %P\n",i,relay_labels[i],relay_pins[i],(get_relay(i) ? PSTR("ON") : PSTR("OFF")));
  }
}

void print_state()
{
  // could stick in all chat from ctrl() in here -- just pass manual_override

  if (manual_override == 0 || manual_override == 1) {
    rfx.fp("\n*** manual override setting ");
    if (manual_override) rfx.fpln("ON");
    else rfx.fpln("OFF");
  }
  else {
    rfx.fpf("\n*** automatic mode, relay state %x\r\n", relay_state);
  }

  rfx.fpln("*** (Enter A for automatic mode, 0 manual override Off, 1 for manual override On)");

  rfx.prln();

  rfx.fpf("heating_mode = %d, fan_on = %d, hold = %d\n", heating_mode, fan_on, iHold);
  rfx.fpf("setpoint = %d\n", setpoint);

//  for (int i = 0; i < SENSORS; i++) { 
    rfx.fpf("curr_Temp = %d\n", curr_Temp);
 // }
  int t_delta = (curr_Temp - setpoint); 

  rfx.fpf("delta = %d, turn_off_delta = %d, turn_on_delta = %d\n", t_delta, turn_off_delta, turn_on_delta);
  rfx.fpf("\nrelay_state = %d\n", relay_state);

  print_relay_state();

  rfx.fpf("\npump_on_timeout = %d, pump_off_timeout = %d\n", pump_on_timeout, pump_off_timeout);
  rfx.fpf("deadman_timeout = %d\n", deadman_timeout);

  rfx.fp("\nuptime: ");
  print_uptime();
  rfx.fpln("\n");
}


void get_new_setpoint() {
 
  char tstat_program[2];
  
  if (heating_mode==0) strcpy(tstat_program, "C");
  else strcpy(tstat_program, "H");
  SEND_FMT_LOG_MSG(MASTER_PORT,"!TPU/4321/%u/%s#", BOARD_ID, tstat_program); 
}


void get_new_setpoint_temp() {
 
  char tstat_program[2];
  
  if (iSystem_temp != 0) {  //Temp system setting is not OFF
    if (iSystem_temp==1) strcpy(tstat_program, "C");
    else strcpy(tstat_program, "H");
    SEND_FMT_LOG_MSG(MASTER_PORT,"!TPT/4321/%u/%s#", BOARD_ID, tstat_program);
  }
}


//*************************************************************************
//SA - Added this code to control auxiliary heat if heat pump cannot keep up
//Should there also be code at some point to turn on back-up heat if outside temperature is below some level?


void aux_heat_cntrl() {
  
  static unsigned long start_auxiliary_heat;  //The time (in millis) when the thermostat automatically turned on the aux heat 
                                              //by setting heating_mode = HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT
  static bool aux_problem = false;  //boolean indicating whether auxiliary heat was turned off due to a potential problem
  static unsigned long aux_problem_time = -aux_problem_timeout_min * 60000UL;  //So can turn on RH as needed any time after startup; aux_problem_time
                                                                 //is the time in millis when the auxiliary heat was last turned off
                                                                 //due to a problem  
 
  //Do nothing unless system is on AUTO
  if (manual_override == 0xFF) {
    //If heating mode is just the heat pump, check to see if should turn on auxiliary heat
    if ((heating_mode == HVAC_HEAT_PUMP_ONLY) && (get_relay(1))) {  //heating mode is heat pump only and heat pump is on
      //Turn on the auxiliary heat if heat pump is not keeping up (has been on for at least [10]
      //minutes, and in last [5] minutes, temp not increasing on pace to reach set temp within [60]
      //minutes OR temp not increased by at least [0.3] degrees in last 5 minutes)
        if ((millis() - last_aux_check) > aux_heat_freq_minutes*60000UL) { //300000UL) {  //Check every [5] minutes

 Serial.println("*********************"); Serial.println();
 Serial.print("start_aux_check: "); Serial.println(start_aux_check);
 Serial.print("millis: "); Serial.println(millis());
 Serial.print("last_aux_check: "); Serial.println(last_aux_check);
 Serial.println("*********************"); Serial.println();

          last_aux_check = millis();
          //1 hour => millis() = 3600000 + start_rh_check + pump_on_timeout * 60000 [last term since another tstat may have just turned pump off]
          float minutes_left = (float)max_heat_window_minutes - (float)(millis() - start_aux_check)/60000.0;
          //minutes_left is the number of minutes in the time period allowed for the heat pump to reach the setpoint
          if (millis()-aux_problem_time > aux_problem_timeout_min*60000UL) {  //Don't turn resistance heat on if problem within last 3 hours
            //Turn resistance heat on if temperature is dropping:

 Serial.println("*********************"); Serial.println();
 Serial.print("curr_Temp: "); Serial.println(curr_Temp);
 Serial.print("t_last_aux_check: "); Serial.println(t_last_aux_check);
 Serial.print("minutes_left: "); Serial.println(minutes_left);
 Serial.print("max_heat_window_minutes: "); Serial.println(max_heat_window_minutes);
 Serial.print("start_aux_check: "); Serial.println(start_aux_check);
 Serial.print("millis(): "); Serial.println(millis());
 Serial.print("setpoint: "); Serial.println(setpoint);
 Serial.print("Check value: "); Serial.println((curr_Temp - t_last_aux_check) * minutes_left / aux_heat_freq_minutes);
 Serial.println("*********************"); Serial.println();

            //Why do I need this if have code below to turn on if temp has not increased by at least 0.3 degrees?
/*            if (curr_Temp < t_last_aux_check) {
              heating_mode = HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT;
              start_resistance_heat = millis();
              rh_temp_on = true;
            }
*/
            //Turn resistance heat on if not on pace to reach setpoint within 1 hour of start_aux_check
//            else if (setpoint - curr_Temp > (curr_Temp - t_last_aux_check) * minutes_left) {
            if ((curr_Temp < t_last_aux_check) || (setpoint - curr_Temp > (curr_Temp - t_last_aux_check) * minutes_left / aux_heat_freq_minutes)) {
              heating_mode = HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT;
              start_auxiliary_heat = millis();
              aux_temp_on = true;
Serial.println("Turned on aux heat due to forecast");
            }
            //Turn resistance heat on if temp not increased by at least [0.3] degrees since last aux heat check
            else if ((curr_Temp < t_last_aux_check) || (curr_Temp - t_last_aux_check < min_increase)) {
              heating_mode = HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT;
              start_auxiliary_heat = millis();
              aux_temp_on = true;
Serial.println("Turned on aux heat due to insufficient temp rise");
            }
          }
          t_last_aux_check = curr_Temp;
        }
//      }
    }
    //Else if heatimg mode is heat pump and resistance heat, check to see if should turn off resistance heat
    else if ((heating_mode == HVAC_HEAT_PUMP_AND_RESISTANCE_HEAT) && (aux_temp_on)) {
      //Turn OFF resistance heat if have reached the set temp [plus delta]
      //Regular code does this, but also need to change the heating_mode
      //Turn OFF the auxiliary heat if has been on for [30] minutes continuously
      //Turn OFF the auxiliary heat if has been on for [10] minutes and room temperature has not increased
      //If turn off here due to a problem, should not just turn it back on next loop.  Should
      //send message to alert user and keep it off (a) for [3] hours or (b) until user sends 
      //a reset signal.  For now, going the [3] hour route for testing.
      if (millis() - start_auxiliary_heat > max_cont_aux_heat_minutes * 60000UL) {
        heating_mode = HVAC_HEAT_PUMP_ONLY;
        aux_temp_on = false;
        SEND_FMT_LOG_MSG(LOG_PORT,"!MSG/4321/%u/Aux_Heat_Err_1#",BOARD_ID);
        SEND_FMT_LOG_MSG(LOG_PORT,"!ELG/4321/%u/1#",BOARD_ID);
        aux_problem_time = millis();
      }
      //Turn OFF the resistance heat if has been on for [10] minutes and temp not increassing (implies temp sensor may have failed)
      else if ((millis() - start_auxiliary_heat > aux_heat_window_minutes * 60000UL) && !(curr_Temp > t_last_aux_check)) {
        heating_mode = HVAC_HEAT_PUMP_ONLY;
        aux_temp_on = false;
        SEND_FMT_LOG_MSG(LOG_PORT,"!MSG/4321/%u/Aux_Heat_Err_2#",BOARD_ID);
        SEND_FMT_LOG_MSG(LOG_PORT,"!ELG/4321/%u/2#",BOARD_ID);
        aux_problem_time = millis();
      }    
    }
  }
}
//*************************************************************************

