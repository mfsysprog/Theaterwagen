   //============================================================================
// Name        : Theaterwagen.cpp
// Author      : Erik Janssen
// Version     :
// Copyright   : MIT License
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "Theaterwagen.hpp"

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "CivetServer.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "UploadHandler.hpp"
#include "ChaseFactory.hpp"

using namespace std;

CivetServer* server = nullptr;

/*
 * Lesson 0: Test to make sure SDL is setup properly
 */
int main(int, char**){

	const char *options[] = {
		    "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0};

	    std::vector<std::string> cpp_options;
	    for (unsigned int i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
	        cpp_options.push_back(options[i]);
	    }

    // CivetServer server(options); // <-- C style start
	CivetServer server_start(cpp_options); // <-- C++ style start
	server = &server_start;

	UploadHandler hu;
	server->addHandler("/upload", hu);

	//const size_t num_faces = 2;
	//FaceDetectorAndTracker detector("haarcascade_frontalface_default.xml", 0, num_faces);
	//FaceSwapper face_swapper("shape_predictor_68_face_landmarks.dat");

    setenv("WIRINGPI_GPIOMEM", "1", 1);
	if (wiringPiSetupGpio() != 0){
		std::cerr << "Unable to start wiringPi interface!" << std::endl;
		return 1;
	}

	ChaseFactory* chase = new ChaseFactory();

	do
	{
		delay(1000);
	} while(true);
    delete chase;
	return 0;
}
