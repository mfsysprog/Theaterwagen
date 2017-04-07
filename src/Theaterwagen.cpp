   //============================================================================
// Name        : Theaterwagen.cpp
// Author      : Erik Janssen
// Version     :
// Copyright   : MIT License
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "FaceDetectorAndTracker.h"
#include "FaceSwapper.h"
#include "CivetServer.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "UploadHandler.hpp"
#include "ChaseFactory.hpp"

using namespace std;

#define DOCUMENT_ROOT "."
#define PORT "8080"

CivetServer* server = nullptr;

/*
 * Lesson 0: Test to make sure SDL is setup properly
 */
int main(int, char**){

	const char *options[] = {
		    "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0};

	    std::vector<std::string> cpp_options;
	    for (int i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
	        cpp_options.push_back(options[i]);
	    }

    // CivetServer server(options); // <-- C style start
	CivetServer server_start(cpp_options); // <-- C++ style start
	server = &server_start;

	UploadHandler hu;
	server->addHandler("/upload", hu);

	const size_t num_faces = 2;
	//FaceDetectorAndTracker detector("haarcascade_frontalface_default.xml", 0, num_faces);
	//FaceSwapper face_swapper("shape_predictor_68_face_landmarks.dat");

	if (wiringPiSetupGpio() != 0){
		std::cerr << "Unable to start wiringPi interface!" << std::endl;
		return 1;
	}

	ChaseFactory* chase = new ChaseFactory();

	double fps = 0;

    while (1==1){
    	delay(1000);
    }

    delete chase;
	return 0;
}
