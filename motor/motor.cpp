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



exploringBB::GPIO dirGPIO(DIR);
exploringBB::GPIO ms1GPIO(MS1);
exploringBB::GPIO ms2GPIO(MS2);
exploringBB::GPIO stepGPIO(STEP);
exploringBB::GPIO enableGPIO(ENABLE);

std::vector<int> spicerack_sectors; // This array is used as a circular array to represent the spicerack's position

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
    	ms1GPIO.setValue(exploringBB::LOW);
	ms2GPIO.setValue(exploringBB::LOW);
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

	// Setup the circular array
	for(int i = 0; i < SECTORS; i++)
	{
		spicerack_sectors.push_back(i + 1);
	}
}

// calculate the number of turns needed to turn from
// the current sector to the desired, selected sector
// returns a negative number for counter clockwise turns
// positive for clockwise.
int turnsNeeded(int selected_sector)
{
	int current = current_sector;
	int turns_needed = 0;
	int counter_clockwise_turns_needed = 0;
	// calculate the number of turns
	// needed to turn to the desired sector
	// when turning clockwise
	while(1)
	{
		if(spicerack_sectors.at(current) == selected_sector)
			break;
		else
		{
			current = (current + 1) % SECTORS;
			turns_needed++;
		}
	}

	current = current_sector;
	// calculate the number of turns
	// needed to turn to the desired sector
	// when turning counter clockwise
	while(1)
	{
		if(spicerack_sectors.at(current) == selected_sector)
		{
			break;
		}
		else
		{
			counter_clockwise_turns_needed++;
			current = current - 1;
			if(current == -1)
			{
				current = SECTORS -1;
			}
		}
	}

	// Figure out which turning direction requires the least amount of
	// turns, and return the appropirate turn number
	if(turns_needed >= counter_clockwise_turns_needed)
		return turns_needed;
	else
		return -counter_clockwise_turns_needed;
}

// turn the motor a given amount of steps based on the amount of turns
// needed. 1 turn equates to turning to 1 sector.
void turnMotorClockwise(int turns)
{
	int steps = turns * (STEPS_PER_REV / SECTORS); 
	dirGPIO.setValue(exploringBB::LOW); // Set forward direction
    ms1GPIO.setValue(exploringBB::LOW); // Set 1/4 Step
	ms2GPIO.setValue(exploringBB::HIGH);

	for(int i = 0; i < steps; i ++)
	{
		stepGPIO.setValue(exploringBB::HIGH);
		sleep_msec(1);
		stepGPIO.setValue(exploringBB::LOW);
		sleep_msec(1);
	}

	resetEDPins(); // sparkfun says we gotta reset the pins when we are done
}

void turnMotorCounterClockwise(int turns)
{
	int steps = turns * (STEPS_PER_REV / SECTORS); 
	dirGPIO.setValue(exploringBB::HIGH); // direction to backwards
    ms1GPIO.setValue(exploringBB::LOW); // Set 1/4 Step
	ms2GPIO.setValue(exploringBB::HIGH);
	for(int i = 0; i < steps; i ++)
	{
		stepGPIO.setValue(exploringBB::HIGH);
		sleep_msec(1);
		stepGPIO.setValue(exploringBB::LOW);
		sleep_msec(1);
	}

	resetEDPins();
}


// Give it the sector you want to turn it to, and the motor will
// turn to that sector.
void turnToSector(int sector)
{
	int needed_turns = turnsNeeded(sector);

	if(needed_turns > 0)
		turnMotorClockwise(needed_turns);
	else
		turnMotorCounterClockwise(needed_turns * -1);
	current_sector = sector;
}
