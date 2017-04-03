/*
 * ToggleFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef TOGGLEFACTORY_HPP_
#define TOGGLEFACTORY_HPP_

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
#define CONFIG_FILE "/home/erik/config/togglefactory.yaml"

extern CivetServer* server;

class ToggleFactory {
	private:
	class ToggleFactoryHandler : public CivetHandler
	{
		public:
		ToggleFactoryHandler(ToggleFactory& togglefactory);
		~ToggleFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		ToggleFactory& togglefactory;
	};
	class Toggle {
	public:
		friend class ToggleFactory;
		Toggle(std::string naam, std::string omschrijving, int GPIO_relay);
		Toggle(std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_relay);
		~Toggle();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		void Stop();
		void Start();
		private:
		class ToggleHandler : public CivetHandler
		{
			public:
			ToggleHandler(Toggle& schip);
			~ToggleHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Toggle& toggle;
		};
		private:
		ToggleHandler* mh;
		void Initialize();
		std::string url;
		std::string naam;
		std::string omschrijving;
		uuid_t uuid;
		int relay;
	};
	public:
	ToggleFactory();
	~ToggleFactory();
	ToggleFactory::Toggle* addToggle(std::string naam, std::string omschrijving, int GPIO_relay);
	void deleteToggle(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, ToggleFactory::Toggle*> togglemap;
	ToggleFactoryHandler* mfh;
};


#endif /* TOGGLEFACTORY_HPP_ */
