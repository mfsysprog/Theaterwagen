cmake_minimum_required(VERSION 3.1)
project(Theaterwagen)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

#Bring the headers into the project
include_directories(include)

#Add sources:
file(GLOB SOURCES "src/*.cpp")

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package( Threads REQUIRED)
add_executable(theaterwagen ${SOURCES})
#target_link_libraries( theaterwagen ${CMAKE_THREAD_LIBS_INIT} intl civetweb uuid yaml-cpp)
target_link_libraries( theaterwagen -pthread intl b64 yaml-cpp uuid sfeMovie sfml-system sfml-window sfml-graphics sfml-audio usb dl X11 wiringPi crypt opencv_core avformat avcodec avutil swscale opencv_imgproc opencv_videoio opencv_objdetect opencv_imgcodecs civetweb)
