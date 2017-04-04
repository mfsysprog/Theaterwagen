/*
 * ChaseFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef CHASEFACTORY_HPP_
#define CHASEFACTORY_HPP_

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
#include "FixtureFactory.hpp"
#include "SceneFactory.hpp"
#include "MusicFactory.hpp"
#include "SoundFactory.hpp"
#include "MotorFactory.hpp"
#include "ToggleFactory.hpp"

#define RESOURCES_DIR "/home/erik/resources/"
#define CONFIG_FILE "/home/erik/config/chasefactory.yaml"

extern CivetServer* server;

class ChaseFactory {
	private:
	class ChaseFactoryHandler : public CivetHandler
	{
		public:
		ChaseFactoryHandler(ChaseFactory& chasefactory);
		~ChaseFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		ChaseFactory& chasefactory;
	};
	class Chase {
	public:
		friend class ChaseFactory;
		Chase(std::string naam, std::string omschrijving, int GPIO_relay);
		Chase(std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_relay);
		~Chase();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		void Stop();
		void Start();
		private:
		class ChaseHandler : public CivetHandler
		{
			public:
			ChaseHandler(Chase& schip);
			~ChaseHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Chase& chase;
		};
		private:
		ChaseHandler* mh;
		void Initialize();
		std::string url;
		std::string naam;
		std::string omschrijving;
		uuid_t uuid;
		int relay;
	};
	public:
	ChaseFactory();
	~ChaseFactory();
	ChaseFactory::Chase* addChase(std::string naam, std::string omschrijving, int GPIO_relay);
	void deleteChase(std::string uuid);
	void load();

	private:
	void save();

	FixtureFactory* fixture;
	SceneFactory* scene;
	MusicFactory* music;
	SoundFactory* sound;
	MotorFactory* motor;
	ToggleFactory* toggle;

	std::map<std::string, ChaseFactory::Chase*> chasemap;
	ChaseFactoryHandler* mfh;
};


#endif /* CHASEFACTORY_HPP_ */
