/*
 * LiftFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef LIFTFACTORY_HPP_
#define LIFTFACTORY_HPP_

#include "Theaterwagen.hpp"
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <iomanip>
#include <functional>
#include <sstream>
#include <fstream>
#include <wiringPi.h>
#include "CivetServer.h"
#include <SFML/Audio.hpp>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <uuid/uuid.h>
#include <vector>
#include "boost/shared_ptr.hpp"
#include "yaml-cpp/yaml.h"
#include "FixtureFactory.hpp"

#define CONFIG_FILE_LIFT CONFIG_DIR "liftfactory.yaml"

extern CivetServer* server;
extern std::stringstream* syslog;

enum liftdir : int {
	 LIFT_UP,
	 LIFT_DOWN,
	 LIFT_STOP
};

class LiftFactory {
	friend class ChaseFactory;
	private:
	class LiftFactoryHandler : public CivetHandler
	{
		public:
		LiftFactoryHandler(LiftFactory& liftfactory);
		~LiftFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		LiftFactory& liftfactory;
	};
	class Lift {
		friend class LiftFactory;
		friend class ChaseFactory;
		public:
		Lift(std::string uuidstr,
				std::string naam,
				std::string omschrijving,
				unsigned char channel,
				int gpio_dir,
				int gpio_trigger,
				int gpio_echo1,
				int gpio_echo2,
				float upper_limit,
				float lower_limit,
				bool leftOff,
				bool rightOff,
				float timeUp,
				float timeDown);
		Lift(std::string naam,
				std::string omschrijving,
				unsigned char channel,
				int gpio_dir,
				int gpio_trigger,
				int gpio_echo1,
				int gpio_echo2);
		~Lift();
		void Up();
		void Down();
		void Stop();
		void Wait();

		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		private:
		class LiftHandler : public CivetHandler
		{
			public:
			LiftHandler(Lift& lift);
			~LiftHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Lift& lift;
		};
		std::string url;
		std::string naam;
		std::string omschrijving;
		uuid_t uuid;
		bool leftOff = false, rightOff = false;
		float timeUp = 0.0, timeDown = 0.0;
		bool getPosition(liftdir direction);
		void Initialize();
		void Echo1();
		void Echo2();
		float upper_limit = 30.00, lower_limit= 120.00;
		unsigned char channel;
		int gpio_dir, gpio_trigger, gpio_echo1, gpio_echo2;
		volatile liftdir direction = LIFT_STOP;
		LiftHandler* lh;
	};
	public:
	LiftFactory();
	~LiftFactory();
	LiftFactory::Lift* addLift(std::string naam,
			std::string omschrijving, unsigned char channel, int gpio_dir, int gpio_trigger, int gpio_echo1, int gpio_echo2);
	void deleteLift(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, LiftFactory::Lift*> liftmap;
	LiftFactoryHandler* mfh;
};


#endif /* LIFTFACTORY_HPP_ */
