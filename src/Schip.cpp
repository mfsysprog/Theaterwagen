/*
 * Schip.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "Schip.h"

/*
 * The wiringPiISR function expects a c function for the callback. Since we cannot directly call a member function
 * (c is not aware of 'non-static' methods) we need a little trick.
 * The myWiringPiISR sets the callback to myCallbackLeft and myCallbackRight.
 * These functions use cbfunc_schip to point to a std::function object that 'wraps' the actual
 * member function and its argument we want to use for the callback.
 * The object is created by a std::bind, that adds the 'this' pointer as the
 * first (hidden) argument on the function call to the actual callback method and then adds the enum direction
 * as the first true argument to the call.
 */


static std::function<void()> cbfunc_schip[2];

static void myCallbackLeft()
{
  cbfunc_schip[LEFT]();
}

static void myCallbackRight()
{
  cbfunc_schip[RIGHT]();
}

void Schip::Dummy(){
	std::cout << "+";
}

int Schip::myWiringPiISR(int val, int mask, direction dir)
{
  switch (dir)
  {
  	  case LEFT:
  		  return wiringPiISR(val, mask, &myCallbackLeft);
  	  case RIGHT:
  		  return wiringPiISR(val, mask, &myCallbackRight);
  	  default:
  		  std::cerr << "Invalid direction specified on myWiringPiISR method call!!" << std::endl;
  		  return -1;
  }
}

bool Schip::Full(direction dir){
	switch (dir){
		case LEFT:
			return(full_left);
		case RIGHT:
			return(full_right);
		default:
			std::cerr << "Invalid direction specified on Full method call!!" << std::endl;
			return false;
	}
}

void Schip::Stop(direction dir){
		switch (dir){
		case LEFT:
			pinMode(left_sensor, INPUT);
			if (!(digitalRead(left_sensor))){
			   /*
			    * motor is NOT at the left sensor! This is unexpected! Full stop and error message!
			    */
     			digitalWrite(left_relay, HIGH);
				digitalWrite(right_relay, HIGH);
				std::cerr << "Schip should be at the left, but it is not!!!" << std::endl;
			}
			else {
				digitalWrite(left_relay, HIGH);
				full_left=true;
				cbfunc_schip[LEFT] = std::bind(&Schip::Dummy,this);
			}
		break;
		case RIGHT:
			pinMode(right_sensor, INPUT);
		    if (!(digitalRead(right_sensor))){
			   /*
			    * motor is NOT at the right sensor! This is unexpected! Full stop and error message!
			    */
			    digitalWrite(left_relay, HIGH);
				digitalWrite(right_relay, HIGH);
				std::cerr << "Schip should be at the right, but it is not!!!" << std::endl;
			}
			else {
				digitalWrite(right_relay, HIGH);
			    full_right=true;
			    cbfunc_schip[RIGHT] = std::bind(&Schip::Dummy,this);
			}
		break;
		default:
			std::cerr << "Unknown state received in Stop function!" << std::endl;
	}
}

void Schip::Start(direction dir){
	switch (dir){
			case LEFT:
				std::cout << "Start left called!" << std::endl;
				pinMode(left_sensor, INPUT);
				if (!(digitalRead(left_sensor))){
						/*
						 * motor is not already in left position, so we should start moving left
						 * after we set the interrupt handler for the sensor.
						 */
						full_left=false;
						cbfunc_schip[LEFT] = std::bind(&Schip::Stop,this,LEFT);
						digitalWrite(left_relay, LOW);
				}
				else full_left=true;
			break;
			case RIGHT:
				std::cout << "Start right called!" << std::endl;
				pinMode(right_sensor, INPUT);
				if (!(digitalRead(right_sensor))){
						/*
						 * motor is not already in right position, so we should start moving right
						 * after we set the interrupt handler for the sensor.
						 */
						full_right=false;
						cbfunc_schip[RIGHT] = std::bind(&Schip::Stop,this,RIGHT);
						digitalWrite(right_relay, LOW);
				}
				else full_right=true;
			break;
			default:
				std::cerr << "Unknown state received in Start function!" << std::endl;
		}
}

Schip::Schip(int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay) {
	/*
	 * initialise gpio
	 */
	left_relay = GPIO_left_relay;
	left_sensor = GPIO_left_sensor;
	right_relay = GPIO_right_relay;
	right_sensor = GPIO_right_sensor;
	/*
	 * set relay to output and full stop
	 */
	pinMode(left_relay, OUTPUT);
	pinMode(right_relay, OUTPUT);
	digitalWrite(left_relay, HIGH);
	digitalWrite(right_relay, HIGH);
	/*
	 * Initialize callback functions to Dummy
	 */
	cbfunc_schip[LEFT] = std::bind(&Schip::Dummy,this);
	cbfunc_schip[RIGHT] = std::bind(&Schip::Dummy,this);
	if ( myWiringPiISR (left_sensor, INT_EDGE_FALLING, LEFT) < 0 ) {
		 std::cerr << "Error setting interrupt for left GPIO sensor " << std::endl;
	 }
	if ( myWiringPiISR (right_sensor, INT_EDGE_FALLING, RIGHT) < 0 ) {
	     std::cerr << "Error setting interrupt for right GPIO sensor " << std::endl;
	}
	/*
     * move the motor to the left
	 */
	Start(LEFT);
}

Schip::~Schip() {
	// TODO Auto-generated destructor stub
}

