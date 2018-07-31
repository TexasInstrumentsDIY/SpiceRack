/* motor.cpp */
#include "motor.h"

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



exploringBB::GPIO* dirGPIO;
exploringBB::GPIO* ms1GPIO;
exploringBB::GPIO* ms2GPIO;
exploringBB::GPIO* stepGPIO;
exploringBB::GPIO* enableGPIO;

std::vector<int> spicerack_sectors; // This array is used as a circular array to represent the spicerack's position

int current_sector = 1; // The current sector that is selected

/* Sleep for specified msec */
static void
sleep_msec(int ms)
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
//	printf("reseting motor pins\n");
	stepGPIO->setValue(exploringBB::LOW);
//	printf("reseting motor pins 2\n");
	dirGPIO->setValue(exploringBB::LOW);
//	printf("reseting motor pins 3\n");
    	ms1GPIO->setValue(exploringBB::LOW);
//	printf("reseting motor pins 4\n");
	ms2GPIO->setValue(exploringBB::LOW);
//	printf("reseting motor pins 5\n");
	enableGPIO->setValue(exploringBB::HIGH);
//	printf("reseting motor pins 6\n");
}


void initMotorPin()
{
        printf("initing motoring pins a \n");
	dirGPIO = new exploringBB::GPIO(DIR);
        printf("initing motoring pins b\n");
	ms1GPIO = new exploringBB::GPIO(MS1);
        printf("initing motoring pins c\n");
	ms2GPIO = new exploringBB::GPIO(MS2);
        printf("initing motoring pins d\n");
        stepGPIO = new exploringBB::GPIO(STEP);
        printf("initing motoring pins e\n");
	sleep_msec(1000);
	enableGPIO = new exploringBB::GPIO(ENABLE);

        printf("initing motoring pins \n");
	while(dirGPIO->setDirection(exploringBB::OUTPUT)==-1){};

        printf("initing motoring pins 2 \n");
    	while(ms1GPIO->setDirection(exploringBB::OUTPUT)==-1){};
        printf("initing motoring pins 3\n");
	while(ms2GPIO->setDirection(exploringBB::OUTPUT)==-1){};
        printf("initing motoring pins 4\n");
	while(stepGPIO->setDirection(exploringBB::OUTPUT)==-1){};
        printf("initing motoring pins 5\n");
	sleep_msec(1000);
	while(enableGPIO->setDirection(exploringBB::OUTPUT)==-1){};
        printf("initing motoring pins 6\n");
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
	int current_index = current_sector - 1;
	int turns_needed = 0;
	int counter_clockwise_turns_needed = 0;
	// calculate the number of turns
	// needed to turn to the desired sector
	// when turning clockwise
	while(1)
	{
		if(spicerack_sectors.at(current_index) == selected_sector)
			break;
		else
		{
			current_index = (current_index + 1) % SECTORS;
			turns_needed++;
		}
	}

	current_index = current_sector - 1;
	// calculate the number of turns
	// needed to turn to the desired sector
	// when turning counter clockwise
	while(1)
	{
		if(spicerack_sectors.at(current_index) == selected_sector)
		{
			break;
		}
		else
		{
			counter_clockwise_turns_needed++;
			current_index = current_index - 1;
			if(current_index == -1)
			{
				current_index = SECTORS -1;
			}
		}
	}
        printf("Clockwise Turns Needed: %d || CounterCW Turns Needed: %d\n", turns_needed, counter_clockwise_turns_needed);;
	// Figure out which turning direction requires the least amount of
	// turns, and return the appropirate turn number
	if(turns_needed <= counter_clockwise_turns_needed)
		return turns_needed;
	else
		return -1*counter_clockwise_turns_needed;
}

// turn the motor a given amount of steps based on the amount of turns
// needed. 1 turn equates to turning to 1 sector.
void turnMotorClockwise(int turns)
{
	printf("Turning Motor Clockwise %d turns\n", turns);
	int steps = turns * (STEPS_PER_REV / SECTORS); 
	dirGPIO->setValue(exploringBB::LOW); // Set forward direction
    ms1GPIO->setValue(exploringBB::LOW); // Set 1/4 Step
	ms2GPIO->setValue(exploringBB::HIGH);

	for(int i = 0; i < steps; i ++)
	{
		stepGPIO->setValue(exploringBB::HIGH);
		sleep_msec(10);
		stepGPIO->setValue(exploringBB::LOW);
		sleep_msec(10);
	}

	resetEDPins(); // sparkfun says we gotta reset the pins when we are done
}

void turnMotorCounterClockwise(int turns)
{
	printf("Turning Motor CounterClockwise %d turns\n", turns);
	int steps = turns * (STEPS_PER_REV / SECTORS); 
	dirGPIO->setValue(exploringBB::HIGH); // direction to backwards
    ms1GPIO->setValue(exploringBB::LOW); // Set 1/4 Step
	ms2GPIO->setValue(exploringBB::HIGH);
	for(int i = 0; i < steps; i ++)
	{
		stepGPIO->setValue(exploringBB::HIGH);
		sleep_msec(10);
		stepGPIO->setValue(exploringBB::LOW);
		sleep_msec(10);
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
