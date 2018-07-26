#ifndef MOTOR_H
#define MOTOR_H 1

extern exploringBB::GPIO dirGPIO;
extern exploringBB::GPIO ms1GPIO;
extern exploringBB::GPIO ms2GPIO;
extern exploringBB::GPIO stepGPIO;
extern exploringBB::GPIO enableGPIO;

#define DIR 46
#define MS1 44
#define MS2 68
#define STEP 26
#define ENABLE 67
#define SECTORS 6
#define STEPS_PER_REV 800

void initMotorPin();
void turnToSector(int sector)



#endif