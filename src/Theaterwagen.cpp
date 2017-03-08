   //============================================================================
// Name        : Theaterwagen.cpp
// Author      : Erik Janssen
// Version     :
// Copyright   : MIT License
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <opencv2/highgui/highgui.hpp>
#include "FaceDetectorAndTracker.h"
#include "FaceSwapper.h"
#include "Schip.h"

using namespace std;

SDL_Texture* convertCV_MatToSDL_Texture(const cv::Mat &matImg, SDL_Renderer *ren)
{
    IplImage opencvimg2 = (IplImage)matImg;
    IplImage* opencvimg = &opencvimg2;

     //Convert to SDL_Surface
    SDL_Surface *frameSurface = SDL_CreateRGBSurfaceFrom(
                         (void*)opencvimg->imageData,
                         opencvimg->width, opencvimg->height,
                         opencvimg->depth*opencvimg->nChannels,
                         opencvimg->widthStep,
                         0xff0000, 0x00ff00, 0x0000ff, 0);

    if(frameSurface == NULL)
    {
        SDL_Log("Couldn't convert Mat to Surface.");
        return NULL;
    }

    //Convert to SDL_Texture
    SDL_Texture *frameTexture = SDL_CreateTextureFromSurface(ren, frameSurface);
    if(frameTexture == NULL)
    {
        SDL_Log("Couldn't convert Mat(converted to surface) to Texture.");
        return NULL;
    }
    else
    {
        //SDL_Log("SUCCESS conversion");
        return frameTexture;
    }
}

/*
 * Lesson 0: Test to make sure SDL is setup properly
 */
int main(int, char**){

	const size_t num_faces = 2;
	//FaceDetectorAndTracker detector("haarcascade_frontalface_default.xml", 0, num_faces);
	//FaceSwapper face_swapper("shape_predictor_68_face_landmarks.dat");

	if (wiringPiSetupGpio() != 0){
		std::cerr << "Unable to start wiringPi interface!" << std::endl;
		return 1;
	}
	Schip s1(27,22,17,18);

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

	double fps = 0;

	if (SDL_Init(SDL_INIT_VIDEO) != 0){
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	/* SDL_WINDOW_FULLSCREEN */
	SDL_Window *win = SDL_CreateWindow("Hello World!", 0, 0, 200, 100, SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI);
	if (win == nullptr){
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	//SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);

	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr){
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	std::string imagePath = "hello.bmp";
	SDL_Surface *bmp = SDL_LoadBMP(imagePath.c_str());
	if (bmp == nullptr){
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
	SDL_FreeSurface(bmp);
	if (tex == nullptr){
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	if (Mix_Init(SDL_INIT_AUDIO) < 0)
	    	printf("Error initializing audio: %s\n", Mix_GetError());

	    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
	    	printf("Error opening audio: %s\n", Mix_GetError());

	    // load the MP3 file "music.mp3" to play as music
	    Mix_Music *music;
	    music=Mix_LoadMUS("BennyHill.ogg");
	    if(!music) {
	        printf("Mix_LoadMUS(\"BennyHill.ogg\"): %s\n", Mix_GetError());
	        // this might be a critical error...
	    }

	    // play music forever
	    // Mix_Music *music; // I assume this has been loaded already
	    if(Mix_PlayMusic(music, -1)==-1) {
	        printf("Mix_PlayMusic: %s\n", Mix_GetError());
	        // well, there's no music, but most games don't break without music...
	    }

	    SDL_Event event;
	    bool gameRunning = true;

	    cv::UMat Uframe;

	    while (gameRunning)
	       {
	    	  if (SDL_PollEvent(&event))
	          {
	             if (event.type == SDL_QUIT)
	             {
	                gameRunning = false;
	             }

	             if (event.type == SDL_KEYDOWN)
	             {
	                switch (event.key.keysym.sym)
	                {
	                   case SDLK_ESCAPE:
	                      gameRunning = false;
	                      break;
	                }
	             }
	          }
	        try{
	        	 auto time_start = cv::getTickCount();

	        	 // Grab a frame

	        	 /*
	        	 detector >> Uframe;

	        	 //cout << "Size: " << Uframe.cols << " x " << Uframe.rows << std::endl;

	        	 cv::Mat frame = Uframe.getMat( cv::ACCESS_READ );

	        	 auto cv_faces = detector.faces();
	        	 if (cv_faces.size() == num_faces)
	        	 {
	        	    face_swapper.swapFaces(frame, cv_faces[0], cv_faces[1]);
	             }

	        	 auto time_end = cv::getTickCount();
	        	 auto time_per_frame = (time_end - time_start) / cv::getTickFrequency();

	        	 fps = (15 * fps + (1 / time_per_frame)) / 16;

	        	 printf("Total time: %3.5f | FPS: %3.2f\n", time_per_frame, fps);

	        	 // Display it all on the screen
	        	 //cv::imshow("Face Swap", frame);
	        	 tex = convertCV_MatToSDL_Texture(frame, ren); */

	        }
	        catch (exception& e)
	        {
	                cout << e.what() << endl;
	        }
	  		//First clear the renderer
	  		SDL_RenderClear(ren);
	  		//Draw the texture
	  		SDL_RenderCopy(ren, tex, NULL, NULL);
	  		//Update the screen
	  		SDL_RenderPresent(ren);
	  		//Take a quick break after all that hard work
	  		SDL_Delay(100);
	       }

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	Mix_FreeMusic(music);
	Mix_CloseAudio();

	SDL_Quit();
	return 0;
}
