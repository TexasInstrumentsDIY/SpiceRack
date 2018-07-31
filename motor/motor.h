#ifndef MOTOR_H
#define MOTOR_H 1

#include "../gpio/GPIO.h"
#include <vector>

#define DIR 46    //P8_16
#define MS1 44    //P8_12
#define MS2 38    //P8_38
#define STEP 26   //P8_14
#define ENABLE 45 //P8_13
#define SECTORS 6 
#define STEPS_PER_REV 800

void initMotorPin();
void turnToSector(int sector);
void setMotorPinDir();


#endif
