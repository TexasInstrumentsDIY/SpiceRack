/* motor.cpp */
#include "motor.h"
#include "../gpio/GPIO.h"

/* Pins */
// P8_02 <--> DGND
// P8_08 <--> ENABLE
// P8_10 <--> MS2
// P8_12 <--> MS1
// P8_14 <--> STEP
// P8_16 <--> DIR

/* Motor facts */
// Bipolar
// 200 Steps per Revolution
// If using 1/4 MicroStepping, then 800 Steps per Revolution
// Assumes that sector one is at "front"

#define DIR 46
#define MS1 44
#define MS2 68
#define STEP 26
#define ENABLE 67
#define SECTORS 6
#define STEPS_PER_REV 800

exploringBB::GPIO dirGPIO(DIR);
exploringBB::GPIO ms1GPIO(MS1);
exploringBB::GPIO ms2GPIO(MS2);
exploringBB::GPIO stepGPIO(STEP);
exploringBB::GPIO enableGPIO(ENABLE);

std::vector<int> spicerack_sectors;

int current_sector = 0; // The current sector that is selected

/* Sleep for specified msec */
static void
sleep_msec(int32 ms)
{
#if (defined(_WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
    Sleep(ms);
#else
    /* ------------------- Unix ------------------ */
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
#endif
}

void resetEDPins()
{
	stepGPIO.setValue(exploringBB::LOW);
	dirGPIO.setValue(exploringBB::LOW);
    ms1GPIO.setValue(exploringBB::LOW); // Enable 1/4 Microstepping
	ms2GPIO.setValue(exploringBB::HIGH);
	enableGPIO.setValue(exploringBB::HIGH);
}

void initMotorPin()
{

	dirGPIO.setDirection(exploringBB::OUTPUT);
    ms1GPIO.setDirection(exploringBB::OUTPUT);
	ms2GPIO.setDirection(exploringBB::OUTPUT);
	stepGPIO.setDirection(exploringBB::OUTPUT);
	enableGPIO.setDirection(exploringBB::OUTPUT);
	resetEDPins();

	for(int i = 0; i < SECTORS; i++)
	{
		spicerack_sectors.push_back(i + 1);
	}
}

int turnsNeeded(int selected_sector)
{
	int current = current_sector;
	int turns_needed = 0;
	while(1)
	{
		if(spicerack_sectors.at(current) == selected_sector)
			break;
		else
		{
			current = (current + 1) % SECTORS;
		}
	}

	return turns_needed;
}

void turnMotor(int turns)
{
	int steps = turns * (STEPS_PER_REV / SECTORS); 

	for(int i = 0; i < steps; i ++)
	{
		stepGPIO.setValue(exploringBB::HIGH);
		sleep_msec(1);
		stepGPIO.setValue(exploringBB::LOW);
		sleep_msec(1);
	}
}


void turnToSector(int sector)
{
	int needed_turns = turnsNeeded(sector);
	turnMotor(needed_turns);
	current_sector = sector;
}