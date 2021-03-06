/*
 * ChaseFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef CHASEFACTORY_HPP_
#define CHASEFACTORY_HPP_

#include "Theaterwagen.hpp"
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <wiringPi.h>
#include <CivetServer.h>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <uuid/uuid.h>
#include <thread>
#include <boost/shared_ptr.hpp>
#include <yaml-cpp/yaml.h>
#include "ButtonFactory.hpp"
#include "FixtureFactory.hpp"
#include "SceneFactory.hpp"
#include "MusicFactory.hpp"
#include "SoundFactory.hpp"
#include "LiftFactory.hpp"
#include "MotorFactory.hpp"
#include "ToggleFactory.hpp"
#include "CloneFactory.hpp"
#include "WebHandler.hpp"

#define CONFIG_FILE_CHASE CONFIG_DIR "chasefactory.yaml"

extern CivetServer* server;

struct sequence_item {
	std::string action;
	std::string uuid_or_milliseconds;
	bool active = false;
	bool invalid = false;
};

class ChaseFactory {
	public:
	friend class ButtonFactory;
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
		friend class ButtonFactory;
		Chase(ChaseFactory& cf, std::string naam, std::string omschrijving, bool autostart);
		Chase(ChaseFactory& cf, std::string uuidstr, std::string naam, std::string omschrijving, bool autostart, std::list<sequence_item>* sequence_list);
		~Chase();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		bool getAutostart();
		std::string getUrl();
		bool isRunning();
		void Stop();
		void Start();
		private:
		class ChaseHandler : public CivetHandler
		{
			public:
			ChaseHandler(Chase& chase);
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
		bool autostart;
		std::string url;
		std::string naam;
		std::string omschrijving;
		std::list<sequence_item>* sequence_list;
		uuid_t uuid;
	};
	public:
	ChaseFactory();
	~ChaseFactory();
	ChaseFactory::Chase* addChase(std::string naam, std::string omschrijving, bool autostart);
	void deleteChase(std::string uuid);
	void load();
	void saveAll();
	private:
	void save();

	ButtonFactory* button;
	CloneFactory* clone;
	FixtureFactory* fixture;
	SceneFactory* scene;
	MusicFactory* music;
	SoundFactory* sound;
	LiftFactory* lift;
	MotorFactory* motor;
	ToggleFactory* toggle;
	WebHandler* web;

	std::map<std::string, ChaseFactory::Chase*> chasemap;
	ChaseFactoryHandler* mfh;
};


#endif /* CHASEFACTORY_HPP_ */
