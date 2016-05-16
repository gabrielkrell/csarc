#ifndef GradientState_h
#define GradientState_h

#include <Arduino.h>
#include "GradientCommand.h"
#include "Typedefs.h"

extern char* GRADIENT_MODE_TEXTS[];

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