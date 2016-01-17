/* we'd like to be able to do a couple things with newline-terminated commands.
 *  
 *  set a constant color value: "V#FFFFFF"
 *  make gradient pulses between two colors: "G#000000:#FFFFFF:pulseTime"
 *  ???
 */

#include <stdarg.h>
static boolean DEBUGGING_MODE = true; // toggle serial debug text
static boolean SINGLE_LED = true; // false = Grove LED driver;
                                  // true = RGB LED for at-home testing
#include "RGBdriver.h"
#define CLK 2 //pin definitions for the driver        
#define DIO 3

RGBdriver Driver(CLK,DIO);
static int BAUDRATE = 19200;
int red,green,blue;
int* rgb[4] = {&red, &green, &blue};
// In use, & means "address of"; * means "follow this address".
// This is an array of pointers (which lead to our RGB vars).  Write
// to it with *rgb[x], which means "follow the address at rgb[x]" which
// leads to a color variable.

void setup() {
  if (SINGLE_LED) { // CC on pin 7
    pinMode(7,OUTPUT);
    pinMode(6,OUTPUT);
    pinMode(5,OUTPUT);
    pinMode(4,OUTPUT);
  }
  Serial.begin(BAUDRATE);
  rgb[4]=0;
}


int timeOutCount = 0;
char input[128];

void loop() {

  
  Serial.readStringUntil('\n').toCharArray(input,255);
  if (input!="") { // if connected    
    switch (input[0]) {
      case 'b': {
        analogWrite(4,150);
        break;
      }
      case '1': {
        *rgb[0]=255;
        *rgb[1]=255;
        *rgb[2]=255;
        setOutput(rgb);
        break;
      }
      case 'o': {
        setOutput(rgb);
        break;
      }
      case '*': {
        Serial.println(*rgb[0]);
        Serial.println(*rgb[1]);
        Serial.println(*rgb[2]);
        break;
      }
      case '?': {
        Serial.println(red);
        Serial.println(green);
        Serial.println(blue);
        break;
      }
      case 'X' : {
        red=0; green=0; blue=0;
        setOutput(rgb);
        break;
      }
      case '!': {
        red=0; green=0; blue=0;
        setOutput(red,green,blue);
        break;
      }
      case 'V': { // set value specified in V#FFFFFF input
        char colorInput[] = {input[1],input[2],input[3],input[4],input[5],input[6],input[7],'\0'}; // who needs loops
        if (DEBUGGING_MODE) {
          Serial.print("Serial input:"); 
          Serial.println(colorInput); 
        }
        
        decodeHex(colorInput,rgb);
        setOutput(rgb);

        if (DEBUGGING_MODE) {
          Serial.println("Color set.  Results:");
        }
        // echo current colors:
        Serial.print('#');
        if (red<=16) {Serial.print('0');}
        Serial.print(red, HEX);
        if (green<=16) {Serial.print('0');}
        Serial.print(green, HEX);
        if (blue<=16) {Serial.print('0');}
        Serial.println(blue, HEX);

        if (DEBUGGING_MODE) {
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
        if (input[9]=='#') {
          char hex1[7], hex2[7];
          char timev[7]; //well our buffer is 7 chars now.
          for (int x=0; x<7; x++) {
            hex1[x]=input[x+1];
            hex2[x]=input[x+9];
            timev[x]=input[x+16];
          }
          int col1[4],col2[4];
          int *col1p[4] = {&col1[0],&col1[1],&col1[2]};
          int *col2p[4] = {&col2[0],&col2[1],&col2[2]};
          decodeHex(hex1,col1p);
          decodeHex(hex2,col2p);

          float timeVal = atoi(timev); //use strtol or sscanf instead

          gradientPulse(col1,col2,timeVal);

          if (DEBUGGING_MODE) {
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
    }
  } else {
    // no input;
    if (timeOutCount > 5*60) { // 5 minutes with no comms
      setOutput( random(255), random(255), random(255) );
    }


    
  }
}


  void setOutput( int red, int green, int blue) {
    //"local variable hides a field" :p

    if (!SINGLE_LED) {
      Driver.begin();
      Driver.SetColor(red, green, blue);
      Driver.end();
    } else { //testing setup: single RGB led with cathode on 7
      analogWrite(6,red*.5);
      analogWrite(5,green*.5);
      analogWrite(4,blue*.5);
    }
  }

  void setOutput( int *color[]) {
    Serial.println(); // without this the first line below doesn't show up.
    Serial.println("---Setting output:---"); // v mysterious
    Serial.println(*color[0]);
    Serial.println(*color[1]);
    Serial.println(*color[2]);
    Serial.println(*color[3]);
    Serial.println("---------------------");

     
      setOutput(*color[0],*color[1],*color[2]);
    
  }

  void decodeHex( char hexColor[], int * output[] ) { // #FFFFFF
    // function strtol() returns a long when given (string,end pointer,radix)
    unsigned long number = strtol( &hexColor[1], NULL, 16);   // from Stack Exchange
    // Split them up into r, g, b values    
    *output[0] = number >> 16;
    *output[1] = number >> 8 & 0xFF;
    *output[2] = number & 0xFF;
    
    if (DEBUGGING_MODE) {
      Serial.println("---decodeHex debugging---");
      Serial.println("hexColor[]: ");
      Serial.println(hexColor);
      Serial.println("unsigned long number:");
      Serial.println(number);
      Serial.println("Output[]:");
      Serial.println(*output[0]);
      Serial.println(*output[1]);
      Serial.println(*output[2]);
      Serial.println(*output[4]);
      Serial.println("output:");
      printArray(*output);
      Serial.println("--------------------------");
      Serial.println();
    }
    
  }

  void printArray( int r[]) {
    int x=0;
    while (r[x]!='\0') {
      Serial.print(r[x]);
      Serial.print(' ');
      x++;
    }
   Serial.println();
   Serial.print(r[0]);
   Serial.print(r[1]);
   Serial.println(r[2]);
  }
  

  void gradientPulse (int col1[], int col2[], float timev) {
    // in this initial version, gradient pulses are going to be locking.
    // This is obviously a problem - add an interrupt when you have time.
    double tick = timev;
    double percentPerTick = tick/1;
    for (float percentage = 0; percentage+=percentPerTick; percentage < 1) {
      gradientValue(col1, col2, percentage,*rgb);
      setOutput(rgb);
    }
  }

  void gradientValue(int col1[3], int col2[3], float percentage, int output[3]) {
    output[0] = col1[0]*percentage + col2[0]*(1-percentage);
    output[1] = col1[1]*percentage + col2[1]*(1-percentage);
    output[2] = col1[2]*percentage + col2[2]*(1-percentage);
    
  }

void p(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}

