/*
 * CaptureFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef CAPTUREFACTORY_HPP_
#define CAPTUREFACTORY_HPP_

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
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <b64/encode.h>

#define RESOURCES_DIR "/home/erik/resources/"
#define CONFIG_FILE "/home/erik/config/capturefactory.yaml"

extern CivetServer* server;

class CaptureFactory {
	friend class ChaseFactory;
	private:
	class CaptureFactoryHandler : public CivetHandler
	{
		public:
		CaptureFactoryHandler(CaptureFactory& capturefactory);
		~CaptureFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		CaptureFactory& capturefactory;
	};
	class Capture {
	public:
		friend class CaptureFactory;
		friend class ChaseFactory;
		Capture(std::string naam, std::string omschrijving);
		Capture(std::string uuidstr, std::string naam, std::string omschrijving);
		~Capture();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		void Stop();
		void Start();
		private:
		class CaptureHandler : public CivetHandler
		{
			public:
			CaptureHandler(Capture& schip);
			~CaptureHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Capture& capture;
		};
		private:
		CaptureHandler* mh;
		void Initialize();
		std::string url;
		std::string naam;
		std::string omschrijving;
		uuid_t uuid;
		std::stringstream serializedStream;
		cv::VideoCapture* cap;
	};
	public:
	CaptureFactory();
	~CaptureFactory();
	CaptureFactory::Capture* addCapture(std::string naam, std::string omschrijving);
	void deleteCapture(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, CaptureFactory::Capture*> capturemap;
	CaptureFactoryHandler* mfh;
};


#endif /* CAPTUREFACTORY_HPP_ */