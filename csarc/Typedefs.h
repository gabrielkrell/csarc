#ifndef Typedefs_h
#define Typedefs_h

enum GradientMode {  SINGLE_FWD_LOOP,  FWD_LOOP,  SMOOTH_LOOP,  NO_GRAD };
typedef float seconds;
typedef int milliseconds;

#ifndef GRADIENT_MODE_TEXTS
char* GRADIENT_MODE_TEXTS[] = {
	"Single forward loop",
	"Forward loop",
	"Smooth loop",
	"No gradient"
};
#endif

#endif
