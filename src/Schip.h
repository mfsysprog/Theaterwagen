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
#include <sstream>
#include <wiringPi.h>
#include "CivetServer.h"
#include "usb.h"

enum direction{
	 LEFT, RIGHT
};

class Schip {
public:
	class SchipHandler : public CivetHandler
	{
		public:
		SchipHandler(Schip& schip);
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		private:
		Schip& schip;
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
	};
	Schip(int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay);
	virtual ~Schip();
	void Start(direction);
	bool Full(direction);
	CivetHandler& getHandler();

private:
	SchipHandler sh;
	int myWiringPiISR(int val, int mask, direction dir);
	void Stop();
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
