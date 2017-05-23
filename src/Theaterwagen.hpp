#ifndef THEATERWAGEN_HPP_
#define THEATERWAGEN_HPP_

#define RESOURCES_DIR "resources/"
#define DOCUMENT_ROOT "."
#define PORT "8080"

#include <string>
using namespace std;

std::stringstream getHtml(std::string meta, std::string message, std::string bodyclass, std::string data, std::string tohead = "");

#endif
