#ifndef THEATERWAGEN_HPP_
#define THEATERWAGEN_HPP_

#define ROOT_DIR "theaterwagen/"
#define RESOURCES_DIR ROOT_DIR "resources/"
#define MOVIES_DIR ROOT_DIR "filmpjes/"
#define CAPTURE_DIR ROOT_DIR "captures/"
#define MUSIC_DIR ROOT_DIR "muziek/"
#define SOUND_DIR ROOT_DIR "geluiden/"
#define CONFIG_DIR ROOT_DIR "config/"
#define TMP_DIR ROOT_DIR "tmp/"

#define PORT "8080"

#include <string>
#include <sstream>

using namespace std;

std::stringstream getHtml(std::string meta, std::string message, std::string bodyclass, std::string data, std::string tohead = "");

#endif
