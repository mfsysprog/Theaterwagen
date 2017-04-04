/*
 * FixtureFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef FIXTUREFACTORY_HPP_
#define FIXTUREFACTORY_HPP_

#include <string>
#include <iostream>
#include <fstream>
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
#include "boost/shared_ptr.hpp"
#include "yaml-cpp/yaml.h"

#define RESOURCES_DIR "/home/erik/resources/"
#define CONFIG_FILE "/home/erik/config/fixturefactory.yaml"

extern CivetServer* server;

class FixtureFactory {
	friend class SceneHandler;
	friend class SceneFactory;
	friend class ChaseFactory;
	private:
	class FixtureFactoryHandler : public CivetHandler
	{
		public:
		FixtureFactoryHandler(FixtureFactory& fixturefactory);
		~FixtureFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		FixtureFactory& fixturefactory;
	};
	class Fixture {
		friend class FixtureFactory;
		friend class SceneHandler;
		friend class SceneFactory;
		friend class ChaseFactory;
		public:
		Fixture(std::string naam, std::string omschrijving, int base_channel, int number_channels);
		~Fixture();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		private:
		class FixtureHandler : public CivetHandler
		{
			public:
			FixtureHandler(Fixture& fixture);
			~FixtureHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Fixture& fixture;
		};
		int base_channel;
		int number_channels;
		std::string url;
		std::string naam;
		std::string omschrijving;
		FixtureHandler* mh;
	};
	public:
	FixtureFactory();
	~FixtureFactory();
	FixtureFactory::Fixture* addFixture(std::string naam, std::string omschrijving, int base_channel, int number_channels);
	void deleteFixture(int base);
	void load();

	private:
	void save();
	std::map<int, FixtureFactory::Fixture*> fixturemap;
	FixtureFactoryHandler* mfh;
};


#endif /* FIXTUREFACTORY_HPP_ */
