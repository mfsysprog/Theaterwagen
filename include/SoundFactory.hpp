/*
 * SoundFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef SOUNDFACTORY_HPP_
#define SOUNDFACTORY_HPP_

#include "Theaterwagen.hpp"
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <wiringPi.h>
#include <CivetServer.h>
#include <SFML/Audio.hpp>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <uuid/uuid.h>
#include <boost/shared_ptr.hpp>
#include <yaml-cpp/yaml.h>

#define CONFIG_FILE_SOUND CONFIG_DIR "soundfactory.yaml"

extern CivetServer* server;
extern std::stringstream* syslog;

class SoundFactory {
	friend class ChaseFactory;
	private:
	class SoundFactoryHandler : public CivetHandler
	{
		public:
		SoundFactoryHandler(SoundFactory& soundfactory);
		~SoundFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		SoundFactory& soundfactory;
	};
	class Sound : public sf::Sound {
		friend class SoundFactory;
		friend class ChaseFactory;
		public:
		Sound(std::string uuidstr, std::string naam, std::string omschrijving, std::string fn, bool loop, float volume, float pitch, unsigned int fadesteps);
		Sound(std::string naam, std::string omschrijving, std::string fn);
		~Sound();
		void fadeOut();
		void fadeIn();
		unsigned int getFadeSteps();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getFilename();
		std::string getUrl();
		private:
		class SoundHandler : public CivetHandler
		{
			public:
			SoundHandler(Sound& sound);
			~SoundHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Sound& sound;
		};
		void setFadeSteps(unsigned int fadesteps);
		unsigned int fadesteps = 1;
		std::string filename;
		std::string naam;
		std::string omschrijving;
		std::string url;
		uuid_t uuid;
		sf::SoundBuffer* sfmbuffer;
	    SoundHandler* mh;
	};
	public:
	SoundFactory();
	~SoundFactory();
	SoundFactory::Sound* addSound(std::string naam, std::string omschrijving, std::string fn);
	void deleteSound(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, SoundFactory::Sound*> soundmap;
	SoundFactoryHandler* mfh;
};


#endif /* SOUNDFACTORY_HPP_ */
