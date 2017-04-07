/*
 * MotorFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef MOTORFACTORY_HPP_
#define MOTORFACTORY_HPP_

#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <wiringPi.h>
#include "CivetServer.h"
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <uuid/uuid.h>
#include "boost/shared_ptr.hpp"
#include "yaml-cpp/yaml.h"

#define RESOURCES_DIR "/home/erik/resources/"
#define CONFIG_FILE "/home/erik/config/motorfactory.yaml"

extern CivetServer* server;

enum direction : int {
	 LEFT = 0,
	 RIGHT = 40
};

typedef void (*fptr)();

class MotorFactory {
	friend class ChaseFactory;
	private:
	class MotorFactoryHandler : public CivetHandler
	{
		public:
		MotorFactoryHandler(MotorFactory& motorfactory);
		~MotorFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		MotorFactory& motorfactory;
	};
	class Motor {
	public:
		friend class MotorFactory;
		friend class ChaseFactory;
		Motor(std::string naam, std::string omschrijving, int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay);
		Motor(std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay);
		~Motor();
		void Start(direction dir);
		bool Full(direction dir);
		void Wait();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		private:
		class MotorHandler : public CivetHandler
		{
			public:
			MotorHandler(Motor& schip);
			~MotorHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Motor& motor;
		};
		private:
		MotorHandler* mh;
		int myWiringPiISR(int val, int mask, direction dir);
		void Stop();
		void Stop(direction);
		void Initialize();
		void Dummy();
		std::string url;
		std::string naam;
		std::string omschrijving;
		uuid_t uuid;
		int left_sensor;
		int right_sensor;
		int left_relay;
		int right_relay;
		bool full_left=false;
		bool full_right=false;
	};
	public:
	MotorFactory();
	~MotorFactory();
	MotorFactory::Motor* addMotor(std::string naam, std::string omschrijving, int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay);
	void deleteMotor(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, MotorFactory::Motor*> motormap;
	MotorFactoryHandler* mfh;
};


#endif /* MOTORFACTORY_HPP_ */
