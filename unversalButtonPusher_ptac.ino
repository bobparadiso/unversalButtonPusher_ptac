#include <Servo.h> 
#include "Trace.h"
#include "IR_receiver_ptac.h"

#define SHOULDER_SERVO 0
#define ELBOW_SERVO 1
#define FINGER_SERVO 2
#define NUM_SERVOS 3

#define FINGER_UP_POS 1260

#define SHOULDER_HOME_POS 1590
#define ELBOW_HOME_POS 780

#define SHOULDER_PIN 9
#define ELBOW_PIN 10
#define FINGER_PIN 11
#define SWITCH_PIN A0

#define MAX_BUTTONS 5
//shoulder, elbow finger_down
uint16_t buttonPositions[] = {
	1590, 1140, 1430, //power
	1770, 800, 1410, //mode
	1340, 978, 1410, //temp up
	1405, 770, 1410, //temp down
	1600, 780, 1410 //fan
};
 
Servo servo[NUM_SERVOS];
uint16_t lastPos[NUM_SERVOS];

long holdStartTime = -1;

//
void setPos(uint8_t servoID, uint16_t pos)
{
	servo[servoID].writeMicroseconds(pos);
	lastPos[servoID] = pos;
}

//
void lerpServo(uint8_t servoID, uint16_t startPos, uint16_t endPos, uint32_t duration)
{
	setPos(servoID, startPos);
	delay(100); //just in case

	uint32_t startTime = millis();
	uint32_t endTime = startTime + duration;

	while (1)
	{
		uint32_t elapsed = millis() - startTime;
		if (elapsed >= duration)
		{
			setPos(servoID, endPos);
			break;
		}

		uint16_t pos = map(elapsed, 0, duration, startPos, endPos);
		setPos(servoID, pos);
	}
}

//assumes last servo positions are stored
void lerpArmTo(uint16_t shoulderEnd, uint16_t elbowEnd, uint32_t duration)
{
	uint32_t startTime = millis();
	uint32_t endTime = startTime + duration;

	uint16_t shoulderStart = lastPos[SHOULDER_SERVO];
	uint16_t elbowStart = lastPos[ELBOW_SERVO];
	while (1)
	{
		uint32_t elapsed = millis() - startTime;
		if (elapsed >= duration)
		{
			setPos(SHOULDER_SERVO, shoulderEnd);
			setPos(ELBOW_SERVO, elbowEnd);
			break;
		}

		uint16_t shoulderPos = map(elapsed, 0, duration, shoulderStart, shoulderEnd);
		setPos(SHOULDER_SERVO, shoulderPos);
		uint16_t elbowPos = map(elapsed, 0, duration, elbowStart, elbowEnd);
		setPos(ELBOW_SERVO, elbowPos);
	}
}

//
void goToButton(uint8_t btn)
{
	//finger up
	setPos(FINGER_SERVO, FINGER_UP_POS);
	delay(500);

	uint16_t shoulderPos = buttonPositions[btn * NUM_SERVOS + SHOULDER_SERVO];
	uint16_t elbowPos = buttonPositions[btn * NUM_SERVOS + ELBOW_SERVO];

	//move over button
	lerpArmTo(shoulderPos, elbowPos, 1000);
	delay(100);
}

//assumes you are over button currently
void pressButton(uint8_t btn)
{
	uint16_t fingerPos = buttonPositions[btn * NUM_SERVOS + FINGER_SERVO];

	//finger down
	lerpServo(FINGER_SERVO, FINGER_UP_POS, fingerPos, 1000);
}

//
uint8_t pressedBtn = -1;
void goToAndPressButton(uint8_t btn)
{
	goToButton(btn);
	pressButton(btn);
	pressedBtn = btn;
}

//
void releaseButton()
{  
  uint16_t fingerPos = buttonPositions[pressedBtn * NUM_SERVOS + FINGER_SERVO];

  //finger up
  lerpServo(FINGER_SERVO, fingerPos, FINGER_UP_POS, 1000);
}

//
void testPress(uint16_t endPos)
{
	//finger up
	setPos(FINGER_SERVO, FINGER_UP_POS);
	delay(500);

	//finger down
	lerpServo(FINGER_SERVO, FINGER_UP_POS, endPos, 1000);
	delay(500);

	//finger up
	lerpServo(FINGER_SERVO, endPos, FINGER_UP_POS, 1000);
} 

#define MAX_CMD_LEN 64
char cmd[MAX_CMD_LEN];
int cmdIdx = 0;

//
void processCmd()
{
	tracef("got cmd: %s\r\n", cmd);

	char *typeStr, *idStr, *posStr, *timeStr;

	typeStr = strtok(cmd, " ");

	if (typeStr[0] == 'b')
	{
		idStr = strtok(NULL, " ");
		timeStr = strtok(NULL, "\n");

		uint8_t idVal = atoi(idStr);

		goToButton(idVal);
	}
	if (typeStr[0] == 'B')
	{
		idStr = strtok(NULL, " ");
		timeStr = strtok(NULL, "\n");

		uint8_t idVal = atoi(idStr);
		uint32_t time = atoi(timeStr);

		goToAndPressButton(idVal);
		delay(time);
		releaseButton();
	}
	else if (typeStr[0] == 'p')
	{
		posStr = strtok(NULL, "\n");
		uint16_t posVal = atoi(posStr);
		testPress(posVal);
	}
	else if (typeStr[0] == 's')
	{
		idStr = strtok(NULL, " ");
		posStr = strtok(NULL, "\n");

		uint8_t idVal = atoi(idStr);
		uint16_t posVal = atoi(posStr);

		tracef("got servo:%d pos:%d\r\n", idVal, posVal);

		setPos(idVal, posVal);
	}
}

//release of manual hold-time buttons (temp up/down)
void checkRelease()
{
	if (holdStartTime != -1)
	{
		long elapsed = millis() - holdStartTime;
		if (elapsed > 3000)
		{
			releaseButton();
			holdStartTime = -1;
		}
	}
}

// 
void setup() 
{
	pinMode(SWITCH_PIN, INPUT_PULLUP);

	servo[SHOULDER_SERVO].attach(SHOULDER_PIN);
	servo[ELBOW_SERVO].attach(ELBOW_PIN);
	servo[FINGER_SERVO].attach(FINGER_PIN);

	setPos(FINGER_SERVO, FINGER_UP_POS);
	setPos(SHOULDER_SERVO, SHOULDER_HOME_POS);
	setPos(ELBOW_SERVO, ELBOW_HOME_POS);  
	delay(500);

	Serial.begin(115200);
	Serial.println("ready");

/*
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		goToAndPressButton(i);
		if (i == 2 || i == 3)
			delay(1500);
		else
			delay(500);
		releaseButton();
	}
*/
	#define FINGER_STATE_UP 0
	#define FINGER_STATE_DOWN 1

	//main loop
	int fingerState = FINGER_STATE_UP;
	int nextButton = -1;
	while (1)
	{
		listenIR();
		checkRelease();

		if (Serial.available())
		{    
			char c = Serial.read();
			cmd[cmdIdx] = c;
			if (cmdIdx < MAX_CMD_LEN)
				cmdIdx++;

			if (c == '\n')
			{
				processCmd();
				cmdIdx = 0;
			}
		}
	}
} 

//
void loop() {}

