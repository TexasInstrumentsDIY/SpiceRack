#ifndef MOTOR_H
#define MOTOR_H 1

#include "../gpio/GPIO.h"
#include <vector>

#define DIR 46
#define MS1 44
#define MS2 68
#define STEP 26
#define ENABLE 45
#define SECTORS 6
#define STEPS_PER_REV 800

void initMotorPin();
void turnToSector(int sector);



#endif
