/* csarc allows you to do do a couple things with newline-terminated commands:
 *  
 *  set a constant color value: "V#FFFFFF"
 *  make gradient pulses between two colors: "G#000000:#FFFFFF:pulseTime"
 *    \--loop between two colors: "L##000000:#FFFFFF:pulseTime"
 *  
 *  Change SINGLE_LED and the color pins to match your hardware.
 *  You can toggle a handy debug mode with "d".
 */

#include <stdarg.h>

// -------------  LED SETUP -------------
#include <RGBdriver.h>
const int CLK = 2; //definitions for
const int DIO = 3; //Grove driver
RGBdriver Driver(CLK,DIO);
const boolean SINGLE_LED = true; // Grove LED driver or DFRobot shield?
const int R_PIN = 10; // pins for
const int G_PIN = 11; // directly driven
const int B_PIN =  9; // setup
int GND = 7; // ( <-for wiring convenience)

// --------- CONSTANTS + CONFIG ---------
const int   OFF[] = {0,   0,   0 };
const int   RED[] = {255, 0,   0 };
const int GREEN[] = {0,   255, 0 };
const int  BLUE[] = {0,   0,   255};
const int WHITE[] = {255, 255, 255};
const int BAUDRATE = 19200;
const int SERIAL_BUFFER_LENGTH = 128;
const int TIMEOUT = 300;  //timeout in seconds before beginning default pattern
const float numdivs = 60.0; //change this to adjust gradient smoothness

// ---------- GLOBAL VARIABLES ----------
boolean debugMode = false;
char inputBuffer[SERIAL_BUFFER_LENGTH];
float gradPerc, gradPercPerTick, gradDelayPerTick;
int gradcol1[3], gradcol2[3];
boolean gradientLoop, recentActivity = false;

int red,green,blue;
int* rgb[4] = {&red, &green, &blue, 0};

// -------------- THREADS ---------------
#include <TimedAction.h>
TimedAction   serialThread = TimedAction(1000, &processSerial);
TimedAction gradientThread = TimedAction(0,  &gradientStep); // initially disabled in setup()
TimedAction  timeoutThread = TimedAction(TIMEOUT*1000,  &timeoutAction);

void setup() {
  if (SINGLE_LED) {
    pinMode(R_PIN,OUTPUT);
    pinMode(G_PIN,OUTPUT);
    pinMode(B_PIN,OUTPUT);
    pinMode(GND,OUTPUT);
  }
  gradientThread.disable();
  Serial.begin(BAUDRATE);
  // power-up notification
  quickCycle(25);
}

void loop() {  
  serialThread.check();
  gradientThread.check();
  timeoutThread.check();
}

void processSerial() {
  Serial.readBytesUntil('\n',inputBuffer,SERIAL_BUFFER_LENGTH);
  if (inputBuffer!="") { // if connected
    recentActivity = true;   
    switch (inputBuffer[0]) {
      case 'd': {
        toggleDebug();
        break;
      }
      case 'V': { // set value specified in V#FFFFFF input
        processColor(inputBuffer);
        break;
      }
      
      case 'G' : {
        processGradient(inputBuffer);
        break;
      }
  
      case 'L' : {
        gradientLoop = true;
        processGradient(inputBuffer);
        break;
      }
    }
    if (debugMode) {
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
            Serial.print(*rgb[i]); Serial.print(',');
          }
          Serial.println(*rgb[3]);
        } 
      }
    }
  }
}

void toggleDebug() {
  debugMode=!debugMode;
  Serial.print("Debug mode ");
  if(debugMode) {
    Serial.println( "ON");  }
  else {
    Serial.println("OFF");  }
}

void processColor(char input[]) {
  char colorInput[] = {input[1],input[2],input[3],input[4],input[5],input[6],input[7],'\0'}; // who needs loops
  if (debugMode) {
    Serial.print("Serial input:"); 
    Serial.println(colorInput);  }
  decodeHex(colorInput,rgb);
  setOutput(rgb);
  printColors();
  if (debugMode) {
    Serial.println("Color set.  Results:");
    Serial.print(   "red: ");  Serial.println(   red, DEC);
    Serial.print( "green: ");  Serial.println( green, DEC);
    Serial.print(  "blue: ");  Serial.println(  blue, DEC);
    Serial.println(""); 
  }
}

void processGradient(char input[]) {
// run gradient pulse between a and b over a length of time; "G#000000:#123456:time" input
        if ((input[1]!='#' || input[9]!='#')) {
          Serial.println("Error: invalid gradient request");
          Serial.println(input);
        } else {
          char hex1[8], hex2[8];
          char timev[8]; //well our buffer is 7 chars now.
          for (int x=0; x<7; x++) {
            hex1[x]=input[x+1];
            hex2[x]=input[x+9];
            timev[x]=input[x+16];
          }
        int col1[4],col2[4];
        int *col1p[4] = {&col1[0],&col1[1],&col1[2],0};
        int *col2p[4] = {&col2[0],&col2[1],&col2[2],0};
        
        decodeHex(hex1,col1p);
        decodeHex(hex2,col2p);
        float timeVal = atol(timev); //use strtol or sscanf instead
        gradientPulseSetup(col1,col2,timeVal);
        
        if (debugMode) {
          Serial.println(hex1);
          Serial.println(hex2);
          Serial.println(timev); }
        }
}


void timeoutAction() {
  if (!recentActivity) {
    // maybe timeout and timeout behavior should be configurable over serial.
    // low priority, bc this will mostly be running before serial comms have
    // been established.
    gradientPulseSetup(OFF, WHITE, 1);
    gradientLoop = true;
  } 
}


void setOutput( int r, int g, int b) {

  if (!SINGLE_LED) {
    Driver.begin();
    Driver.SetColor(r, g, b);
    Driver.end();
  } else {
    analogWrite(R_PIN,r);
    analogWrite(G_PIN,g);
    analogWrite(B_PIN,b);
  }
}

void setOutput( int *color[]) {
  
  if (debugMode) {
    Serial.println(); // for some reason a lot of this output
    Serial.println("---Setting output:---"); // doesn't show up
    Serial.println(*color[0]);
    Serial.println(*color[1]);
    Serial.println(*color[2]);
    Serial.println(*color[3]);
    Serial.println("---------------------");
  }
    setOutput(*color[0],*color[1],*color[2]);
  
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




void gradientPulseSetup (const int col1[3],const int col2[3], float timev) { // timev is now in seconds   
  // in this initial version, gradient pulses are going to be locking.
  // This is obviously a problem - add an interrupt when you have time.

  gradDelayPerTick = (float) timev*1000/numdivs;
  gradPercPerTick = (float) 1/numdivs;
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
    Serial.print("gradDelayPerTick = ");
    Serial.println(gradDelayPerTick);
    Serial.print("gradPercPerTick = ");
    Serial.println(gradPercPerTick);
    Serial.println();
  }
  colorCopy(gradcol1,col1);
  colorCopy(gradcol2,col2);
  gradientThread.setInterval(gradDelayPerTick);
  gradientThread.reset();
  gradientThread.enable();
}



void gradientStep() {
  if (gradPerc < 1.0) {
    Serial.print(gradPerc);
    Serial.println("/1");
    gradientValue(gradcol1, gradcol2, gradPerc, rgb);
    setOutput(rgb);
    gradPerc+=gradPercPerTick;
  }
  else {
    if (gradientLoop = true) {
      gradPerc = 0;
    } else {
      gradientThread.disable();
    }
  }
}

void gradientValue(int col0[3], int col1[3], float percentage, int *output[3]) {
  *output[0] = col1[0]*percentage + col0[0]*(1-percentage);
  *output[1] = col1[1]*percentage + col0[1]*(1-percentage);
  *output[2] = col1[2]*percentage + col0[2]*(1-percentage);
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

void setValue ( int r, int g, int b) {
  red=r; green=g; blue=b;
  setOutput(rgb);
}

void colorCopy( int recipient[3], const int donor[3] ) {
  recipient[0] = donor[0];
  recipient[1] = donor[1];
  recipient[2] = donor[2];
}

void quickCycle(int baseTime) {
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
