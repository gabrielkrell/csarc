/* we'd like to be able to do a couple things with newline-terminated commands.
 *  
 *  set a constant color value: "V#FFFFFF"
 *  make gradient pulses between two colors: "G#000000:#FFFFFF:pulseTime"
 *  ???
 */

#include <stdarg.h>


static boolean SINGLE_LED = true; // real LED driver or test rig?
static int R_PIN = 6;   // pins for
static int G_PIN = 5;   // directly driven
static int B_PIN = 4;   // test setup
static int GND = 7;     // ( <-so we don't have to split cables)

#include "RGBdriver.h"
#define CLK 2 //pin definitions for Grove driver        
#define DIO 3
RGBdriver Driver(CLK,DIO);

static int BAUDRATE = 19200;
boolean DEBUG = false; // toggle serial debug text
int TIMEOUT = 300;  //timeout in seconds before beginning default pattern
int red,green,blue;
int* rgb[4] = {&red, &green, &blue, 0};
// In use, & means "address of"; * means "follow this address".
// This is an array of pointers (which lead to our RGB vars).  Write
// to it with *rgb[x], which means "follow the address at rgb[x]" which
// leads to a color variable.

void setup() {
  if (SINGLE_LED) {
    pinMode(R_PIN,OUTPUT);
    pinMode(G_PIN,OUTPUT);
    pinMode(B_PIN,OUTPUT);
    pinMode(GND,OUTPUT);
  }
  Serial.begin(BAUDRATE);
}


int timeOutCount = 0;
char input[128];

void loop() {  
  Serial.readStringUntil('\n').toCharArray(input,255);
  if (input!="") { // if connected    
    switch (input[0]) {

      case 'd': { //enables debug mode, lowercase commands
        DEBUG=!DEBUG;
        Serial.print("Debug mode ");
        if (DEBUG) 
          Serial.println("ON");
         else 
          Serial.println("OFF");
        break;
      }
      
      case 'V': { // set value specified in V#FFFFFF input
        char colorInput[] = {input[1],input[2],input[3],input[4],input[5],input[6],input[7],'\0'}; // who needs loops
        if (DEBUG) {
          Serial.print("Serial input:"); 
          Serial.println(colorInput); 
        }
        
        decodeHex(colorInput,rgb);
        setOutput(rgb);

        printColors();

        if (DEBUG) {
          Serial.println("Color set.  Results:");
          Serial.print("red: ");
          Serial.println(red, DEC);
          Serial.print("green: ");
          Serial.println(green, DEC);
          Serial.print("blue: ");
          Serial.println(blue, DEC);
          Serial.println("");
        }
        
        break;
      }
    case 'G' : {
        // run gradient pulse between a and b with appropriate timestep; "G#000000:#123456:time" input
        if (input[1]=='#' && input[9]=='#') {
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

          float timeVal = atoi(timev); //use strtol or sscanf instead

          gradientPulse(col1,col2,timeVal);

          if (DEBUG) {
            Serial.println(hex1);
            Serial.println(hex2);
            Serial.println(timev);
          }
        } else {
          Serial.println("Error: malformed gradient request");
          Serial.println(input);
        }
        break;
      }

      // begin debugging char commands
      

    }
  } else {
    // no input;
    if (timeOutCount > TIMEOUT) { // after TIMEOUT, flash randomly per second
      setOutput( random(255), random(255), random(255) ); // change to be nicer (1s gradients?)
    }


    
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
    
    if (DEBUG) {
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
    if (DEBUG) {
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


  static int numdivs = 60; //change this to adjust gradient smoothness
  void gradientPulse (int col1[], int col2[], float timev) {    
    // in this initial version, gradient pulses are going to be locking.
    // This is obviously a problem - add an interrupt when you have time.

    float delayPerTick = (float) timev/numdivs;
    float   perPerTick = (float) 1/numdivs;

    if (DEBUG) {
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
      Serial.println();
    }
    
    for (float per = 0; per+=perPerTick; per<=1.0) {
      Serial.println(per);
      gradientValue(col1,col2,per,rgb);
      setOutput(rgb);
      delay(delayPerTick);
    }
  }

  void gradientValue(int col0[3], int col1[3], float percentage, int *output[3]) {
    *output[0] = col1[0]*percentage + col0[0]*(1-percentage);
    *output[1] = col1[1]*percentage + col0[1]*(1-percentage);
    *output[2] = col1[2]*percentage + col0[2]*(1-percentage);
    
  }

void p(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}

void sClear() {
  for (int x=0; x<40; x++) { Serial.println(); }
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