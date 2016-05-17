#ifndef GradientCommand_h
#define GradientCommand_h

#include <Arduino.h>
#include "Typedefs.h"

extern char* GRADIENT_MODE_TEXTS[];
 
class GradientCommand {
	public:
    	GradientCommand( );
        GradientCommand( int col1[3], int col2[3], GradientMode gradMode, seconds pulselen );
        GradientCommand( const int col1[3], const int col2[3], GradientMode gradMode, const seconds pulselen );
        boolean equals(GradientCommand ogc );
        void setCol1( int col1[3] );
        void setCol2( int col2[3] );
        void setCol1( const int col1[3] );
        void setCol2( const int col2[3] );
        void setMode( GradientMode gradMode );
        void setPulse( seconds pulselen );
        void print();

		int col1[3];
		int col2[3];
		GradientMode gradMode;
		milliseconds pulselen;

    private:
    static void printA( int arr[3] ) {
      for(int i = 0; i < 3; i++) {
        Serial.print(arr[i]);
        Serial.print(" ");
      }
    }
};


 
#endif
