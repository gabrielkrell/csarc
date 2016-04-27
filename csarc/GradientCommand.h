#ifndef GradientCommand.h
#define GradientComamnd.h
#include <Arduino.h>


typedef enum GradientMode {  SINGLE_FWD_LOOP,  FWD_LOOP,  SMOOTH_LOOP,  NO_GRAD };
typedef float seconds;
 
class GradientCommand {
	public:
		GradientCommand();
    GradientCommand( int col1[3], int col2[3], GradientMode gradMode, int pulselen);
    GradientCommand( const int col1[3], const int col2[3], GradientMode gradMode, seconds pulselen);
    boolean equals(GradientCommand ogc);
    void setcol1( int col1[3] );
    void setcol2( int col2[3] );
    void setcol1( const int col1[3] );
    void setcol2( const int col2[3] );
    void setMode( GradientMode gradMode);
    void setPulse( seconds pulselen);

		int col1[3];
		int col2[3];
		GradientMode gradMode;
		int pulselen;
};


 
#endif
