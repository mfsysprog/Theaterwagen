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
#include <thread>
#include "boost/shared_ptr.hpp"
#include "yaml-cpp/yaml.h"
#include "FixtureFactory.hpp"
#include "SceneFactory.hpp"
#include "MusicFactory.hpp"
#include "SoundFactory.hpp"
#include "MotorFactory.hpp"
#include "ToggleFactory.hpp"
#include "CaptureFactory.hpp"

#define RESOURCES_DIR "/home/erik/resources/"
#define CONFIG_FILE "/home/erik/config/chasefactory.yaml"

extern CivetServer* server;

struct sequence_item {
	std::string action;
	std::string uuid_or_milliseconds;
	bool active = false;
	bool invalid = false;
};

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
		Chase(ChaseFactory& cf, std::string naam, std::string omschrijving);
		Chase(ChaseFactory& cf, std::string uuidstr, std::string naam, std::string omschrijving, std::list<sequence_item>* sequence_list);
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
		void Action();
		ChaseFactory& cf;
		ChaseHandler* mh;
		volatile bool running;
		std::string url;
		std::string naam;
		std::string omschrijving;
		std::list<sequence_item>* sequence_list;
		uuid_t uuid;
	};
	public:
	ChaseFactory();
	~ChaseFactory();
	ChaseFactory::Chase* addChase(std::string naam, std::string omschrijving);
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
	CaptureFactory* capture;

	std::map<std::string, ChaseFactory::Chase*> chasemap;
	ChaseFactoryHandler* mfh;
};


#endif /* CHASEFACTORY_HPP_ */
