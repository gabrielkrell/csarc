#ifndef GradientCommand.h
#define GradientComamnd.h
#include <Arduino.h>


typedef enum GradientMode {  SINGLE_FWD_LOOP,  FWD_LOOP,  SMOOTH_LOOP,  NO_GRAD };
typedef float seconds;
typedef int milliseconds;
 
class GradientCommand {
	public:
	GradientCommand();
    GradientCommand( int col1[3], int col2[3], GradientMode gradMode, seconds pulselen);
    GradientCommand( const int col1[3], const int col2[3], GradientMode gradMode, const seconds pulselen);
    boolean equals(GradientCommand ogc);
    void setCol1( int col1[3] );
    void setCol2( int col2[3] );
    void setCol1( const int col1[3] );
    void setCol2( const int col2[3] );
    void setMode( GradientMode gradMode);
    void setPulse( seconds pulselen);

		int col1[3];
		int col2[3];
		GradientMode gradMode;
		milliseconds pulselen;
};


 
#endif
