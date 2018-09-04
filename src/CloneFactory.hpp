/*
 * CloneFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef CLONEFACTORY_HPP_
#define CLONEFACTORY_HPP_

#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <vector>
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
#include <opencv2/objdetect/objdetect.hpp>
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <b64/encode.h>
#include <thread>
#include <mutex>
#include <sfeMovie/Movie.hpp>
#include "Theaterwagen.hpp"

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <x264.h>
}

#define CONFIG_FILE_CLONE CONFIG_DIR "clonefactory.yaml"
// video width at 1016 instead of 1024 to avoid 'zerocopy' bug
// https://github.com/raspberrypi/firmware/issues/851
#define VIDEO_WIDTH 1016
#define VIDEO_HEIGHT 768
#define VIDEO_FPS 10
#define VIDEO_EXT "mp4"

extern CivetServer* server;
extern std::stringstream* syslog;

enum cloneType{
	CAP_CAM,
	CAP_FILE,
	CAP_FILE2
};

struct ellipse_s {
    int centerx = 0;
    int centery = 0;
    float axeheight = 0;
    float axewidth = 0;
    double angle = 360;
    double blue = 255;
    double green = 255;
    double red = 255;
    int thickness = 2;
    int linetype = 8;
};

class CloneFactory {
	friend class ChaseFactory;
	private:
	class CloneFactoryHandler : public CivetHandler
	{
		public:
		CloneFactoryHandler(CloneFactory& clonefactory);
		~CloneFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		CloneFactory& clonefactory;
	};
	class Clone {
	public:
		friend class CloneFactory;
		friend class ChaseFactory;
		Clone(CloneFactory& cf, std::string naam, std::string omschrijving);
		Clone(CloneFactory& cf, std::string uuidstr, std::string naam,
				std::string omschrijving, std::string filename, std::string filename2,
				std::vector<ellipse_s>* ellipses_in,
				std::vector<ellipse_s>* ellipses_out,
				bool fileonly,
				bool clone_to_file,
				bool clone_to_file2,
				unsigned int mix_file,
				unsigned int mix_file2,
				unsigned int filesteps,
				unsigned int file2steps,
				unsigned int morphsteps);
		~Clone();
		void captureAndMerge();
		void mergeToScreen();
		void mergeToFile();
		void onScreen();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		void Stop();
		void Start();
		private:
		class CloneHandler : public CivetHandler
		{
			public:
			CloneHandler(Clone& schip);
			~CloneHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Clone& clone;
		};
		private:
		CloneFactory& cf;
		CloneHandler* mh;
		void openCap(cloneType clonetype);
		void closeCap();
		cv::Mat cloneFrame(cloneType clonetype, bool draw);
		void setPhoto(cv::Mat* input, bool draw);
		void getFrame(cloneType type, int framenumber, bool draw);
		std::vector<std::string> copyEllipses(cloneType type, unsigned int mix_file, std::vector<ellipse_s>* ellipses_in, std::vector<ellipse_s>* ellipses_out, bool fileonly);
		void mergeFrames();
		std::vector<std::string> morph(cv::Mat orig, cv::Mat target, unsigned int morphsteps);
		void cloneLoop();
		void loadFilmpje();
		//void detectFilmpje();
		std::string url;
		std::string naam;
		std::string omschrijving;
		std::string filename = "";
		std::string filename2 = "";

		unsigned int mix_file = 100;
		unsigned int mix_file2 = 100;
		unsigned int filesteps = 1;
		unsigned int file2steps = 1;
		unsigned int morphsteps = 10;
		std::vector<ellipse_s>* ellipses_in;
		std::vector<ellipse_s>* ellipses_out;
		uuid_t uuid;
		cv::VideoCapture* cap;
		bool model_loaded = false;
		bool fileonly = false;
		bool clone_to_file = false;
		bool clone_to_file2 = false;
	};
	public:
	CloneFactory();
	~CloneFactory();
	CloneFactory::Clone* addClone(std::string naam, std::string omschrijving);
	void deleteClone(std::string uuid);
	void clearScreen();
	void load();

	private:
	std::string on_screen;
	volatile bool loadme = false, loaded = false;
	sf::RenderWindow* window;
	cv::Mat* camMat;
	void renderingThread(sf::RenderWindow *window);
	void save();
	std::map<std::string, CloneFactory::Clone*> clonemap;
	CloneFactoryHandler* mfh;
};


#endif /* CLONEFACTORY_HPP_ */
