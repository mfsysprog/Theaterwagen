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
#include "FixtureFactory.hpp"
#include "SceneFactory.hpp"
#include "MusicFactory.hpp"
#include "SoundFactory.hpp"
#include "MotorFactory.hpp"
#include "ToggleFactory.hpp"

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

	//Schip s1(27,22,17,18);
	//server->addHandler("/schip", s1.getHandler());

	FixtureFactory fixture1;
	fixture1.load();

	SceneFactory scene1(&fixture1);
	scene1.load();

	MusicFactory music1;
	music1.load();

	SoundFactory sound1;
	sound1.load();

	MotorFactory motor1;
	motor1.load();

	ToggleFactory toggle1;
	toggle1.load();

	music1.musicmap.find("c0f5d6cf-1cd6-4a22-b2c8-05b9b5e3e836")->second->Play();
	/*
	while(!(s1.Full(LEFT)))
	{
		delay(100);
	}
	s1.Start(RIGHT);

	while(!(s1.Full(RIGHT)))
		{
			delay(100);
		}
	s1.Start(LEFT);
	*/

	double fps = 0;

    while (1==1){
    	delay(100);
    }

	return 0;
}
