#define SAM 20 // sample size for averaging
#define TEMP_MOVE 10  // temp is in degrees times 10 so move 10 on button press

bool unsavedChanges = false;

void checkTouchscreenInput() {

  static unsigned long lastInput = 0;  //millis() value of last registered touchscreen input
  static bool inSystemMenu = false;  //indicates whether touchscreen in main menu (0) or system menu (1)
  
  if (myTouch.dataAvailable()) {
    
    lastInput = millis();
 
    if (digitalRead(LED)) {
      digitalWrite(LED, LOW);    //Turn LED on if it is OFF.  With a high side transistor switch, LOW is ON and HIGH is OFF\
      getTouchCoords();
    }
    else if (!inSystemMenu) {  //In main menu

      unsavedChanges = true;
      getTouchCoords();

      //Check for bad coordinates (usually -1 for x and -1 for y)
      if ((iButtonX<0) || (iButtonY<0)) {}
      //Check temp up
      else if ((iButtonX>250) && (iButtonY>50) && (iButtonY<100) && iSystem_temp) changeSetTemp(TEMP_MOVE);
      //Check temp down
      else if ((iButtonX>250) && (iButtonY>100) && (iButtonY<150) && iSystem_temp) changeSetTemp(-TEMP_MOVE);
      //Check fan
      else if ((iButtonX>210) && (iButtonY<50)) changeFan();
      //Check hold
      else if ((iButtonX>150) && (iButtonY>170)) changeHold();
      //Check whether to save
      else if ((iButtonX<130) && (iButtonY>170) && (unsavedChanges)) saveChanges();
      //Check system
      else if ((iButtonX<200) && (iButtonY<50)) {
        systemDisplay();
        inSystemMenu = true;
      }
    }
    else {  //In system menu
      unsavedChanges = true;
      changeSystem();
      mainDisplay();
      inSystemMenu = false;
    }
  } else {  //No myTouch.dataAvailable()
    if ((unsavedChanges) && (millis() - lastInput) > TOUCH_TIMEOUT) {  //User has walked away from input
      resetTempVariables();
      inSystemMenu = false;
      unsavedChanges = false;
      mainDisplay();
    }
    if ((millis()-lastInput>TURN_OFF_INTERVAL) && !digitalRead(LED)) digitalWrite(LED, HIGH);  //Turn LED OFF if it is ON. With a high side transistor switch, LOW is ON and HIGH is OFF
  }
}


void getTouchCoords() 
{
  int x, y;
  int iXVals[SAM];
  int iYVals[SAM];
 
  //Retrieve up to SAM coordinate pairs
  int i = 0;
  bool full = false;

  while (myTouch.dataAvailable()) {

    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY(); 

    if (x < 0 || x >= 324 || y < 0 || y >= 239) {
      continue;
    }
    
//    x += 5; // calibration?

    if (i < SAM) {
      iXVals[i] = x;
      iYVals[i] = y;
      i++;
    }
    else {
      full = true;
      i = 0;
    }
    delay(5);
  }

  if (full) i = SAM;

  //Sort the values obtained and use the median
  if (i < 1) {
    iButtonX = -1;
    iButtonY = -1;
    Serial.println("bad read x, y");
  }
  else {
    bubsort(iXVals, i);
    if (i % 2 == 0) iButtonX = (iXVals[i/2-1] + iXVals[i/2])/2;
    else iButtonX = iXVals[i/2];
    bubsort(iYVals, i);
    if (i % 2 == 0) iButtonY = (iYVals[i/2-1] + iYVals[i/2])/2;
    else iButtonY = iYVals[i/2];
    for (int j = 0; j < i; j++) {
      Serial.print(iXVals[j]); Serial.print("   "); Serial.println(iYVals[j]);
    }
    Serial.print("x-value: "); Serial.println(iButtonX);
    Serial.print("y-value: "); Serial.println(iButtonY);
    Serial.println("-------------------------"); Serial.println();
  }
}


void bubsort(int v[], size_t n)
{
  bool sorted;
  do {
    sorted = true;
    for (int i = 1; i < n; i++) {
      if (v[i-1] > v[i]) {
	int temp = v[i];
	v[i] = v[i-1];
	v[i-1] = temp;
	sorted = false;
      } 
    }
  } while (!sorted);
}


// controls

void changeSetTemp(int iMove) 
{ 
  //Erase the old temp
  displaySetTemp(ILI9341_BLACK);
  //Increment the temp
  setpoint_temp += iMove;
  //Write the new temp 
  if (iSystem_temp == 1) displaySetTemp(ILI9341_BLUE);
  else displaySetTemp(ILI9341_RED);
  //Display the hold setting so user can set a hold on new temperature
  if (iHold_temp==0) {
    displayHold(ILI9341_BLACK);
    iHold_temp = 1;  //If hold was Program, set to Temporary
  }
  displayHold(ILI9341_WHITE);
  displaySave(ILI9341_WHITE);
}


void changeFan()
{  
  //Erase old fan setting
  displayFan(ILI9341_BLACK);
  //Change the fan setting
  fan_on_temp = !fan_on_temp;
  //Write the new setting
  displayFan(ILI9341_WHITE);
  displaySave(ILI9341_WHITE);
}


void changeHold()
{  
  //Erase old Hold setting
  displayHold(ILI9341_BLACK);
  //Change the hold setting
  iHold_temp += 1;
  if (iHold_temp == 3) iHold_temp = 0;
  //Write the new setting
  displayHold(ILI9341_WHITE);
  displaySave(ILI9341_WHITE);
}

void changeSystem() 
{  
  const int menundx = 0;
  const int col1 = ILI9341_WHITE; // normal color
  const int col2 = ILI9341_GREEN; // highlight color

  int prev = -1, last = -1;
  unsigned prevcount = 0, lastcount = 0;
  int start_iSystem_temp = iSystem_temp;

  //Read the user selection
  while (myTouch.dataAvailable()) {  //Wait until user ends press

    myTouch.read();
    iButtonX = myTouch.getX();
    iButtonY = myTouch.getY(); 

    if (iButtonY < 0) {
    }
    else if (iButtonY < 48) {
      iSystem_temp = 0;
      iHold_temp = 0;
    }
    else if (iButtonY < 96) {
      iSystem_temp = 1;
    }
    else if (iButtonY < 144) { 
      iSystem_temp = 2;
    }
    else if (iButtonY < 194) {
      iSystem_temp = 3;
    }
    else {
      iSystem_temp = 4;
    }

    if (iSystem_temp != last) {
      drawrectndx(menundx,iSystem_temp,col2); // highlight this selection
      if (last >= 0) drawrectndx(menundx,last,col1); // unhighlight prev selection
      prev = last;
      last = iSystem_temp;
      prevcount = lastcount;
      lastcount = 0;   
    }
    else {
      lastcount++;
      if (lastcount >= 100) break; // return after lingering 1s on same item
    }
    delay(10);
  }
  
  if (prevcount > lastcount && prev >= 0) iSystem_temp = prev; // handle switch bounce on press release
  
  //Update the setpoint_temp variable to reflect new iSystem_temp setting and the the hold variable back to Program
  if (((iSystem_temp == 1) && (start_iSystem_temp != 1)) || ((iSystem_temp > 1) && (start_iSystem_temp <=1))){  
    //Changed to cooling from something else or to heating from something else
    get_new_setpoint_temp();
//    displayHold(ILI9341_BLACK);
    iHold_temp = 0;  //Set iHold_temp to "Program"
//    displayHold(ILI9341_WHITE);
  }
}

void resetTempVariables() {
  iSystem_temp = iSystem;
  iHold_temp = iHold;
  fan_on_temp = fan_on;
  setpoint_temp = setpoint;
  rfx.fpf("*****End of resetTempVariables. iSystem_temp = %u\n", iSystem_temp);
}

void setSystemVariable(){

  if (manual_override == 0) {
    iSystem = 0; //OFF
    heating_mode = 1;  //Change to heat so reversing valve is off
  }
  else {
    switch (heating_mode) {
      case 0:
        iSystem = 1;  //COOL
        break;
      case 1:
        iSystem = 2;  //HEAT
        break;
      case 2:
        iSystem = 4;  //EM HEAT
        break;
      case 3:  //This will need to be modified once turn on auxiliary heat automatically as needed
        iSystem = 3;  //AUX HEAT
        break;
    }
  }
}

void saveChanges() {

  if (iSystem_temp != iSystem) {
    iSystem = iSystem_temp;
    manual_override = 0xFF;
    switch (iSystem) {
      case 0:  //OFF
        manual_override = 0;
        heating_mode = 1;
        break;
      case 1:  //  COOL
        heating_mode = 0;
        break;
      case 2:  //  HEAT
        heating_mode = 1;
        break;
      case 3:  //  AUX HEAT
        heating_mode = 3;
        break;
      case 4:  //  EM HEAT
        heating_mode = 2;
        break;
    }
  }
  fan_on = fan_on_temp;
  setpoint = setpoint_temp;
  iHold = iHold_temp;
  unsavedChanges = false;
  displaySave(ILI9341_BLACK);
  displayRelayState();
  write_eeprom_values();

  //Send setting to the server to be stored in the database and the server thermostat object
  //Server expects a message in the usual format (!TSU) with the message part consisting of instructions separated by commas (e.g. A,s=210,f=0)
  //For override, instruction looks like s=210T or s=210H or s=210N, or s=N
  //Will need to adjust message and server program to server know not to relay info back to thermostat in those cases

  char* hold;

  if (iHold==2) hold = "H";  //The hold value is set to ignore the program
  else if (iHold == 1) hold = "T";
  else hold = "N";
  
  char tstat_status[100];
  char temp_itoa[4];
  if (manual_override == 0xFF) strcpy(tstat_status, "A");
  else {
    itoa(manual_override, temp_itoa, 10);
    strcpy(tstat_status, temp_itoa);
  }

  strcat(tstat_status, ",h="); 
  itoa(heating_mode, temp_itoa, 10);
  strcat(tstat_status, temp_itoa);

  strcat(tstat_status, ",f=");
  itoa(fan_on, temp_itoa, 10);
  strcat(tstat_status, temp_itoa);

  strcat(tstat_status, ",s="); 
  itoa((int)((float)setpoint/10.0+0.5), temp_itoa, 10);
  strcat(tstat_status, temp_itoa);
  strcat(tstat_status, hold);

  SEND_FMT_LOG_MSG(MASTER_PORT,"!TSU/4321/%u/%s#", BOARD_ID, tstat_status);

}




// displays

void mainDisplay() 
{
  uint16_t iTempColor;  //Color to use for temperatures
  
  tft.fillScreen(ILI9341_BLACK);
  if (iSystem_temp == 0) iTempColor = ILI9341_WHITE;
  else if (iSystem_temp == 1) iTempColor = ILI9341_BLUE;
  else iTempColor = ILI9341_RED;
//  displayRoomTemp(iTempColor);
  displayRoomTemp(displayTemp, true);
  displayRoomTemp((int)((float)curr_Temp/10.0+0.5), false);
  if (iSystem_temp) displaySetTemp(iTempColor);
  displayLabels();
//  if (iHold) displayHold(ILI9341_WHITE);
  if (iSystem_temp) displayHold(ILI9341_WHITE);  //No hold display if system is OFF
  displaySystem(ILI9341_WHITE);
  displayFan(ILI9341_WHITE);
  if (unsavedChanges) displaySave(ILI9341_WHITE);
//  else displaySave(ILI9341_BLACK);
  else displayRelayState();
}


void displayRoomTemp(uint16_t roomTemp, bool bErase) 
{  
//  int iRoomTemp = (int)((float)curr_Temp/10.0+0.5);
  if (bErase) tft.setTextColor(ILI9341_BLACK);
  else if (iSystem_temp==0) tft.setTextColor(ILI9341_WHITE);
  else if (iSystem_temp==1) tft.setTextColor(ILI9341_BLUE);
  else tft.setTextColor(ILI9341_RED);
//  tft.setTextColor(iColor);
  tft.setTextSize(10);
  tft.setCursor(5, 60);
  tft.print(roomTemp);
  tft.setCursor(41,156);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Room");  
  displayTemp = roomTemp;
}


void displaySetTemp(uint16_t iColor) 
{
  tft.setTextColor(iColor);
  tft.setTextSize(10);
  tft.setCursor(160, 60);
  tft.print((int)((float)setpoint_temp/10.0 + 0.5));
  if (iColor != ILI9341_BLACK) iColor = ILI9341_WHITE;
  tft.fillTriangle(295, 60, 280, 95, 310, 95, iColor);
  tft.fillTriangle(295, 140, 280, 105, 310, 105, iColor);
  tft.setTextColor(iColor);
  tft.setCursor(202,156);
  tft.setTextSize(2);
  tft.print("Set");   
}  


void displayLabels() 
{ 
  //Print the various setting labels
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0,10);
  tft.print("System: ");

  tft.setCursor(215,10);
  tft.print("Fan: ");  
}


void displayHold(uint16_t iColor) 
{  
  tft.setTextColor(iColor);
  tft.setTextSize(3);
  tft.setCursor(150,200);
  if (iHold_temp == 0) tft.print("Program");
  else if (iHold_temp == 1) tft.print("Temporary");
  else tft.print("  Hold");  
//  if (iColor == ILI9341_BLACK) bViewHold = 0;
//  else bViewHold = 1;
}


void displaySystem(uint16_t iColor) 
{  
  tft.setCursor(90, 10);
  tft.setTextColor(iColor);
  tft.setTextSize(2);
  switch (iSystem_temp) {
    case 0:
      tft.print("OFF");
      break;
    case 1:
      tft.print("COOL");
      break;
    case 2:
      tft.print("HEAT");
      break;
    case 3:
      tft.print("AUX HEAT");
      break;
    case 4:
      tft.print("EM HEAT");
      break;
  }
}

void updateSystemDisplay() {

  //This updates the diplay of System variables when they are changed remotely instead of through the touchscreen
  if (iSystem != iSystem_temp) {
    displaySystem(ILI9341_BLACK);
    iSystem_temp = iSystem;
    rfx.fpf("*****In handle_serial_input. iSystem_temp = %u\n", iSystem_temp);
    displaySystem(ILI9341_WHITE);
    //Change in system means color of set temp may have to change as well or erase if system is OFF
    displaySetTemp(ILI9341_BLACK);
    if (iSystem_temp == 1) displaySetTemp(ILI9341_BLUE);
    else if (iSystem_temp > 1) displaySetTemp(ILI9341_RED);
  }
}

void displayFan(uint16_t iColor) 
{  
  tft.setTextColor(iColor);
  tft.setTextSize(2);
  tft.setCursor(270, 10);
  if (fan_on_temp) tft.print("ON");
  else tft.print("AUTO");
}

void displaySave(uint16_t iColor) 
{  
  tft.fillRect(0,200,70,24,ILI9341_BLACK);
  tft.setTextColor(iColor);
  tft.setTextSize(3);
  tft.setCursor(0,200);
  tft.print("SAVE");
}


void displayRelayState()
{
  static uint8_t old_relay_state=0;

  if (!unsavedChanges) {  
    tft.setTextSize(3);
    tft.setCursor(0,200);
    tft.setTextColor(ILI9341_BLACK);
    for (int i=0; i<RELAYS; i++) {
      uint8_t mask = 1 << i;
      tft.print((old_relay_state & mask) >> i);
    }
//    tft.print(old_relay_state, BIN);
    tft.setCursor(0,200);
    tft.setTextColor(ILI9341_WHITE);
    for (int i=0; i<RELAYS; i++) {
      uint8_t mask = 1 << i;
      tft.print((relay_state & mask) >> i);
    }
//    tft.print(relay_state, BIN);
    old_relay_state = relay_state;
  }
}


/*
--------------------------------------------------------------------
Below this line are the system display functions
--------------------------------------------------------------------
*/

void systemDisplay() 
{
  const int col = ILI9341_WHITE;
  const int menundx = 0;

  tft.fillScreen(ILI9341_BLACK);

  //Print out the system display options
  tft.setTextSize(4);
  tft.setTextColor(col);
  drawrectndx(menundx,0,col);
  tft.setCursor(10,10);
  tft.print("OFF       ");
  drawrectndx(menundx,1,col);
  tft.setCursor(10,58);
  tft.print("COOL      ");
  drawrectndx(menundx,2,col);
  tft.setCursor(10,106);
  tft.print("HEAT      ");
  drawrectndx(menundx,3,col);
  tft.setCursor(10,154);
  tft.print("AUX HEAT  ");
  drawrectndx(menundx,4,col);
  tft.setCursor(10,202);
  tft.print("EM HEAT   ");
}

void drawrectndx(int menundx, int itemndx, int col)
{
  if (menundx == 0) {
    if (itemndx == 0) {
      tft.drawRoundRect(0,6,240,36,10,col);
    }
    else if (itemndx == 1) {
      tft.drawRoundRect(0,54,240,36,10,col);
    }
    else if (itemndx == 2) {
      tft.drawRoundRect(0,102,240,36,10,col);
    }
    else if (itemndx == 3) {
      tft.drawRoundRect(0,150,240,36,10,col);
    }
    else if (itemndx == 4) {
      tft.drawRoundRect(0,198,240,36,10,col);
    }
  }
}


