#include "GradientCommand.h"

GradientCommand::GradientCommand( int col1[3], int col2[3], GradientMode gradMode, int pulselen) {
	memcpy(this->col1,col1,3);
	memcpy(this->col2,col2,3);
	this->gradMode = gradMode;
	this->pulselen = pulselen;
};

GradientCommand::GradientCommand( const int col1[3], const int col2[3], const GradientMode gradMode, const seconds pulselen) {
  memcpy(this->col1,col1,3);
  memcpy(this->col2,col2,3);
  this->gradMode = gradMode;
  this->pulselen = pulselen;
};


GradientCommand::GradientCommand() {
  this->gradMode = NO_GRAD; // investigate enum defaults; what happens when we leave it undefined?
}

boolean GradientCommand::equals( GradientCommand ogc) {
  if ((this-> gradMode != ogc.gradMode) || (this->pulselen != ogc.pulselen)) {
    return false;
  }
  for (int i = 0; i<3; i++) {
    if (this->col1[i]!=ogc.col1[i]) { return false; }
    if (this->col2[i]!=ogc.col2[i]) { return false; }
  }
  return true;
}

void GradientCommand::setcol1( int col1[3] ) {
  memcpy(this->col1,col1,3);
}

void GradientCommand::setcol2( int col2[3] ) {
  memcpy(this->col2,col2,3);
}

void GradientCommand::setcol1( const int col1[3] ) {
  memcpy(this->col1,col1,3);
}

void GradientCommand::setcol2( const int col2[3] ) {
  memcpy(this->col2,col2,3);
}

void GradientCommand::setMode( GradientMode gradMode) {
	this->gradMode = gradMode;
}

void GradientCommand::setPulse( seconds timeInSeconds ) {
	this->pulselen = timeInSeconds;
}

