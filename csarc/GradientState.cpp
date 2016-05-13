#include "GradientState.h"


// some constructors go here
GradientState::GradientState() {
	//careful about behavior here.  Maybe should print
	//diagnostics if accessed while not fully initialized
	this->reset();
}

GradientState::GradientState(GradientCommand gc) {
	this->setGradientCommand(gc);
}

void GradientState::setGradientCommand(GradientCommand gc) {
	this->reset();
	// set up
	this->setGradientCommand(gc);
	this->setDelayPerTick(1 / (gc.pulselen * defaultDelay));
	// 1 / number of ticks == percent per tick
	// 1 / (tv * defDelay) == ppt
}

milliseconds GradientState::getDelay() {
	return delayPerTick;
}

void GradientState::tickAndGetColor(int* const output[3]) {
  if (debugMode) { Serial.println("Gradient processing."); }
  if (this->percent <= 1.0) {
    Serial.print(this->percent);
    Serial.println("/1");
    int *col1, *col2;
    if (!this->colorsReversed) {
    	col1 = this->command.col1;
    	col2 = this->command.col2;
    } else {
    	col1 = this->command.col2;
    	col2 = this->command.col1;
    }
	*output[0] = col2[0]*this->percent + col1[0]*(1-this->percent);
	*output[1] = col2[1]*this->percent + col1[1]*(1-this->percent);
	*output[2] = col2[2]*this->percent + col1[2]*(1-this->percent);
    Serial.println("col1: "); printColor(this->command.col1);
    Serial.println("col2: "); printColor(this->command.col1);
    this->percent += this->percentPerTick;
  }
  else {
    switch (this->command.gradMode) {
      case SMOOTH_LOOP: {
      	this->colorsReversed = !this->colorsReversed;
        this->percent = 0;
        if (debugMode) { Serial.println("Looping gradient."); }
        break;
      }
      case FWD_LOOP: {
        this->percent = 0;
        if (debugMode) { Serial.println("Looping gradient."); }
        break;
      }
      case SINGLE_FWD_LOOP: {
        if (debugMode) { Serial.println("Gradient finished"); }
        // gradientThread.disable();
      }
      case NO_GRAD: {
        if (debugMode) { Serial.println("Gradient disabled."); }
        // gradientThread.disable();
      }
    }
  }
}

void GradientState::reset() {
	// zero important things
}

void GradientState::toggleDebug() {
	this->debugMode = !this->debugMode;
}

// private methods:

void GradientState::setDelayPerTick(milliseconds d) {
	delayPerTick = d;
}

void GradientState::printColor(int rgb[3]) {  
  Serial.print('#');
  if (rgb[0]<=16) {Serial.print('0');}
  Serial.print(rgb[0], HEX);
  if (rgb[1]<=16) {Serial.print('0');}
  Serial.print(rgb[1], HEX);
  if (rgb[2]<=16) {Serial.print('0');}
  Serial.println(rgb[2], HEX);
}