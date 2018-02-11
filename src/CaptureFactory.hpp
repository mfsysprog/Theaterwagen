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
#include <opencv2/objdetect/objdetect.hpp>
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <b64/encode.h>
#include <thread>
#include <dlib/opencv.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/image_io.h>
#include <dlib/gui_widgets.h>
#include <sfeMovie/Movie.hpp>
#include "Theaterwagen.hpp"

#define CONFIG_FILE_CAPTURE CONFIG_DIR "capturefactory.yaml"
#define FACE_DOWNSAMPLE_RATIO 2
#define MOVIE_FRAMERATE 10

extern CivetServer* server;
extern std::stringstream* syslog;

enum captureType{
	CAP_CAM,
	CAP_FILE,
	CAP_FILE2
};

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
		Capture(CaptureFactory& cf, std::string naam, std::string omschrijving);
		Capture(CaptureFactory& cf, std::string uuidstr, std::string naam,
				std::string omschrijving, std::string filename, std::string filename2,
				std::vector<std::vector<std::vector<cv::Point2f>>>* filepoints,
				std::vector<std::vector<std::vector<cv::Point2f>>>* file2points,
				bool fileonly,
				unsigned int mix_file,
				unsigned int mix_file2);
		~Capture();
		void captureDetectAndMerge();
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
		CaptureFactory& cf;
		CaptureHandler* mh;
		void openCap(captureType capturetype);
		void closeCap();
		cv::Mat captureFrame(captureType capturetype);
		void getFrame(int framenumber, bool drawFaces);
		std::vector<std::vector<cv::Point2f>> detectFrame(cv::Mat* input, captureType capturetype);
		std::vector<std::string> mergeFaces(captureType type, unsigned int mix_file, std::vector<std::vector<std::vector<cv::Point2f>>>* filePoints, bool fileonly);
		void mergeFrames();
		std::vector<std::string> morph(cv::Mat orig, cv::Mat target, unsigned int morphsteps);
		void loadModel();
		void captureLoop();
		void loadFilmpje();
		//void detectFilmpje();
		std::string url;
		std::string naam;
		std::string omschrijving;
		std::string filename = "";
		std::string filename2 = "";

		unsigned int mix_file = 100;
		unsigned int mix_file2 = 100;
		unsigned int morphsteps = 10;
		std::vector<std::vector<std::vector<cv::Point2f>>>* filePoints;
		std::vector<std::vector<std::vector<cv::Point2f>>>* file2Points;
		uuid_t uuid;
		cv::VideoCapture* cap;
		cv::CascadeClassifier* face_cascade;
		//dlib::frontal_face_detector* detector;
		bool model_loaded = false;
		bool fileonly = false;
	};
	public:
	CaptureFactory();
	~CaptureFactory();
	CaptureFactory::Capture* addCapture(std::string naam, std::string omschrijving);
	void deleteCapture(std::string uuid);
	void clearScreen();
	void load();

	private:
	dlib::shape_predictor* pose_model;
	std::string on_screen;
	volatile bool loadme = false, loaded = false;
	sf::RenderWindow* window;
	std::vector<cv::Mat>* camMat;
	std::vector<std::vector<std::vector<cv::Point2f>>>* camPoints;
	void renderingThread(sf::RenderWindow *window);
	void save();
	double scaleFactor = 1.2;
	int minNeighbors = 4;
	int minSizeX = 20;
	int minSizeY = 20;
	std::map<std::string, CaptureFactory::Capture*> capturemap;
	CaptureFactoryHandler* mfh;
};


#endif /* CAPTUREFACTORY_HPP_ */
