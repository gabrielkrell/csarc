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
//        store commands in structs for easy comparisons and transport
//        implement modeswitching between I2C/serial if the default Wire interrupts are messing things up
//        improve mode selection
//        gradients should have a constant tick speed and a different percentage change to improve behavior of
//          long gradients

#include <stdarg.h>
#include <Wire.h>
#include "Typedefs.h"
#include "GradientState.h"
#include "GradientCommand.h"
typedef int milliseconds;
typedef float seconds;

struct GradientStateStruct {
  const GradientCommand * command;
  float percent;
  float percentPerTick;
  milliseconds delayPerTick;
  boolean colors_reversed;
};


struct FlashCommand {
  int col1[3];
  int col2[3];
  int pulselen;
};

// -------------USER CONFIG -------------
const seconds TIMEOUT_S = 600;
const boolean TIMEOUT_ENABLED = false;  // timeout is buggy
const float GRAD_TOTAL_TICKS = 60.0;    //tune this to adjust gradient smoothness
const int BAUDRATE = 19200;
const int SERIAL_BUFFER_LENGTH = 32;    //leaves 15 chars for last val in longest command
const milliseconds SERIAL_CYCLE_TIME_MS = 70;
const milliseconds SERIAL_INPUT_WAIT_MS = 10; //should be substantially less than serialThread's execution window

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

// -------------- CONSTANTS -------------
const int   OFF[3] = {0,   0,   0 };
const int   RED[3] = {255, 0,   0 };
const int GREEN[3] = {0,   255, 0 };
const int  BLUE[3] = {0,   0,   255};
const int WHITE[3] = {255, 255, 255};
const GradientCommand RED_ALLIANCE = GradientCommand( RED, OFF, SMOOTH_LOOP, 2);
const GradientCommand BLUE_ALLIANCE = GradientCommand( BLUE, OFF, SMOOTH_LOOP, 2);

// ---------- GLOBAL VARIABLES ----------
boolean debugMode = true;
char inputBuffer[SERIAL_BUFFER_LENGTH];
boolean recentActivity = false;

int red,green,blue;
int * const rgb[4] = {&red, &green, &blue, 0};
// array 4 of const pointer to int


GradientCommand currentGradCommand;
GradientStateStruct currentGradState;

int flashcol1[3], flashcol2[3];
int flashTimePerColor_ms;
boolean flashingFirstColor = false;
// -------------- THREADS ---------------
#include <TimedAction.h>
TimedAction   serialThread = TimedAction(0, SERIAL_CYCLE_TIME_MS, &processSerialIn);
TimedAction gradientThread = TimedAction(0,  &gradientStep); // initially disabled in setup()
TimedAction    flashThread = TimedAction(0, flashTimePerColor_ms, &flash); // initially disabled in setup()
TimedAction  timeoutThread = TimedAction(TIMEOUT_S * 1000, TIMEOUT_S * 1000,  &timeoutAction);

void setup() {
  currentGradState.command = &currentGradCommand;
  setupLEDs();
  gradientThread.disable();
     flashThread.disable();
   timeoutThread.setEnabled(TIMEOUT_ENABLED);
  Serial.setTimeout(SERIAL_INPUT_WAIT_MS);
  Serial.begin(BAUDRATE);
//  quickCycle(150); // power-up notification

  currentGradState.command = &BLUE_ALLIANCE;
  gradientPulseSetup( &currentGradState, currentGradState.command);
  
  
  Wire.begin(1); // slave device, address 1
  Wire.onReceive(processI2CIn); //I guess this'll run between loop()s when we get data; hopefully it won't actually interrupt anything...
}

void loop() {  
    serialThread.check();
  gradientThread.check();
     flashThread.check();
   timeoutThread.check();
}

void setupLEDs() {
  if (!GROVE_DRIVER) {
    pinMode(R_PIN,OUTPUT);
    pinMode(G_PIN,OUTPUT);
    pinMode(B_PIN,OUTPUT);
    pinMode(EXTRA_GND,OUTPUT);
  }
}

void processI2CIn (int numBytesRead) {
  for (int x=0; x<numBytesRead; x++) {
    inputBuffer[x] = Wire.read();
  }
  recentActivity = true;
  switch (inputBuffer[0]) {
    case 'd' : { toggleDebug(); break; }
    case 'V' : { processColorFromBuffer(); break; }
    case 'S' : { processGradient(SMOOTH_LOOP);    break;}
    case 'L' : { processGradient(FWD_LOOP);       break;}
    case 'G' : { processGradient(SINGLE_FWD_LOOP);break;}
    case 'F' : { processFlash();                  break;}
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
    switch (inputBuffer[0]) {
      case 'd' : { toggleDebug(); break; }
      case 'V' : { processColorFromBuffer(); break; }
      case 'S' : { processGradient(SMOOTH_LOOP);    break;}
      case 'L' : { processGradient(FWD_LOOP);       break;}
      case 'G' : { processGradient(SINGLE_FWD_LOOP);break;}
      case 'F' : { processFlash();                  break;}
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
          setSolidColor(OFF);
          Serial.println("LEDs turned off.");
          break; }
        case '1' : {
          setSolidColor(255,255,255);
          Serial.println("LEDs turned on.");
          break; }
        case 'r' : {
          setSolidColor(255,0,0);
          Serial.println("Color set to red.");
          break;
        }
        case 'g' : {
          setSolidColor(0,255,0);
          Serial.println("Color set to green.");
          break;
        }
        case 'b' : {
          setSolidColor(0,0,255);
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

void processColorFromBuffer() {
  char colorInput[8];
  memcpy(colorInput,&inputBuffer[1],7);
  if (debugMode) {
    Serial.print("Serial input:"); 
    Serial.println(colorInput);
  }
  decodeHex(colorInput,rgb);
  showColor(rgb);
  printColor(rgb);
  if (debugMode) {
    Serial.println("Color set.  Results:");
    Serial.print(   "red: ");  Serial.println(   red, DEC);
    Serial.print( "green: ");  Serial.println( green, DEC);
    Serial.print(  "blue: ");  Serial.println(  blue, DEC);
    Serial.println(""); 
  }
}

void processGradient(GradientMode gm) {
// run gradient pulse between a and b over a given duration; "G#000000:#123456:time" input
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
    int col1[3],col2[3];
    int *const col1p[3] = {&col1[0],&col1[1],&col1[2]};
    int *const col2p[3] = {&col2[0],&col2[1],&col2[2]};

    if (debugMode) {
      Serial.println("input char[]s");
      Serial.println(hex1);
      Serial.println(hex2);
      Serial.println(timev); }

    decodeHex(hex1,col1p);
    decodeHex(hex2,col2p);
    seconds timeVal = atof(timev); //use strtol or sscanf instead

// so now we have col1, col2, seconds timeVal, and a mode from earlier.
    currentGradCommand = GradientCommand(col1,col2,gm,timeVal);
    currentGradState.command = &currentGradCommand;
    gradientPulseSetup(&currentGradState, &currentGradCommand);

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
    memcpy(hex1,&inputBuffer[1],7);
    memcpy(hex2,&inputBuffer[9],7);
    memcpy(timev,&inputBuffer[17],7);
    int col1[3],col2[3];    
    int *const col1p[3] = {&col1[0],&col1[1],&col1[2]};
    int *const col2p[3] = {&col2[0],&col2[1],&col2[2]};
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
  setValue( flashingFirstColor ? flashcol1 : flashcol2 );
  flashingFirstColor = !flashingFirstColor;
  // refactor to be able to use printColors()
//  if (debugMode) {Serial.println(millis());}
}

GradientCommand TIMEOUT_ACTION = GradientCommand(OFF, WHITE, SMOOTH_LOOP, TIMEOUT_S);
void timeoutAction() {
  if (debugMode) { Serial.println("Processing timeout."); }
  if (!recentActivity) {
    Serial.print("No recent activity for "); Serial.print(TIMEOUT_S); Serial.println(" seconds.");
    // Maybe we should be able to input a new TIMEOUT_ACTION over serial.  Low priority.
    gradientPulseSetup(&currentGradState, &TIMEOUT_ACTION);
  } 
}



void gradientPulseSetup( GradientStateStruct * gs, const GradientCommand * gc ) {
  gs->delayPerTick = (milliseconds) gc->pulselen*1000/GRAD_TOTAL_TICKS; // convert to millis
  gs->percentPerTick = (float) 1/GRAD_TOTAL_TICKS;
  gs->percent = 0;
  gradientThread.setInterval(gs->delayPerTick);

  
  flashThread.disable(); // this logic doesn't belong here
  gradientThread.reset();
  gradientThread.enable();
  if (debugMode) {
    Serial.println("Gradient setup done.  Gradient thread enabled.");
  }
}


void gradientStep() {
  if (debugMode) { Serial.println("Gradient processing."); }
  if (currentGradState.percent <= 1.0) {
    Serial.print(currentGradState.percent);
    Serial.println("/1");
    Serial.println("col1: "); printArray(currentGradCommand.col1,3);
    Serial.println("col2: "); printArray(currentGradCommand.col1,3);
    gradientValue(currentGradCommand.col1, currentGradCommand.col2, currentGradState.percent, rgb);
    showColor(rgb);
    currentGradState.percent+=currentGradState.percentPerTick;
  }
  else {
    switch (currentGradState.command->gradMode) {
      case SMOOTH_LOOP: {
        int temp[3]; //swap 1 and 2
        memcpy(temp,currentGradCommand.col1,3);
        memcpy(currentGradCommand.col1,currentGradCommand.col2,3);
        memcpy(currentGradCommand.col2,temp,3);
        currentGradState.percent = 0;

        
        if (debugMode) { Serial.println("Looping gradient."); }
        break;
      }
      case FWD_LOOP: {
        currentGradState.percent = 0;
        if (debugMode) { Serial.println("Looping gradient."); }
        break;
      }
      case SINGLE_FWD_LOOP: {
        if (debugMode) { Serial.println("Halting gradient."); }
        gradientThread.disable();
      }
      case NO_GRAD: {
        if (debugMode) { Serial.println("Halting gradient."); }
        gradientThread.disable();
      }
    }
  }
}

void gradientValue(const int col0[3], const int col1[3], const float percentage, int * const output[3]) {
  *output[0] = col1[0]*percentage + col0[0]*(1-percentage);
  *output[1] = col1[1]*percentage + col0[1]*(1-percentage);
  *output[2] = col1[2]*percentage + col0[2]*(1-percentage);
}


void decodeHex( const char hexColor[], int* const output[] ) {
  //const char hexColor[]
  //  int * const rgb[4] = {&red, &green, &blue, 0};
//// array 4 of const pointer to 
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
  red=r; green=g; blue=b;
  showColor(rgb);
}

void setValue ( const int color[]) {
  red=color[0]; green=color[1]; blue=color[2];
  showColor(rgb);
}

void setSolidColor ( int r, int g, int b) {
  gradientThread.disable();
  flashThread.disable();
  setValue(r,g,b);
}

void setSolidColor ( const int color[]) {
  gradientThread.disable();
  flashThread.disable();
  setValue(color);
}


void showColor(const int r, const int g, const int b) {
  if (debugMode) {
    Serial.println();
    Serial.println("---Setting output:---");
    Serial.print(r); Serial.print(", ");
    Serial.print(g); Serial.print(", ");
    Serial.print(b);
    Serial.println("---------------------");
  }
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

void showColor( int * const color[]) {  
  showColor(*color[0],*color[1],*color[2]);
}

void showColor( int color[]) {  
    showColor(color[0],color[1],color[2]);
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
  showColor(255,0,0);
  delay(baseTime);
  showColor(0,255,0);
  delay(baseTime);
  showColor(0,0,255);
  delay(baseTime);
  showColor(255,255,255);
  delay(baseTime);
  showColor(0,0,0);
}

void quickCycle(int totalTime_ms) {
/*  |-----------1/2 R/G/B/W pulse-----------|-----1/4 pulse-----|--1/8 pulse--|-1/8 grad->off-| */
  int one32th = totalTime_ms / 32;
  rgbFlash(4*one32th);  // 1/2 tTime * 1/4 rgb = 1/8  = 4*1/32
  rgbFlash(2*one32th);  // 1/4 tTime * 1/4 rgb = 1/16 = 2*1/32
  rgbFlash(  one32th);  // 1/8 tTime * 1/4 rgb = 1/32 =   1/32
//  gradientPulseSetup(WHITE,OFF, one32th*4000); // 1/8th tTime = 4*1/32; convert to seconds                                       // FIX
}


void toggleDebug() {
  debugMode = !debugMode;
  Serial.print("Debug mode ");
  Serial.println( debugMode ? "ON" : "OFF");
}

void printColor(const int * const rgb[]) {  
  Serial.print('#');
  if (*rgb[0]<=16) {Serial.print('0');}
  Serial.print(red, HEX);
  if (*rgb[1]<=16) {Serial.print('0');}
  Serial.print(green, HEX);
  if (*rgb[2]<=16) {Serial.print('0');}
  Serial.println(blue, HEX);
}

boolean redundantFlashCommand(const int col1[], const int col2[], int fTime) {
  for (int x=0; x<3; x++) {
    if (flashcol1[x]!=col1[x]) {return false;}
    if (flashcol2[x]!=col2[x]) {return false;}
  }
  if (flashTimePerColor_ms != fTime) {return false;}
  return true;
}

void printArray( int *ptr, int len) {
  for(int i = 0; i < len; i++)
    Serial.println( ptr[i]);
}

