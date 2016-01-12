#include "RGBdriver.h"
#define CLK 2//pins definitions for the driver        
#define DIO 3

RGBdriver Driver(CLK,DIO);
static int BAUDRATE = 9600;
int red,green,blue;

void setup() {
  Serial.begin(BAUDRATE);
}

void loop() {
  while (Serial.available() > 0) {
    // each input is in the form 000,123,255\n
    
    //parseInt skips the first non-digit and non-minus sign chars, so input is split:
    red = Serial.parseInt();  // 123
    green = Serial.parseInt();// (,)123
    blue = Serial.parseInt(); // (,)123
    if (Serial.read() == '\n') { //data probably received successfuly
      red = constrain(red, 0, 255);
      green = constrain(green, 0, 255);
      blue = constrain(blue, 0, 255);

      Driver.begin();
      Driver.SetColor(red, green, blue);
      Driver.end();

      // report back hex values
      Serial.print(red, HEX);
      Serial.print(green, HEX);
      Serial.println(blue, HEX);

      
     }
  }
}
