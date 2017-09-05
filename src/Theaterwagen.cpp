   //============================================================================
// Name        : Theaterwagen.cpp
// Author      : Erik Janssen
// Version     :
// Copyright   : MIT License
// Description : Control for theater performances.
//               Sensors should be connected from pin to ground, on the
//               normally closed pin. (i.e.: if the button is NOT pressed, it
//               should be conducting. This way the button is always pulled to
//               ground (value 0) and only becomes 1 when the button is pressed
//               (and the circuit opened) because of the pull up.
//============================================================================

#include "Theaterwagen.hpp"

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "CivetServer.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "ChaseFactory.hpp"
#include "HomeHandler.hpp"
#include "FileHandler.hpp"

using namespace std;

CivetServer* server = nullptr;

std::stringstream getHtml(std::string meta, std::string message, std::string bodyclass, std::string data, std::string tohead){
	std::stringstream ss;
	ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
	ss << "text/html\r\nConnection: close\r\n\r\n";
	ss << "<!doctype html>";
	ss << "<html lang=\"en\">";
	ss << "<head>";
	ss << "	<meta charset=\"UTF-8\">";
	ss << "	<title>Theaterwagen</title>";
	ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
	ss << meta;
	ss << "	<link rel=\"stylesheet\" href=\"resources/style.css\">";
	ss << " <script type=\"text/javascript\" src=\"resources/jquery-3.2.0.min.js\"></script>";
	ss << tohead;
	ss << "</head>";
	ss << "<body class=\"" << bodyclass << "\" >";
	ss << "<label for=\"show-menu\" class=\"show-menu\">Toon Menu</label>";
	ss << "<input type=\"checkbox\" id=\"show-menu\" role=\"button\">";
	ss << "<ul id=\"menu\">";
	ss << "	<li class=\"home\"><a href=\"/theaterwagen\">Home</a></li>";
	ss << "	<li class=\"capture\"><a href=\"/capturefactory\">Foto's</a></li>";
	ss << "	<li class=\"chase\"><a href=\"/chasefactory\">Acties</a></li>";
	ss << "	<li class=\"motor\"><a href=\"/motorfactory\">Motoren</a></li>";
	ss << "	<li class=\"button\"><a href=\"/buttonfactory\">Knoppen</a></li>";
	ss << "	<li class=\"toggle\"><a href=\"/togglefactory\">Schakelaar</a></li>";
	ss << "	<li class=\"fixture\"><a href=\"/fixturefactory\">Lampen</a></li>";
	ss << "	<li class=\"scene\"><a href=\"/scenefactory\">Scene's</a></li>";
	ss << "	<li class=\"music\"><a href=\"/musicfactory\">Muziek</a></li>";
	ss << "	<li class=\"sound\"><a href=\"/soundfactory\">Geluidseffecten</a></li>";
	ss << "	<li class=\"files\"><a href=\"/files\">Bestanden</a></li>";
	ss << "</ul>";
	ss << "<br>";
	ss << "<h2>" << message << "</h2>";
	ss << data;
	ss << "</body></html>";
	return ss;
}

/*
 * Lesson 0: Test to make sure SDL is setup properly
 */
int main(int, char**){

	std::string home = std::string(getenv("HOME")) + "/" + ROOT_DIR;
	const char *options[] = {"cgi_interpreter", "/usr/bin/php-cgi",
//			"access_log_file", "access.log",
		    "document_root", home.c_str(), "listening_ports", PORT, 0};

	    std::vector<std::string> cpp_options;
	    for (unsigned int i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
	        cpp_options.push_back(options[i]);
	    }

    // CivetServer server(options); // <-- C style start
	CivetServer server_start(cpp_options); // <-- C++ style start
	server = &server_start;

	//UploadHandler hu;
	//server->addHandler("/upload", hu);

	//const size_t num_faces = 2;
	//FaceDetectorAndTracker detector("haarcascade_frontalface_default.xml", 0, num_faces);
	//FaceSwapper face_swapper("shape_predictor_68_face_landmarks.dat");

    setenv("WIRINGPI_GPIOMEM", "1", 1);
	if (wiringPiSetupGpio() != 0){
		std::cerr << "Unable to start wiringPi interface!" << std::endl;
		return 1;
	}

	ChaseFactory* chase = new ChaseFactory();

	FileHandler fh;
	server->addHandler("/files", fh);

	HomeHandler hh;
	server->addHandler("/theaterwagen", hh);

	do
	{
		delay(1000);
	} while(true);
    delete chase;
	return 0;
}
