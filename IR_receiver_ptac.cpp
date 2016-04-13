#include <Arduino.h>
#include "Trace.h"
#include "Utils.h"
#include "IR_receiver_ptac.h"

// Digital pin #2 is the same as Pin D2 see
// http://arduino.cc/en/Hacking/PinMapping168 for the 'raw' pin mapping
#define IRpin_PIN PIND
#define IRpin 2

// the maximum pulse we'll listen for
#define MAXPULSE 10000 //loose, num loops

#define MAX_PULSE_IDX 50

uint16_t pulses[MAX_PULSE_IDX][2]; // pair is high and low pulse
uint8_t currentpulse = 0; // index for pulses we're storing

int IRsignal_power[] = {
	9000, 4200,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 0};

int IRsignal_mode[] = {
	9000, 4200,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 0};

int IRsignal_temp_up[] = {
	9000, 4200,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 0};


int IRsignal_temp_down[] = {
	9000, 4200,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 0};


int IRsignal_fan[] = {
	9000, 4200,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 600,
	600, 1800,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 600,
	600, 1800,
	600, 600,
	600, 1800,
	600, 1800,
	600, 1800,
	600, 0};


typedef enum {POWER, MODE, TEMP_UP, TEMP_DOWN, FAN, MAX_SIGNAL} SIGNAL_TYPES;
int *IRsignals[] = {IRsignal_power, IRsignal_mode, IRsignal_temp_up, IRsignal_temp_down, IRsignal_fan};
char *names[] = {"power", "mode", "temp_up", "temp_down", "fan"};

extern long holdStartTime;
void goToAndPressButton(uint8_t btn);
void releaseButton();
void checkRelease();

//
void listenIR(void)
{
	//Serial.println("\r\nReady to decode IR!");    

	uint16_t highpulse, lowpulse; // temporary storage timing
	highpulse = lowpulse = 0; // start out with no pulse length
	unsigned long start;

	start = micros();
	while (IRpin_PIN & (1 << IRpin))
	{
		//pin is still HIGH
		highpulse++;

		//timeout
		if (highpulse >= MAXPULSE)
		{
			checkRelease();
			if (currentpulse != 0)
			{
				onCodeFinish();
				currentpulse=0;
				return;
			}
		}
	}

	// we didn't time out so lets stash the reading
	pulses[currentpulse][0] = micros() - start;

	// same as above
	start = micros();  
	while (! (IRpin_PIN & _BV(IRpin)))
	{
		// pin is still LOW
		lowpulse++;

		if (lowpulse >= MAXPULSE)
		{
			checkRelease();
			if (currentpulse != 0)
			{
				onCodeFinish();
				currentpulse=0;
				return;
			}
		}
	}
	pulses[currentpulse][1] = micros() - start;

	// we read one high-low pulse successfully, continue!
	currentpulse++;
	
	if (currentpulse >= MAX_PULSE_IDX)
	{
		Serial.println("pulse too long, truncating");
		onCodeFinish();
	}
}

//
void onCodeFinish()
{
	//printpulses();
	matchCode();
}

//
void matchCode()
{
	Serial.println("matchCode");
	Serial.print("freeRam:");
	Serial.println(freeRam());
	int status[MAX_SIGNAL];
	for (int i = 0; i < MAX_SIGNAL; i++)
		status[i] = 0;

	for (uint8_t i = 0; i < currentpulse; i++)
	{
		int received_on = applyResolution(pulses[i][1]);
		int received_off;
		if (i < currentpulse - 1)
			received_off = applyResolution(pulses[i+1][0]);
		else
			received_off = 0;
		
		for (int j = 0; j < MAX_SIGNAL; j++)
		{
			int pos = status[j];
			int recorded_on = IRsignals[j][pos*2];
			int recorded_off = IRsignals[j][pos*2+1];
			//tracef("comparing aon:%d bon:%d aoff:%d boff:%d\r\n", received_on, recorded_on, received_off, recorded_off);
			if (received_on == recorded_on && received_off == recorded_off)
			{
				if (recorded_off == 0)
				{
					tracef("MATCH: %s\r\n", names[j]);
					if (j != TEMP_UP && j != TEMP_DOWN)
					{
						goToAndPressButton(j);
						delay(300);
						releaseButton();
						holdStartTime = -1;
					}
					else
					{
						if (holdStartTime == -1)
						{
							goToAndPressButton(j);
							holdStartTime = millis();
						}
						else
						{
							releaseButton();
							holdStartTime = -1;
						}							
					}
					return;
				}
				status[j]++;
				//tracef("i:%d j:%d status:%d\r\n", i, j, status[j]);
			}
			else
			{
				status[j] = 0;
			}
		}
	}
	Serial.println("no match");
}

//
//#define RESOLUTION 1
//#define RESOLUTION 300
#define RESOLUTION 600
uint16_t applyResolution(uint16_t val)
{
    return ((val + RESOLUTION / 2) / RESOLUTION) * RESOLUTION;
}

void printpulses(void)
{
	//print it in a 'array' format
	//skip initial 'off' duration
	Serial.println("int IRsignal[] = {");
	for (uint8_t i = 0; i < currentpulse-1; i++)
	{
		Serial.print("\t"); // tab
		Serial.print(applyResolution(pulses[i][1]), DEC);
		Serial.print(", ");
		Serial.print(applyResolution(pulses[i+1][0]), DEC);
		Serial.print(",\r\n");
	}
	Serial.print("\t"); // tab
	Serial.print(applyResolution(pulses[currentpulse-1][1]), DEC);
	Serial.print(", 0};\r\n\r\n");

	Serial.print("\r\n");
}




