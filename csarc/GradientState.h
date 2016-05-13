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
  GradientState(GradientCommand gc);
  void setGradientCommand(GradientCommand gc);
  milliseconds getDelay();
  void tickAndGetColor(int* const output[3]);
  void reset();
  void toggleDebug();

private:
  static const milliseconds defaultDelay = 16; // ~60Hz

  void setDelayPerTick(milliseconds delayMillis);
  void printColor(int rgb[3]);

    GradientCommand command;
    float percent;
    float percentPerTick;
    milliseconds delayPerTick;
    boolean colorsReversed;
    boolean debugMode;
};

#endif