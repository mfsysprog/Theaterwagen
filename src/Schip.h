/*
 * Schip.h
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#ifndef SCHIP_H_
#define SCHIP_H_

#include <string>
#include <iostream>
#include <functional>
#include <wiringPi.h>

enum direction{
	 LEFT, RIGHT
};

class Schip {
public:
	Schip(int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay);
	virtual ~Schip();
	void Start(direction);
	bool Full(direction);

private:
	int myWiringPiISR(int val, int mask, direction dir);
	void Stop(direction);
	void Dummy();
	int left_sensor;
	int right_sensor;
	int left_relay;
	int right_relay;
	bool full_left=false;
	bool full_right=false;
};


#endif /* SCHIP_H_ */
