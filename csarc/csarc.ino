/* csarc allows you to do do a couple things with newline-terminated commands:
 *  
 *  set a constant color value: ------------- "V#FFFFFF"
 *  make gradient pulses between two colors:- "G#000000:#FFFFFF:pulseTime"
 *    \--loop gradient between two colors: -- "L#000000:#FFFFFF:pulseTime"
 *    \--smooth loop between two colors: ---- "S#000000:#FFFFFF:pulseTime
 *  start flashing two colors --------------- "F#000000:#FFFFFF:pulseTime"
 *  
 *  To use, toggle GROVE_DRIVER or set the color pins to match your hardware.
 *  You can toggle a handy debug mode with "d".
 */
// TO DO: cut off timeout gradient when activity resumes
//        implement verbosity levels
//        evaluate memory gain from using #define for some constants

#include <stdarg.h>
#include <Wire.h>

// -------------  LED SETUP -------------
const boolean GROVE_DRIVER = false; // Grove LED driver or DFRobot shield?

#include <RGBdriver.h> // Grove driver
const int CLK = 2;
const int DIO = 3;
RGBdriver Driver(CLK,DIO);

const int R_PIN = 10; // pins for directly
const int G_PIN = 11; // driven setup or
const int B_PIN =  9; // DFRobot shield
const int EXTRA_GND = 7; // <- for wiring convenience

// --------- CONSTANTS + CONFIG ---------
const int   OFF[] = {0,   0,   0 };
const int   RED[] = {255, 0,   0 };
const int GREEN[] = {0,   255, 0 };
const int  BLUE[] = {0,   0,   255};
const int WHITE[] = {255, 255, 255};
const int BAUDRATE = 19200;
const int SERIAL_BUFFER_LENGTH = 32;  //leaves 15 characters for pulseTime
const int SERIAL_CYCLE_LENGTH_MS = 70;
const int TIMEOUT_S = 600;  //seconds of no activity before beginning default pattern
const boolean TIMEOUT_ENABLED = false; // timeout is buggy
const float numdivs = 60.0; //tune this to adjust gradient smoothness

// ---------- GLOBAL VARIABLES ----------
boolean debugMode = false;
char inputBuffer[SERIAL_BUFFER_LENGTH];
boolean recentActivity = false;

int red,green,blue;
int* rgb[4] = {&red, &green, &blue, 0};

int gradcol1[3], gradcol2[3];
float gradPerc, gradPercPerTick, gradDelayPerTick_ms;
boolean gradientLoop = false, gradientReversingMode = true;

int flashcol1[3], flashcol2[3];
int flashTimePerColor_ms;
boolean flashLoop = true, flashingFirstColor = false;

// -------------- THREADS ---------------
#include <TimedAction.h>
TimedAction   serialThread = TimedAction(0, SERIAL_CYCLE_LENGTH_MS, &processSerialIn);
TimedAction gradientThread = TimedAction(0,  &gradientStep); // initially disabled in setup()
TimedAction    flashThread = TimedAction(0, flashTimePerColor_ms, &flash); // initially disabled in setup()
TimedAction  timeoutThread = TimedAction(TIMEOUT_S * 1000, TIMEOUT_S * 1000,  &timeoutAction);

void setup() {
  if (!GROVE_DRIVER) {
    pinMode(R_PIN,OUTPUT);
    pinMode(G_PIN,OUTPUT);
    pinMode(B_PIN,OUTPUT);
    pinMode(EXTRA_GND,OUTPUT);
  }
  gradientThread.disable();
  flashThread.disable();
  if (!TIMEOUT_ENABLED) { timeoutThread.disable(); }
  Serial.setTimeout(SERIAL_CYCLE_LENGTH_MS/7); //should be substantially less than serialThread's execution window
  Serial.begin(BAUDRATE);
  quickCycle(150); // power-up notification
  gradientPulseSetup(RED,OFF,1.5);
  gradientLoop = true;
  gradientReversingMode = true;
  Wire.begin(1); // slave device, address 1
  Wire.onReceive(processI2CIn); //I guess this'll run between loop()s when we get data; hopefully it won't actually interrupt anything...
}

void loop() {  
  serialThread.check();
  gradientThread.check();
  flashThread.check();
  timeoutThread.check();
}

void processI2CIn (int numBytesRead) {
  for (int x=0; x<numBytesRead; x++) {
    inputBuffer[x] = Wire.read();
  }
  recentActivity = true;
  switch (inputBuffer[0]) {
    case 'd': { toggleDebug(); break; }
    case 'V': { processColor(); break; } // set value specified in V#FFFFFF input
    case 'S' : { gradientLoop = true; gradientReversingMode = true;  processGradient(); break;}
    case 'L' : { gradientLoop = true; gradientReversingMode = false;  processGradient(); break;}
    case 'G' : { gradientLoop = false; processGradient(); break;}
    case 'F' : { processFlash(); break; }
  }
  if (debugMode) {
    processDebugIn();
  }
  clearBuffer();
}


void processSerialIn() {
  Serial.readBytesUntil('\n',inputBuffer,SERIAL_BUFFER_LENGTH);
  if (inputBuffer[0]!=0) {
    recentActivity = true;
//    if (debugMode) { Serial.print("|");
//                     for (int x=0; x<10; x++) { Serial.print(inputBuffer[x]); }
//                     Serial.print("|"); }
    switch (inputBuffer[0]) {
      case 'd': { toggleDebug(); break; }
      case 'V': { processColor(); break; } // set value specified in V#FFFFFF input
      case 'S' : { gradientLoop = true; gradientReversingMode = true;  processGradient(); break;}
      case 'L' : { gradientLoop = true; gradientReversingMode = false;  processGradient(); break;}
      case 'G' : { gradientLoop = false; processGradient(); break;}
      case 'F' : { processFlash(); break; }
    }
    if (debugMode) {
      processDebugIn();
    }
    clearBuffer();
  } else {
    if (debugMode) { Serial.println("No serial input."); }
    recentActivity = false;
  }
}

void processDebugIn() {
      switch(inputBuffer[0]) {
        case '0' : {
          setValue(0,0,0);
          Serial.println("LEDs turned off.");
          break; }
        case '1' : {
          setValue(255,255,255);
          Serial.println("LEDs turned on.");
          break; }
        case 'r' : {
          setValue(255,0,0);
          Serial.println("Color set to red.");
          break;
        }
        case 'g' : {
          setValue(0,255,0);
          Serial.println("Color set to green.");
          break;
        }
        case 'b' : {
          setValue(0,0,255);
          Serial.println("Color set to blue.");
          break;
        }
        case '?' : { // display contents of *rgb: three colors and a null pointer
          Serial.println("Current color values:");
          for (int i=0; i<3; i++) {
            Serial.print(*rgb[i]);
            Serial.print(',');
          }
          Serial.println(*rgb[3]);
        }
      }
}

void processColor() {
  gradientThread.disable(); // cut off anything else
  flashThread.disable();
  gradientLoop = false;
  char colorInput[8];
  arrayCopy(colorInput,&inputBuffer[1],7);
  if (debugMode) {
    Serial.print("Serial input:"); 
    Serial.println(colorInput);  }
  decodeHex(colorInput,rgb);
  outputColor(rgb);
  printColors();
  if (debugMode) {
    Serial.println("Color set.  Results:");
    Serial.print(   "red: ");  Serial.println(   red, DEC);
    Serial.print( "green: ");  Serial.println( green, DEC);
    Serial.print(  "blue: ");  Serial.println(  blue, DEC);
    Serial.println(""); 
  }
}

void processGradient() {
// run gradient pulse between a and b over a length of time; "G#000000:#123456:time" input
  if ((inputBuffer[1]!='#' || inputBuffer[9]!='#')) {
    Serial.println("Error: invalid gradient request");
    Serial.println(inputBuffer);
  } else {
    char hex1[8], hex2[8];
    char timev[8];
    for (int x = 0; x<7; x++) {
      hex1[x] = inputBuffer[1+x];
      hex2[x] = inputBuffer[9+x];
      timev[x] = inputBuffer[17+x];
    }
    int col1[4],col2[4];
    int *col1p[4] = {&col1[0],&col1[1],&col1[2],0};
    int *col2p[4] = {&col2[0],&col2[1],&col2[2],0};

    if (debugMode) {
      Serial.println("input char[]s");
      Serial.println(hex1);
      Serial.println(hex2);
      Serial.println(timev); }

    
    decodeHex(hex1,col1p);
    decodeHex(hex2,col2p);
    float timeVal = atof(timev); //use strtol or sscanf instead
    gradientPulseSetup(col1,col2,timeVal);

    if (debugMode) {
      Serial.println("col1: ");
      Serial.print(col1[0]); Serial.print(" "); Serial.print(col1[1]); Serial.print(" "); Serial.println(col1[2]);
      Serial.println("col2: ");
      Serial.print(col2[0]); Serial.print(" "); Serial.print(col2[1]); Serial.print(" "); Serial.println(col2[2]);
      Serial.print("timeVal: ");
      Serial.println(timeVal);
    }
  }
}

void processFlash() {
// start flashing between A and B every timeval seconds: "F#000000:#123456:timeVal"
  if ((inputBuffer[1]!='#' || inputBuffer[9]!='#')) {
    Serial.println("Error: invalid flash request");
    Serial.println(inputBuffer);
  } else {
    char hex1[8], hex2[8];
    char timev[8];
    for (int x = 0; x<7; x++) {
      hex1[x] = inputBuffer[1+x];
      hex2[x] = inputBuffer[9+x];
      timev[x] = inputBuffer[17+x];
    }
    int col1[4],col2[4];
    int *col1p[4] = {&col1[0],&col1[1],&col1[2],0};
    int *col2p[4] = {&col2[0],&col2[1],&col2[2],0};    
    decodeHex(hex1,col1p);
    decodeHex(hex2,col2p);
    float timeVal = atof(timev); //use strtol or sscanf instead
    flashSetup(col1,col2,timeVal);
  }
}

void flashSetup (const int col1[3],const int col2[3], float timev) {
  if (redundantFlashCommand(col1,col2, timev*1000)) { return;}
  gradientThread.disable();
  flashTimePerColor_ms = (float) timev*1000;
  for (int x=0; x<3; x++) {
    flashcol1[x]=col1[x];
    flashcol2[x]=col2[x];
  }
  
  flashThread.setInterval(flashTimePerColor_ms);
  flashThread.reset();
  flashThread.enable();
  if (debugMode) {
    Serial.println(timev);
    Serial.println("Flash setup done; flash thread enabled.");
  }
}

void flash() {
//  if(debugMode) { Serial.print("Flash: ");}
  outputColor( flashingFirstColor ? flashcol1 : flashcol2 );
  flashingFirstColor = !flashingFirstColor;
  // refactor to be able to use printColors()
//  if (debugMode) {Serial.println(millis());}
}

void timeoutAction() {
  if (debugMode) { Serial.println("Processing timeout."); }
  if (!recentActivity) {
    Serial.print("No recent activity for "); Serial.print(TIMEOUT_S); Serial.println(" seconds.");
    // maybe timeout and timeout behavior should be configurable over serial.
    // low priority, bc this will mostly be running before serial comms have
    // been established.
    gradientLoop = false;
    gradientPulseSetup(OFF, WHITE, TIMEOUT_S);
  } 
}

void gradientPulseSetup (const int col1[3],const int col2[3], float timev) {
  flashThread.disable();
  gradDelayPerTick_ms = (float) timev*1000/numdivs;
  gradPercPerTick = (float) 1/numdivs;
  gradPerc = 0;
  if (debugMode) {
    Serial.println("---gradientPulse---");
    Serial.println("col1,col2:");
    for (int i=0; i<3;i++) {
      Serial.print(col1[i]);
      Serial.print(' '); 
    }
    Serial.println();
    for (int i=0; i<3;i++) {
      Serial.print(col2[i]);
      Serial.print(' '); 
    }
    Serial.print("timev = ");
    Serial.println(timev);
    Serial.print("numdivs = ");
    Serial.println(numdivs);
    Serial.print("gradDelayPerTick_ms = ");
    Serial.println(gradDelayPerTick_ms);
    Serial.print("gradPercPerTick = ");
    Serial.println(gradPercPerTick);
    Serial.println();
  }

  for (int x=0; x<3; x++) {
    gradcol1[x]=col1[x];
    gradcol2[x]=col2[x];
  }
  
  gradientThread.setInterval(gradDelayPerTick_ms);
  gradientThread.reset();
  gradientThread.enable();
  if (debugMode) {
    Serial.println("Gradient setup done, gradient thread enabled.");
  }
}



void gradientStep() {
  if (debugMode) { Serial.println("Gradient processing."); }
  if (gradPerc <= 1.0) {
    Serial.print(gradPerc);
    Serial.println("/1");
    gradientValue(gradcol1, gradcol2, gradPerc, rgb);
    outputColor(rgb);
    gradPerc+=gradPercPerTick;
  }
  else {
    if (gradientLoop) {
      gradPerc = 0;
      if (gradientReversingMode) {
        int temp[3]; //swap 1 and 2
        arrayCopy(temp,gradcol1,3);
        arrayCopy(gradcol1,gradcol2,3);
        arrayCopy(gradcol2,temp,3);
      }
      if (debugMode) { Serial.println("Looping gradient."); }
    } else {
      if (debugMode) { Serial.println("Halting gradient."); }
      gradientThread.disable();
    }
  }
}

void gradientValue(int col0[3], int col1[3], float percentage, int *output[3]) {
  *output[0] = col1[0]*percentage + col0[0]*(1-percentage);
  *output[1] = col1[1]*percentage + col0[1]*(1-percentage);
  *output[2] = col1[2]*percentage + col0[2]*(1-percentage);
}


void decodeHex( char hexColor[], int * output[] ) { //in: #FFFFFF\0
  unsigned long number = strtol( &hexColor[1], NULL, 16);
  *output[0] = number >> 16;      //first
  *output[1] = number >> 8 & 0xFF;//mid
  *output[2] = number & 0xFF;     //last
  if (debugMode) {
    Serial.println("---decodeHex debugging---");
    Serial.print("hexColor[]: ");
    Serial.println(hexColor);
    Serial.print("unsigned long number: ");
    Serial.println(number);
    Serial.println("*output[0-4]: ");
    Serial.println(*output[0]);
    Serial.println(*output[1]);
    Serial.println(*output[2]);
    Serial.println(*output[4]);
    Serial.println("--------------------------");
    Serial.println();
  }    
}

void setValue ( int r, int g, int b) {
  gradientThread.disable(); // cut off anything else
  flashThread.disable();
  gradientLoop = false;  
  red=r; green=g; blue=b;
  outputColor(rgb);
}

void outputColor( int r, int g, int b) {

  if (GROVE_DRIVER) {
    Driver.begin();
    Driver.SetColor(r, g, b);
    Driver.end();
  } else {
    analogWrite(R_PIN,r);
    analogWrite(G_PIN,g);
    analogWrite(B_PIN,b);
  }
}



void outputColor( int *color[]) {  
  if (debugMode) {
  
    Serial.println(); // for some reason a lot of this output
    Serial.println("---Setting output:---"); // doesn't show up
    Serial.println(*color[0]);
    Serial.println(*color[1]);
    Serial.println(*color[2]);
    Serial.println(*color[3]);
    Serial.println("---------------------");
  }
    outputColor(*color[0],*color[1],*color[2]);
}

void outputColor( int color[]) {  
  if (debugMode) {
  
    Serial.println(); // for some reason a lot of this output
    Serial.println("---Setting output:---"); // doesn't show up
    Serial.println(color[0]);
    Serial.println(color[1]);
    Serial.println(color[2]);
    Serial.println(color[3]);
    Serial.println("---------------------");
  }
    outputColor(color[0],color[1],color[2]);
}

void clearBuffer() {
  //optimized for a mostly empty buffer
  char *ptr = &inputBuffer[0];
  if(debugMode) {
    Serial.print("Buffer: [");
    while (ptr<( &inputBuffer[0]+SERIAL_BUFFER_LENGTH ) && *ptr!=0) {
      Serial.print(*ptr);
      *ptr = 0;
      ptr++;
    }
    Serial.print(*ptr);
    Serial.println( "] wiped.");
  } else {
    while (ptr<( &inputBuffer[0]+SERIAL_BUFFER_LENGTH ) && *ptr!=0) {
      *ptr = 0;
      ptr++;
    }
  }
  *ptr = 0;
}

void rgbFlash(int baseTime) {
  setValue(255,0,0);
  delay(baseTime);
  setValue(0,255,0);
  delay(baseTime);
  setValue(0,0,255);
  delay(baseTime);
  setValue(255,255,255);
  delay(baseTime);
  setValue(0,0,0);
}

void quickCycle(int totalTime_ms) {
/*  |-----------1/2 R/G/B/W pulse-----------|-----1/4 pulse-----|--1/8 pulse--|-1/8 grad->off-| */
  int one32th = totalTime_ms / 32;
  rgbFlash(4*one32th);  // 1/2 length * 1/4 rgb = 1/8  = 4*1/32
  rgbFlash(2*one32th);  // 1/4 length * 1/4 rgb = 1/16 = 2*1/32
  rgbFlash(  one32th);  // 1/8 length * 1/4 rgb = 1/32 =   1/32
  gradientPulseSetup(WHITE,OFF, one32th*4000); // 1/8th length = 4*1/32; convert to seconds
}


void toggleDebug() {
  debugMode = !debugMode;
  Serial.print("Debug mode ");
  if(debugMode) {
    Serial.println("ON" );  }
  else {
    Serial.println("OFF");  }
}

void printColors() {  
  Serial.print('#');
  if (red<=16) {Serial.print('0');}
  Serial.print(red, HEX);
  if (green<=16) {Serial.print('0');}
  Serial.print(green, HEX);
  if (blue<=16) {Serial.print('0');}
  Serial.println(blue, HEX);
}

void arrayCopy( char recipient[], char donor[], int len) {
  for (int x=0; x<len; x++) { // apparently takes <1ms, so whatever
    recipient[x]=donor[x];
  }
}

void arrayCopy( int recipient[], int donor[], int len) {
  for (int x=0; x<len; x++) { // apparently takes <1ms, so whatever
    recipient[x]=donor[x];
  }
}

boolean redundantFlashCommand(const int col1[], const int col2[], int fTime) {
  for (int x=0; x<3; x++) {
    if (flashcol1[x]!=col1[x]) {return false;}
    if (flashcol2[x]!=col2[x]) {return false;}
  }
  if (flashTimePerColor_ms != fTime) {return false;}
  return true;
}

