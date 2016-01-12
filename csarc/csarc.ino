/* we'd like to be able to do a couple things with newline-terminated commands.
 *  
 *  set a constant color value: "V#FFFFFF"
 *  make gradient pulses between two colors: "G#000000:#FFFFFF:pulseTime"
 *  ???
 */


#include "RGBdriver.h"
#define CLK 2 //pin definitions for the driver        
#define DIO 3

RGBdriver Driver(CLK,DIO);
static int BAUDRATE = 9600;
int red,green,blue;
int* rbg[3] = {&red, &green, &blue};
// probably actually learn about pointers before using this, but it'd make some things simpler.

void setup() {
  Serial.begin(BAUDRATE);
}

void loop() {

  char input[255];

  Serial.readStringUntil('\n').toCharArray(input,255);
  if (input!="") { // if connected
    switch (input[0]) {
      case 'V': { // set value specified in V#FFFFFF input
        // function strtol() returns a long when given (string,end pointer,radix)
        long long number = strtol( &input[2], NULL, 16);   // from Stack Exchange
        // Split them up into r, g, b values
        int red = number >> 16;
        int green = number >> 8 & 0xFF;
        int blue = number & 0xFF;                           // end SE code=
        setOutput(red,green,blue);
        
        // report back hex values
        // to-do: fix this to include leading zeroes; right now 0F0F0F is transmitted as FFF
        Serial.print(red, HEX);
        Serial.print(green, HEX);
        Serial.println(blue, HEX);
        
        break;
      }
    case 'G' : {
        // run gradient pulse between a and b with appropriate timestep; "G#000000:#123456:time" input
        break;
      }
    }
  }
}

  void setOutput( int red, int green, int blue) {
    //"local variable hides a field" :p
    Driver.begin();
    Driver.SetColor(red, green, blue);
    Driver.end();
  }

  void setOutput( int color[]) {
    if (sizeof(color)!=3) {
      // log error message
    } else {
      setOutput(color[0],color[1],color[2]);
    }
  }

// TODO: reuse relevant driver code (like two lines)
//  while (Serial.available() > 0) {
//    // each input is in the form 000,123,255\n
//    
//    //parseInt skips the first non-digit and non-minus sign chars, so input is split:
//    red = Serial.parseInt();  // 123
//    green = Serial.parseInt();// (,)123
//    blue = Serial.parseInt(); // (,)123
//    if (Serial.read() == '\n') { //data probably received successfuly
//      red = constrain(red, 0, 255);
//      green = constrain(green, 0, 255);
//      blue = constrain(blue, 0, 255);
//
//      Driver.begin();
//      Driver.SetColor(red, green, blue);
//      Driver.end();
//
//      // report back hex values
//      Serial.print(red, HEX);
//      Serial.print(green, HEX);
//      Serial.println(blue, HEX);
//
//      
//     }
//  }
//}
