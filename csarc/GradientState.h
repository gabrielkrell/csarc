#ifndef GradientState_h
#define GradientState_h
#include <Arduino.h>

#ifndef GradientCommand_h
#include "GradientCommand.h"
#endif

#ifndef Typedefs_h
#include "Typedefs.h"
#endif

class GradientState {
public:
	GradientState();

	GradientCommand command;
	float percent;
	float percentPerTick;
	milliseconds delayPerTick;
	boolean colors_reversed;

};

#endif

/* struct GradientState {
  const GradientCommand * command;
  float percent;
  float percentPerTick;
  milliseconds delayPerTick;
  boolean colors_reversed;
};
*/

/* void gradientPulseSetup( GradientState * gs, const GradientCommand * gc ) {
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

*/

/* void gradientStep() {
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
*/

/* void gradientValue(const int col0[3], const int col1[3], const float percentage, int * const output[3]) {
  *output[0] = col1[0]*percentage + col0[0]*(1-percentage);
  *output[1] = col1[1]*percentage + col0[1]*(1-percentage);
  *output[2] = col1[2]*percentage + col0[2]*(1-percentage);
}
*/