/*
 * MusicFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef MUSICFACTORY_HPP_
#define MUSICFACTORY_HPP_

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

#define CONFIG_FILE_MUSIC CONFIG_DIR "musicfactory.yaml"

extern CivetServer* server;
extern std::stringstream* syslog;

class MusicFactory {
	friend int main(int, char**);
	friend class ChaseFactory;
	private:
	class MusicFactoryHandler : public CivetHandler
	{
		public:
		MusicFactoryHandler(MusicFactory& musicfactory);
		~MusicFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		MusicFactory& musicfactory;
	};
	class Music : public sf::Music{
		friend class MusicFactory;
		friend class ChaseFactory;
		public:
        Music(std::string uuidstr, std::string naam, std::string omschrijving, std::string fn, bool loop, float volume, float pitch, unsigned int fadesteps);
		Music(std::string naam, std::string omschrijving, std::string fn);
		~Music();
		void fadeOut();
		void fadeIn();
		unsigned int getFadeSteps();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getFilename();
		std::string getUrl();
		private:
		class MusicHandler : public CivetHandler
		{
			public:
			MusicHandler(Music& music);
			~MusicHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Music& music;
		};
		void setFadeSteps(unsigned int fadesteps);
		unsigned int fadesteps = 1;
		std::string filename;
		std::string naam;
		std::string omschrijving;
		std::string url;
		uuid_t uuid;
	    MusicHandler* mh;
	};
	public:
	MusicFactory();
	~MusicFactory();
	MusicFactory::Music* addMusic(std::string naam, std::string omschrijving, std::string fn);
	void deleteMusic(std::string uuid);
	void load();

	private:
	void save();
	void shakeOff();
	void shakeOn();
	std::map<std::string, MusicFactory::Music*> musicmap;
	MusicFactoryHandler* mfh;
};


#endif /* MUSICFACTORY_HPP_ */
