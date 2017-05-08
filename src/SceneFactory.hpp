/*
 * SceneFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef SCENEFACTORY_HPP_
#define SCENEFACTORY_HPP_

#include "Theaterwagen.hpp"
#include <string>
#include <iostream>
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

#define CONFIG_FILE_SCENE "config/scenefactory.yaml"

extern CivetServer* server;

enum fadetype {
	 FADE_OFF,
	 FADE_IN,
	 FADE_OUT
};

class SceneFactory {
	friend class ChaseFactory;
	private:
	class SceneFactoryHandler : public CivetHandler
	{
		public:
		SceneFactoryHandler(SceneFactory& scenefactory);
		~SceneFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		SceneFactory& scenefactory;
	};
	class Scene {
		friend class SceneFactory;
		friend class ChaseFactory;
		public:
		Scene(FixtureFactory* ff, std::string uuidstr, std::string naam, std::string omschrijving, fadetype fade, unsigned int fadesteps, std::vector<std::vector<unsigned char>>* channels);
		Scene(FixtureFactory* ff, std::string naam, std::string omschrijving);
		~Scene();
		void Play();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		fadetype getFade();
		unsigned int getFadeSteps();
		std::string getUrl();
		private:
		class SceneHandler : public CivetHandler
		{
			public:
			SceneHandler(Scene& scene);
			~SceneHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Scene& scene;
		};
		std::string url;
		std::string naam;
		std::string omschrijving;
		fadetype fade = FADE_OFF;
		uuid_t uuid;
		unsigned int fadesteps = 1;
		std::vector<std::vector<unsigned char>>* channels;
		SceneHandler* mh;
		FixtureFactory* ff;
	};
	public:
	SceneFactory(FixtureFactory* ff);
	~SceneFactory();
	SceneFactory::Scene* addScene(std::string naam, std::string omschrijving);
	SceneFactory::Scene* addScene(std::string naam, std::string omschrijving, std::string uuid_like);
	void deleteScene(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, SceneFactory::Scene*> scenemap;
	SceneFactoryHandler* mfh;
	FixtureFactory* ff;
};


#endif /* SCENEFACTORY_HPP_ */
