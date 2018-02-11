
/*
 * CaptureFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "CaptureFactory.hpp"
using namespace std;
using namespace cv;
using namespace dlib;
using namespace sf;
using namespace sfe;

#include <chrono>
#include <iostream>
using namespace std::chrono;

#include <libintl.h>
#define _(String) gettext (String)

cv::Size feather_amount;
uint8_t LUTJE[3][256];
int source_hist_int[3][256];
int target_hist_int[3][256];
float source_histogram[3][256];
float target_histogram[3][256];

std::mutex m;
std::mutex m_merging;
std::mutex m_pose;

bool first_open = true;

class mythread : public std::thread
{
  public:
    mythread() {}
    static void setScheduling(std::thread &th, int policy, int priority) {
        sched_param sch_params;
        sch_params.sched_priority = priority;
        if(pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
        	(*syslog) << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
        }
    }
  private:
    sched_param sch_params;
};

/*
static cv::Rect dlibRectangleToOpenCV(dlib::rectangle r)
  return cv::Rect(cv::Point2i(r.left(), r.top()), cv::Point2i(r.right() + 1, r.bottom() + 1));
}
*/
static dlib::rectangle openCVRectToDlib(cv::Rect r)
{
  return dlib::rectangle((long)r.tl().x, (long)r.tl().y, (long)r.br().x - 1, (long)r.br().y - 1);
}

static std::string matToJPG(cv::Mat* input)
{
    std::vector<uchar> buf;
    cv::Mat resized;
    try
    {
        cv::resize((*input), resized, cv::Size(VIDEO_WIDTH,VIDEO_HEIGHT), 0, 0);
        int params[2] = {0};
        params[0] = CV_IMWRITE_JPEG_QUALITY;
        params[1] = 100;
    	cv::imencode(".jpg", resized, buf, std::vector<int>(params, params+1) );
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        (*syslog) << "exception caught: " << err_msg << std::endl;
    }

    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    return incoming.str();
}

static cv::Mat ImgToMat(std::string* input)
{

	std::vector<uchar> buf(input->begin(),input->end());

    cv::Mat restored;
    try
    {
        cv::imdecode(buf, cv::IMREAD_UNCHANGED, &restored);
        //cv::cvtColor(restored, restored, CV_RGB2BGR);
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        (*syslog) << "exception caught: " << err_msg << std::endl;
    }

    return restored;
}

// Apply affine transform calculated using srcTri and dstTri to src
void applyAffineTransform(Mat &warpImage, Mat &src, std::vector<Point2f> &srcTri, std::vector<Point2f> &dstTri)
{
    // Given a pair of triangles, find the affine transform.
    Mat warpMat = getAffineTransform( srcTri, dstTri );

    // Apply the Affine Transform just found to the src image
    warpAffine( src, warpImage, warpMat, warpImage.size(), INTER_LINEAR, BORDER_REFLECT_101);
}

// Calculate Delaunay triangles for set of points
// Returns the vector of indices of 3 points for each triangle
static void calculateDelaunayTriangles(cv::Rect rect, std::vector<cv::Point2f> &points, std::vector< std::vector<int> > &delaunayTri){

	// Create an instance of Subdiv2D
    Subdiv2D subdiv(rect);

	// Insert points into subdiv
    for( std::vector<cv::Point2f>::iterator it = points.begin(); it != points.end(); it++)
        subdiv.insert(*it);

	std::vector<Vec6f> triangleList;
	subdiv.getTriangleList(triangleList);
	std::vector<cv::Point2f> pt(3);
	std::vector<int> ind(3);

	for( size_t i = 0; i < triangleList.size(); i++ )
	{
		Vec6f t = triangleList[i];
		pt[0] = cv::Point2f(t[0], t[1]);
		pt[1] = cv::Point2f(t[2], t[3]);
		pt[2] = cv::Point2f(t[4], t[5 ]);

		if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])){
			for(int j = 0; j < 3; j++)
				for(size_t k = 0; k < points.size(); k++)
					if(abs(pt[j].x - points[k].x) < 1.0 && abs(pt[j].y - points[k].y) < 1)
						ind[j] = k;

			delaunayTri.push_back(ind);
		}
	}

}

std::vector <cv::Point2f> get_points(const dlib::full_object_detection& d)
{
    std::vector <cv::Point2f> points;
    for (int i = 0; i < 68; ++i)
    {
    	points.push_back(cv::Point2f(d.part(i).x(), d.part(i).y()));
    }

    //calculate forehead left (point index 68)
    //points.push_back(cv::Point2f(points[17].x+(points[17].x-points[2].x),points[17].y - (points[2].y-points[17].y)));

    //calculate forehead middle (point index 69)
    //points.push_back(cv::Point2f(points[27].x+(points[29].x-points[8].x), points[29].y - (points[8].y - points[29].y)));

    //calculate forehead right ((point index 70)
    //points.push_back(cv::Point2f(points[26].x+(points[26].x-points[14].x),points[26].y - (points[14].y-points[26].y)));

    return points;
}

void draw_polyline(cv::Mat &img, const dlib::full_object_detection& d, const int start, const int end, bool isClosed = false)
{
    std::vector <cv::Point> points;
    for (int i = start; i <= end; ++i)
    {
        points.push_back(cv::Point(d.part(i).x(), d.part(i).y()));
    }
    cv::polylines(img, points, isClosed, cv::Scalar(255,0,0), 2, 16);
}

void draw_polyline(cv::Mat &img, std::vector<cv::Point2f> points, const int start, const int end, bool isClosed = false)
{
    std::vector <cv::Point> points_part;
    for (int i = start; i <= end; ++i)
    {
        points_part.push_back(points[i]);
    }
    cv::polylines(img, points_part, isClosed, cv::Scalar(255,0,0), 2, 16);
}

// Warps and alpha blends triangular regions from img1 and img2 to img
void warpTriangle(Mat &img1, Mat &img2, std::vector<Point2f> &t1, std::vector<Point2f> &t2)
{

    cv::Rect r1 = boundingRect(t1);
    cv::Rect r2 = boundingRect(t2);

    // Offset points by left top corner of the respective rectangles
    std::vector<Point2f> t1Rect, t2Rect;
    std::vector<Point> t2RectInt;
    for(int i = 0; i < 3; i++)
    {

        t1Rect.push_back( Point2f( t1[i].x - r1.x, t1[i].y -  r1.y) );
        t2Rect.push_back( Point2f( t2[i].x - r2.x, t2[i].y - r2.y) );
        t2RectInt.push_back( Point(t2[i].x - r2.x, t2[i].y - r2.y) ); // for fillConvexPoly

    }

    // Get mask by filling triangle
    Mat mask = Mat::zeros(r2.height, r2.width, CV_32FC3);
    fillConvexPoly(mask, t2RectInt, Scalar(1.0, 1.0, 1.0), 16, 0);

    // Apply warpImage to small rectangular patches
    Mat img1Rect;
    img1(r1).copyTo(img1Rect);

    Mat img2Rect = Mat::zeros(r2.height, r2.width, img1Rect.type());

    applyAffineTransform(img2Rect, img1Rect, t1Rect, t2Rect);

    multiply(img2Rect,mask, img2Rect);
    multiply(img2(r2), Scalar(1.0,1.0,1.0) - mask, img2(r2));
    img2(r2) = img2(r2) + img2Rect;


}

void featherMask(cv::Mat &refined_masks)
{
    cv::erode(refined_masks, refined_masks, getStructuringElement(cv::MORPH_RECT, feather_amount), cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, cv::Scalar(0));

    cv::blur(refined_masks, refined_masks, feather_amount, cv::Point(-1, -1), cv::BORDER_CONSTANT);
}

inline void pasteFacesOnFrame(cv::Mat& small_frame, cv::Mat& warpped_faces, cv::Mat& refined_masks)
{
    for (int i = 0; i < small_frame.rows; i++)
    {
        auto frame_pixel = small_frame.row(i).data;
        auto faces_pixel = warpped_faces.row(i).data;
        auto masks_pixel = refined_masks.row(i).data;

        for (int j = 0; j < small_frame.cols; j++)
        {
            if (*masks_pixel != 0)
            {
                *frame_pixel = ((255 - *masks_pixel) * (*frame_pixel) + (*masks_pixel) * (*faces_pixel)) >> 8; // divide by 256
                *(frame_pixel + 1) = ((255 - *(masks_pixel + 1)) * (*(frame_pixel + 1)) + (*(masks_pixel + 1)) * (*(faces_pixel + 1))) >> 8;
                *(frame_pixel + 2) = ((255 - *(masks_pixel + 2)) * (*(frame_pixel + 2)) + (*(masks_pixel + 2)) * (*(faces_pixel + 2))) >> 8;
            }

            frame_pixel += 3;
            faces_pixel += 3;
            masks_pixel++;
        }
    }
}

void specifyHistogram(const cv::Mat source_image, cv::Mat target_image, cv::Mat mask)
{

    std::memset(source_hist_int, 0, sizeof(int) * 3 * 256);
    std::memset(target_hist_int, 0, sizeof(int) * 3 * 256);

    for (int i = 0; i < mask.rows; i++)
    {
        auto current_mask_pixel = mask.row(i).data;
        auto current_source_pixel = source_image.row(i).data;
        auto current_target_pixel = target_image.row(i).data;

        for (int j = 0; j < mask.cols; j++)
        {
            if (*current_mask_pixel != 0) {
                source_hist_int[0][*current_source_pixel]++;
                source_hist_int[1][*(current_source_pixel + 1)]++;
                source_hist_int[2][*(current_source_pixel + 2)]++;

                target_hist_int[0][*current_target_pixel]++;
                target_hist_int[1][*(current_target_pixel + 1)]++;
                target_hist_int[2][*(current_target_pixel + 2)]++;
            }

            // Advance to next pixel
            current_source_pixel += 3;
            current_target_pixel += 3;
            current_mask_pixel++;
        }
    }

    // Calc CDF
    for (int i = 1; i < 256; i++)
    {
        source_hist_int[0][i] += source_hist_int[0][i - 1];
        source_hist_int[1][i] += source_hist_int[1][i - 1];
        source_hist_int[2][i] += source_hist_int[2][i - 1];

        target_hist_int[0][i] += target_hist_int[0][i - 1];
        target_hist_int[1][i] += target_hist_int[1][i - 1];
        target_hist_int[2][i] += target_hist_int[2][i - 1];
    }

    // Normalize CDF
    for (int i = 0; i < 256; i++)
    {
        source_histogram[0][i] = (source_hist_int[0][255] ? (float)source_hist_int[0][i] / source_hist_int[0][255] : 0);
        source_histogram[1][i] = (source_hist_int[1][255] ? (float)source_hist_int[1][i] / source_hist_int[1][255] : 0);
        source_histogram[2][i] = (source_hist_int[2][255] ? (float)source_hist_int[2][i] / source_hist_int[2][255] : 0);

        target_histogram[0][i] = (target_hist_int[0][255] ? (float)target_hist_int[0][i] / target_hist_int[0][255] : 0);
        target_histogram[1][i] = (target_hist_int[1][255] ? (float)target_hist_int[1][i] / target_hist_int[1][255] : 0);
        target_histogram[2][i] = (target_hist_int[2][255] ? (float)target_hist_int[2][i] / target_hist_int[2][255] : 0);
    }

    // Create lookup table

    auto binary_search = [&](const float needle, const float haystack[]) -> uint8_t
    {
        uint8_t l = 0, r = 255, m;
        while (l < r)
        {
            m = (l + r) / 2;
            if (needle > haystack[m])
                l = m + 1;
            else
                r = m - 1;
        }
        // TODO check closest value
        return m;
    };

    for (size_t i = 0; i < 256; i++)
    {
        LUTJE[0][i] = binary_search(target_histogram[0][i], source_histogram[0]);
        LUTJE[1][i] = binary_search(target_histogram[1][i], source_histogram[1]);
        LUTJE[2][i] = binary_search(target_histogram[2][i], source_histogram[2]);
    }

    // repaint pixels
    for (int i = 0; i < mask.rows; i++)
    {
        auto current_mask_pixel = mask.row(i).data;
        auto current_target_pixel = target_image.row(i).data;
        for (int j = 0; j < mask.cols; j++)
        {
            if (*current_mask_pixel != 0)
            {
                *current_target_pixel = LUTJE[0][*current_target_pixel];
                *(current_target_pixel + 1) = LUTJE[1][*(current_target_pixel + 1)];
                *(current_target_pixel + 2) = LUTJE[2][*(current_target_pixel + 2)];
            }

            // Advance to next pixel
            current_target_pixel += 3;
            current_mask_pixel++;
        }
    }
}

static cv::Mat drawFaces(cv::Mat* input, std::vector<std::vector<cv::Point2f>>* points)
{
	cv::Mat output = (*input).clone();
	//Read points
    for (unsigned long i = 0; i < (*points).size(); ++i)
    {
        draw_polyline(output, (*points)[i], 0, 16);           // Jaw line
        draw_polyline(output, (*points)[i], 17, 21);          // Left eyebrow
        draw_polyline(output, (*points)[i], 22, 26);          // Right eyebrow
        draw_polyline(output, (*points)[i], 27, 30);          // Nose bridge
        draw_polyline(output, (*points)[i], 30, 35, true);    // Lower nose
        draw_polyline(output, (*points)[i], 36, 41, true);    // Left eye
        draw_polyline(output, (*points)[i], 42, 47, true);    // Right Eye
        draw_polyline(output, (*points)[i], 48, 59, true);    // Outer lip
        draw_polyline(output, (*points)[i], 60, 67, true);    // Inner lip
        //draw_polyline(output, (*points)[i], 68, 70);          // Forehead

        cv::circle(output, (*points)[i][8], 3, cv::Scalar(0,255,255), -1, 16);
        cv::circle(output, (*points)[i][29], 3, cv::Scalar(0,255,255), -1, 16);
        //cv::circle(output, (*points)[i][69], 3, cv::Scalar(0,255,255), -1, 16);
    }
    return output;
}

/*
static std::string drawToJPG(cv::Mat* input, std::vector<std::vector<cv::Point2f>>* points)
{
	cv::Mat output = (*input).clone();

	//Read points
    for (unsigned long i = 0; i < (*points).size(); ++i)
    {
        draw_polyline(output, (*points)[i], 0, 16);           // Jaw line
        draw_polyline(output, (*points)[i], 17, 21);          // Left eyebrow
        draw_polyline(output, (*points)[i], 22, 26);          // Right eyebrow
        draw_polyline(output, (*points)[i], 27, 30);          // Nose bridge
        draw_polyline(output, (*points)[i], 30, 35, true);    // Lower nose
        draw_polyline(output, (*points)[i], 36, 41, true);    // Left eye
        draw_polyline(output, (*points)[i], 42, 47, true);    // Right Eye
        draw_polyline(output, (*points)[i], 48, 59, true);    // Outer lip
        draw_polyline(output, (*points)[i], 60, 67, true);    // Inner lip
        //draw_polyline(output, (*points)[i], 68, 70);          // Forehead

        cv::circle(output, (*points)[i][8], 3, cv::Scalar(0,255,255), -1, 16);
        cv::circle(output, (*points)[i][29], 3, cv::Scalar(0,255,255), -1, 16);
        //cv::circle(output, (*points)[i][69], 3, cv::Scalar(0,255,255), -1, 16);

    }

    return matToJPG(&output);
} */

/*
 * CaptureFactory Constructor en Destructor
 */
CaptureFactory::CaptureFactory(){
    mfh = new CaptureFactory::CaptureFactoryHandler(*this);
    pose_model = new dlib::shape_predictor();
	(*syslog) << "Reading in shape predictor..." << endl;
    deserialize("theaterwagen/shape_predictor_68_face_landmarks.dat") >> *pose_model;
    (*syslog) << "Done reading in shape predictor ..." << endl;
	server->addHandler("/capturefactory", mfh);
	camMat = new std::vector<cv::Mat>();
	camPoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();
	load();
	std::thread t1( [this] {
		setenv("DISPLAY",":0.0",0);
//		this->window = new sf::RenderWindow(sf::VideoMode(1024, 768), "RenderWindow",sf::Style::Fullscreen);
		//this->window = new sf::RenderWindow(sf::VideoMode(1024, 768), "RenderWindow");
		//sf::RenderWindow window(sf::VideoMode(640, 480), "RenderWindow");
		sf::VideoMode desktop = sf::VideoMode().getDesktopMode();
		this->window = new sf::RenderWindow(desktop, "Theaterwagen", sf::Style::None);
	    window->setMouseCursorVisible(false);
	    window->setVerticalSyncEnabled(true);
	    //window->setFramerateLimit(10);
	    //window->setActive(false);
		renderingThread(window); } );
	//mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();
}

CaptureFactory::~CaptureFactory(){
	delete mfh;
	delete pose_model;
	delete camMat;
	delete camPoints;
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();
	if (it != capturemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    capturemap.erase(it);
	}
}

/*
 * CaptureFactoryHandler Constructor en Destructor
 */
CaptureFactory::CaptureFactoryHandler::CaptureFactoryHandler(CaptureFactory& capturefactory):capturefactory(capturefactory){
}


CaptureFactory::CaptureFactoryHandler::~CaptureFactoryHandler(){
}

/*
 * Capture Constructor en Destructor
 */
CaptureFactory::Capture::Capture(CaptureFactory& cf, std::string naam, std::string omschrijving):cf(cf){
	mh = new CaptureFactory::Capture::CaptureHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->cap = new cv::VideoCapture();

	filePoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();
	file2Points = new std::vector<std::vector<std::vector<cv::Point2f>>>();

	//cv::Mat boodschap(1024,768,CV_8UC3,cv::Scalar(255,255,255));
	//cv::putText(boodschap, "Gezichtsherkenningsmodel wordt geladen!", Point2f(100,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
	//std::unique_lock<std::mutex> l(m);
	//this->manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&boodschap).str() << "\r\n--frame\r\n";
	//l.unlock();
	//detector = new dlib::frontal_face_detector();

	std::thread t1( [this] { loadModel(); } );
	mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();

	std::stringstream ss;
	ss << "/capture-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CaptureFactory::Capture::Capture(CaptureFactory& cf, std::string uuidstr, std::string naam,
		                         std::string omschrijving, std::string filename, std::string filename2,
								 std::vector<std::vector<std::vector<cv::Point2f>>>* filepoints,
								 std::vector<std::vector<std::vector<cv::Point2f>>>* file2points,
								 bool fileonly,
								 unsigned int mix_file,
								 unsigned int mix_file2):cf(cf){
	mh = new CaptureFactory::Capture::CaptureHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->cap = new cv::VideoCapture();
	this->filename = filename;
	this->filename2 = filename2;
	this->filePoints = filepoints;
	this->file2Points = file2points;
	this->fileonly = fileonly;
	this->mix_file = mix_file;
	this->mix_file2 = mix_file2;

	//cv::Mat boodschap(1024,768,CV_8UC3,cv::Scalar(255,255,255));
	//cv::putText(boodschap, "Gezichtsherkenningsmodel wordt geladen!", Point2f(100,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
	//std::unique_lock<std::mutex> l(m);
	//this->manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&boodschap).str() << "\r\n--frame\r\n";
	//l.unlock();
	//detector = new dlib::frontal_face_detector();

	std::thread t1( [this] { loadModel(); } );
	mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();

	std::stringstream ss;
	ss << "/capture-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CaptureFactory::Capture::~Capture(){
	delete mh;
	delete cap;
	delete filePoints;
	//delete detector;
}

/*
 * Capture Handler Constructor en Destructor
 */
CaptureFactory::Capture::CaptureHandler::CaptureHandler(CaptureFactory::Capture& capture):capture(capture){
}

CaptureFactory::Capture::CaptureHandler::~CaptureHandler(){
}


/* overige functies
 *
 */

void CaptureFactory::renderingThread(sf::RenderWindow *window)
{
	sf::Image image;
	sf::Texture texture;
	sf::Sprite sprite;
	sfe::Movie movie;

	// the rendering loop
    while (window->isOpen())
    {
    	if (loadme)
    	{
    		try
		    {
    			if (movie.openFromFile(on_screen))
    			{
    				movie.play();
    				loaded = true;
    				loadme = false;
    			}
    			else
    			{
    				(*syslog) << "Videofile kon niet geopend worden!" << endl;
    			}
    		}
    		catch( cv::Exception& e )
    		{
    		  	(*syslog) << "Error loading merged mp4: " << e.msg << endl;
    		}

    	}
		window->clear(sf::Color::Black);
    	if (loaded)
    	{
        	if (movie.getStatus() == sfe::Status::Stopped)
        		movie.play();
			movie.update();
			movie.fit(0,0,window->getSize().x,window->getSize().y, false);
	   		window->draw(movie);
    	}
 		window->display();

 	   /*
       std::unique_lock<std::mutex> l(m_screen);
       if (on_screen->size() > 0)
       {

   		for (unsigned int i = 0; i < on_screen->size(); ++i)
    	{

    		const std::string tmp = manipulated.str();
    		std::ostringstream result;
    		result << std::setw(2) << std::setfill('0') << std::hex << std::uppercase;
    		std::copy(tmp.begin(), tmp.begin()+8, std::ostream_iterator<unsigned int>(result, " "));
    		std::cout << ":" << result.str() << std::endl;
    		const char* p = tmp.c_str();
    		int size = tmp.size();
    		cout << "size: " << size << endl;
    		image.create(1024,768,sf::Color(0,0,0));

   			//const std::string& tmp = ;
   			//const char* cstr = tmp.c_str();
       	    //image.loadFromMemory(cstr,tmp.length());

    		image.loadFromMemory(manipulated.str().c_str() + std::string("Content-Type: image/jpeg\r\n\r\n").length(),
    				             manipulated.str().length() - std::string("Content-Type: image/jpeg\r\n\r\n\r\n--frame\r\n").length());

       		if (!texture.loadFromMemory((*on_screen)[i].data(),(*on_screen)[i].size()))
       		{
       			break;

   			if (!texture.loadFromFile("resources/merged.avi"))
   			{
   				break;
   			}
       		//texture.setSmooth(false);
       		//sprite.setTexture(texture);
       		//sprite.setPosition(0,0);
    	}
       }
       else
       {
          window->display();
       }
       l.unlock();
       */

		sf::Event event;
        while (window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window->close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    *(syslog) << "the escape key was pressed" << std::endl;
                    window->close();
                }
            }
        }
    }
}


void CaptureFactory::load(){
	for (std::pair<std::string, CaptureFactory::Capture*> element  : capturemap)
	{
		deleteCapture(element.first);
	}

	char filename[] = CONFIG_FILE_CAPTURE;
	std::fstream file;
	file.open(filename, std::fstream::in | std::fstream::out | std::fstream::app);
	/* als bestand nog niet bestaat, dan leeg aanmaken */
	if (!file)
	{
		file.open(filename,  std::fstream::in | std::fstream::out | std::fstream::trunc);
        file <<"\n";
        file.close();
	}
	else file.close();

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_CAPTURE);

	if (node[0]["scaleFactor"])
		scaleFactor = node[0]["scaleFactor"].as<double>();
	if (node[0]["minNeighbors"])
		minNeighbors = node[0]["minNeighbors"].as<int>();
	if (node[0]["minSizeX"])
		minSizeX = node[0]["minSizeX"].as<int>();
	if (node[0]["minSizeY"])
		minSizeY = node[0]["minSizeY"].as<int>();

	//we start this loop at 1 to skip previous node
	for (std::size_t i=1;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		std::string filename = node[i]["filename"].as<std::string>();
		std::string filename2 = node[i]["filename2"].as<std::string>();
		bool fileonly = node[i]["fileonly"].as<bool>();
		unsigned int mix_file = (unsigned int) node[i]["mix_file"].as<int>();
		unsigned int mix_file2 = (unsigned int) node[i]["mix_file2"].as<int>();
		std::vector<std::vector<std::vector<cv::Point2f>>>* filepoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();
		for (std::size_t frame=0;frame < node[i]["filepoints"].size();frame++)
		{
		  std::vector<std::vector<cv::Point2f>> faces;
		  for(std::size_t face=0;face < node[i]["filepoints"][frame].size();face++)
		  {
			 std::vector<cv::Point2f> points;
			 for(std::size_t point=0;point < node[i]["filepoints"][frame][face].size();point++)
			 {
				 float x = node[i]["filepoints"][frame][face][point]["x"].as<float>();
				 float y = node[i]["filepoints"][frame][face][point]["y"].as<float>();
				 Point2f point2f(x,y);
				 points.push_back(point2f);
			 }
			 faces.push_back(points);
		  }
		  filepoints->push_back(faces);
		}
		std::vector<std::vector<std::vector<cv::Point2f>>>* file2points = new std::vector<std::vector<std::vector<cv::Point2f>>>();
		for (std::size_t frame=0;frame < node[i]["file2points"].size();frame++)
		{
		  std::vector<std::vector<cv::Point2f>> faces;
		  for(std::size_t face=0;face < node[i]["file2points"][frame].size();face++)
		  {
			 std::vector<cv::Point2f> points;
			 for(std::size_t point=0;point < node[i]["file2points"][frame][face].size();point++)
			 {
				 float x = node[i]["file2points"][frame][face][point]["x"].as<float>();
				 float y = node[i]["file2points"][frame][face][point]["y"].as<float>();
				 Point2f point2f(x,y);
				 points.push_back(point2f);
			 }
			 faces.push_back(points);
		  }
		  file2points->push_back(faces);
		}
		CaptureFactory::Capture * capture = new CaptureFactory::Capture(*this, uuidstr, naam, omschrijving, filename, filename2, filepoints, file2points, fileonly, mix_file, mix_file2);
		std::string uuid_str = capture->getUuid();
		capturemap.insert(std::make_pair(uuid_str,capture));
	}
}

void CaptureFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_CAPTURE);
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();

	emitter << YAML::BeginSeq;
	emitter << YAML::BeginMap;
	emitter << YAML::Key << "scaleFactor";
	emitter << YAML::Value << scaleFactor;
	emitter << YAML::Key << "minNeighbors";
	emitter << YAML::Value << minNeighbors;
	emitter << YAML::Key << "minSizeX";
	emitter << YAML::Value << minSizeX;
	emitter << YAML::Key << "minSizeY";
	emitter << YAML::Value << minSizeY;
	emitter << YAML::EndMap;

	for (std::pair<std::string, CaptureFactory::Capture*> element  : capturemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "filename";
		emitter << YAML::Value << element.second->filename;
		emitter << YAML::Key << "filename2";
		emitter << YAML::Value << element.second->filename2;
		emitter << YAML::Key << "fileonly";
		emitter << YAML::Value << element.second->fileonly;
		emitter << YAML::Key << "mix_file";
    	emitter << YAML::Value << (int) element.second->mix_file;
    	emitter << YAML::Key << "mix_file2";
		emitter << YAML::Value << (int) element.second->mix_file2;
		emitter << YAML::Key << "filepoints";
		emitter << YAML::BeginSeq;
		/* frames */
		for (unsigned int frame = 0; frame < (*element.second->filePoints).size() ; frame++)
		{
		  /* faces */
	      emitter << YAML::BeginSeq;
		  for (unsigned int face = 0; face < (*element.second->filePoints)[frame].size() ; face++)
		  {
		     /* points */
             emitter << YAML::Flow;
			 emitter << YAML::BeginSeq;
			 for (unsigned int point = 0; point < (*element.second->filePoints)[frame][face].size(); point++)
			 {
				 emitter << YAML::BeginMap;
				 emitter << YAML::Key << "x";
				 emitter << YAML::Value << (*element.second->filePoints)[frame][face][point].x;
				 emitter << YAML::Key << "y";
				 emitter << YAML::Value << (*element.second->filePoints)[frame][face][point].y;
				 emitter << YAML::EndMap;
			 }
			 emitter << YAML::EndSeq;
		  }
		  emitter << YAML::EndSeq;
		}
		emitter << YAML::EndSeq;
		emitter << YAML::Key << "file2points";
		emitter << YAML::BeginSeq;
		/* frames */
		for (unsigned int frame = 0; frame < (*element.second->file2Points).size() ; frame++)
		{
		  /* faces */
	      emitter << YAML::BeginSeq;
		  for (unsigned int face = 0; face < (*element.second->file2Points)[frame].size() ; face++)
		  {
		     /* points */
             emitter << YAML::Flow;
			 emitter << YAML::BeginSeq;
			 for (unsigned int point = 0; point < (*element.second->file2Points)[frame][face].size(); point++)
			 {
				 emitter << YAML::BeginMap;
				 emitter << YAML::Key << "x";
				 emitter << YAML::Value << (*element.second->file2Points)[frame][face][point].x;
				 emitter << YAML::Key << "y";
				 emitter << YAML::Value << (*element.second->file2Points)[frame][face][point].y;
				 emitter << YAML::EndMap;
			 }
			 emitter << YAML::EndSeq;
		  }
		  emitter << YAML::EndSeq;
		}
		emitter << YAML::EndSeq;

		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void CaptureFactory::Capture::openCap(captureType type)
{
	if (type == CAP_CAM)
	{
		if(!cap->isOpened()){
			cap->open(CAP_GPHOTO2); // connect to the camera
			if(!cap->isOpened())
				{
					if (first_open)
					{
						(*syslog) << "Could not open SLR camera, retrying with webcam." << endl;
						first_open = false;
					}
					cap->open(0); //if no camera, try webcam on usb0
				}
			// if camera found output all of its options
			else
			if (first_open)
			{
				(*syslog) << (const char*)((intptr_t)cap->get(CV_CAP_PROP_GPHOTO2_WIDGET_ENUMERATE)) << endl;
				first_open = false;
			}
			cap->set(CAP_PROP_FOURCC ,CV_FOURCC('M', 'J', 'P', 'G') );
			cap->set(CAP_PROP_FRAME_WIDTH,2304);   // width pixels 2304
			cap->set(CAP_PROP_FRAME_HEIGHT,1296);   // height pixels 1296
		}
	}
	else if (type == CAP_FILE)
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(filename);
		}
	}
	else if (type == CAP_FILE2)
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(filename2);
		}
	}
}

void CaptureFactory::Capture::closeCap(){
	if(cap->isOpened()){   // connect to the camera
		cap->release();
    }

}

void CaptureFactory::Capture::loadModel(){
	//delete detector;
	//this->detector = new dlib::frontal_face_detector(get_frontal_face_detector());
	this->face_cascade = new CascadeClassifier();
	if( !(*face_cascade).load("theaterwagen/haarcascade_frontalface_alt.xml") ){ printf("--(!)Error loading face cascade\n"); return; };
    model_loaded = true;
}

void CaptureFactory::Capture::captureDetectAndMerge()
{
	std::thread t1( [this] {
	std::unique_lock<std::mutex> l2(m_merging);
	openCap(CAP_CAM);
	cv::Mat captured;
	std::unique_lock<std::mutex> l(m);
	cf.camPoints->clear();
	cf.camMat->clear();
	for (int i = 0; i < 6; i++)
	{
		captured = captureFrame(CAP_CAM);

		if (captured.empty()) continue;

		std::vector<std::vector<cv::Point2f>> points = detectFrame(&captured, CAP_CAM);
		if (points.size() == 0)
			continue;
		else
		{
			/* add points of found faces */
			cf.camMat->push_back(captured);
			cf.camPoints->push_back(points);
			break;
		}
	}

	closeCap();
	if (cf.camPoints->empty())
	{
		loadFilmpje();
		l.unlock();
		l2.unlock();
		return;
	}

	mergeFrames();

	l.unlock();
	l2.unlock();
	return;
	});
	t1.detach();
}

void CaptureFactory::Capture::mergeToScreen()
{
	std::thread t1( [this] {
	std::unique_lock<std::mutex> l2(m_merging);
	std::unique_lock<std::mutex> l(m);

	if (!fileonly)
	{
		mergeFrames();
	}

	l.unlock();
	l2.unlock();
	return;
	});
	t1.detach();
}

void CaptureFactory::Capture::mergeToFile()
{
	std::thread t1( [this] {
	std::unique_lock<std::mutex> l2(m_merging);
	std::unique_lock<std::mutex> l(m);

	if (fileonly)
	{
		mergeFrames();
	}

	l.unlock();
	l2.unlock();
	return;
	});
	t1.detach();
}

void CaptureFactory::Capture::loadFilmpje()
{
    openCap(CAP_FILE);
	cv::Mat frame;
	std::vector<std::string> jpg;
	for (;;)
	{
	   frame = captureFrame(CAP_FILE);
	   if (frame.empty()) break;
	    jpg.push_back(matToJPG(&frame));
	}
	closeCap();
}

void CaptureFactory::clearScreen()
{
	 loaded = false;
	 loadme = false;
}

void CaptureFactory::Capture::onScreen()
{
	 delay(200);
	 std::unique_lock<std::mutex> l(m_merging);
	 cf.on_screen = TMP_DIR + this->getUuid() + ".mp4";
	 cf.loaded = false;
	 cf.loadme = true;
	 l.unlock();
	 /*
	 delay(200);
	 std::unique_lock<std::mutex> l3(m_merging);
	 std::unique_lock<std::mutex> l(m_screen);
	 std::unique_lock<std::mutex> l2(m);
	 delete cf.on_screen;
	 cf.on_screen = new std::vector<std::vector<char>>();
	 for (unsigned int i = 0; i < off_screen->size(); ++i)
	 {
		 const char *cstr = (*off_screen)[i].str().c_str();
		 std::vector<char> tmp(cstr, cstr + (*off_screen)[i].str().size());
		 cf.on_screen->push_back(tmp);
	 }
	 this->off_screen = new std::vector<std::stringstream>();
	 l.unlock();
	 l2.unlock();
	 l3.unlock();
	 */
}

void CaptureFactory::Capture::getFrame(int framenumber, bool draw){
	std::string photo = TMP_DIR + this->getUuid() + "_file.jpg";
	std::vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(95);
    std::unique_lock<std::mutex> l(m);
	openCap(CAP_FILE);
	this->cap->set(CV_CAP_PROP_POS_FRAMES, framenumber - 1);
	cv::Mat input;
	*cap >> input;
	closeCap();
	if (draw)
		input = drawFaces(&input,&((*filePoints)[framenumber - 1]));
	imwrite(photo.c_str(), input, compression_params);
	l.unlock();
}

cv::Mat CaptureFactory::Capture::captureFrame(captureType capturetype){
    // Grab a frame
	cv::Mat input;
	try
	{
		*cap >> input;
		if (!input.empty())
		{
			cv::resize(input,input,cv::Size(VIDEO_WIDTH,VIDEO_HEIGHT));
		}
		if (capturetype == CAP_CAM)
		{
			std::string photo = TMP_DIR + this->getUuid() + "_photo.jpg";
			std::vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
			compression_params.push_back(95);
			imwrite(photo.c_str(), input, compression_params);
		}
	}
	catch( cv::Exception& e )
	{
	    const char* err_msg = e.what();
	    (*syslog) << "exception caught: " << err_msg << std::endl;
	}
	return input;
    /*
     * <img alt="Embedded Image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA..." />
     */
}

std::vector<std::vector<cv::Point2f>> CaptureFactory::Capture::detectFrame(cv::Mat* input, captureType capturetype)
{
       std::vector<std::vector<cv::Point2f>> points;
       cv::Mat im_small;
       // Resize image for face detection
       try
       {
        cv::resize(*input, im_small, cv::Size(), 1.0/FACE_DOWNSAMPLE_RATIO, 1.0/FACE_DOWNSAMPLE_RATIO);
       }
       catch( cv::Exception& e )
       {
  	    const char* err_msg = e.what();
        (*syslog) << "exception caught in resize: " << err_msg << std::endl;
        return points;
       }

       //std::vector<dlib::rectangle> faces;
       std::vector<cv::Rect> faces;
       // Detect faces
       cv_image<bgr_pixel> img(*input);
       cv_image<bgr_pixel> cimg(im_small);
       //faces = (*detector)(cimg);
       cv::UMat reeds;
       cvtColor( *input, reeds, CV_BGR2GRAY);
       equalizeHist( reeds, reeds );
       (*face_cascade).detectMultiScale( reeds, faces, cf.scaleFactor, cf.minNeighbors, 0|CV_HAAR_SCALE_IMAGE, Size(cf.minSizeX, cf.minSizeY) );
       if (faces.size() == 0)
       {
    	 return points;
       }
       // Find the pose of each face.
       std::vector<full_object_detection> shapes;
       full_object_detection shape;
       /*
       for (unsigned long i = 0; i < faces.size(); ++i)
       {
     	// Resize obtained rectangle for full resolution image.
     	     dlib::rectangle r(
     	                   (long)(faces[i].left() * FACE_DOWNSAMPLE_RATIO),
     	                   (long)(faces[i].top() * FACE_DOWNSAMPLE_RATIO),
    	                   (long)(faces[i].right() * FACE_DOWNSAMPLE_RATIO),
    	                   (long)(faces[i].bottom() * FACE_DOWNSAMPLE_RATIO)
     	                ); */

       for (unsigned long i = 0; i < faces.size(); ++i)
              {
        dlib::rectangle r = openCVRectToDlib(faces[i]);

    	// Landmark detection on full sized image
    	std::unique_lock<std::mutex> l(m_pose);
    	shape = (*cf.pose_model)(img, r);
    	l.unlock();
        //shapes.push_back(pose_model(cimg, faces[i]));
        shapes.push_back(shape);

       }

       //Read points
       for (unsigned long i = 0; i < faces.size(); ++i)
       {
    	   points.push_back(get_points(shapes[i]));
       }

       //sort all found faces in frame by face with lowest x to highest x (and lowest y to highest y if x values are the same)
       std::sort(points.begin(), points.end(), [](const std::vector<cv::Point2f> &a, const std::vector<cv::Point2f> &b) {
           return (a[0].x == b[0].x ? a[0].y < b[0].y: a[0].x < b[0].x);
       });

		if (capturetype == CAP_CAM)
		{
			std::string photo = TMP_DIR + this->getUuid() + "_photo.jpg";
			std::vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
			compression_params.push_back(95);
			imwrite(photo.c_str(), drawFaces(input,&points), compression_params);
		}

       return points;
}

std::vector<std::string> CaptureFactory::Capture::morph(cv::Mat orig, cv::Mat target, unsigned int morphsteps)
{
      std::vector<std::string> totaal;

      cv::Mat resultaat;
	  unsigned int step = (unsigned int) 100 / morphsteps;

	  for (unsigned int i = 0; i < morphsteps; i++)
	  {
		  float value = ((float)step * (float)i) / 100.0;
		  addWeighted(orig, 1.0 - value, target, value, 0.0, resultaat);
		  totaal.push_back(matToJPG(&resultaat));
	  }

	  return totaal;
}

std::vector<std::string> CaptureFactory::Capture::mergeFaces(captureType type, unsigned int mix_file, std::vector<std::vector<std::vector<cv::Point2f>>>* filePoints, bool fileonly)
{
	std::vector<std::string> totaal;

	openCap(type);

	cv::Mat img_file;

	/*
	 * if we have no filepoints (faces) in the movie we just copy it asis
	*/
	if ((*filePoints).size() == 0) {
		while(true)
		{
			img_file = captureFrame(type);
			if (img_file.empty()) break;

			totaal.push_back(matToJPG(&img_file));
		}
		//closeCap();
		//return totaal;
	}
	else
	//if we have no campoints (faces) at all we just copy the input
	if ((*cf.camPoints).size() == 0) {
		while(true)
		{
			img_file = captureFrame(type);
			if (img_file.empty()) break;

			totaal.push_back(matToJPG(&img_file));
		}
		//closeCap();
		//return totaal;
	}
	else
	for (unsigned int frame = 0; frame < (*filePoints).size(); ++frame)
	{
		//delete img_file;
		img_file = captureFrame(type);
		if (img_file.empty()) break;

		//cv::Mat img_orig = img_file.clone();

		//cv::resize(img_file,img_file,cv::Size(1024,768));
		//remove frame to keep memory available
		//(*fileMat).erase((*fileMat).begin() + frame);

		cv::Mat resultaat = img_file.clone();

		cv::Mat img_cam = (*cf.camMat)[frame % ((*cf.camMat).size())].clone();

		for (unsigned int gezicht = 0; (fileonly ? gezicht < (*cf.camPoints)[frame].size() : gezicht < (*filePoints)[frame].size()); ++gezicht)
		{
			// if we have less faces in the capture than in the img_file for this frame we do nothing

			if (!fileonly && gezicht >= (*cf.camPoints)[frame % ((*cf.camPoints).size())].size()) continue;

			//convert Mat to float data type
	        img_cam.convertTo(img_cam, CV_32F);
	        resultaat.convertTo(resultaat, CV_32F);

	        // Find convex hull
	        std::vector<Point2f> hull1;
	        std::vector<Point2f> hull2;
	        std::vector<int> hullIndex;

	        try
	        {
	         convexHull((*filePoints)[frame][(fileonly == true ? 0 : gezicht)], hullIndex, false, false);
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	   	     (*syslog) << "exception caught in convexHull: " << err_msg << std::endl;
    	     return totaal;
	      	}

	        for(int i = 0; i < (int)hullIndex.size(); i++)
	        {
	            hull1.push_back((*cf.camPoints)[frame % ((*cf.camPoints).size())][gezicht % ((*cf.camPoints)[frame % ((*cf.camPoints).size())].size())][hullIndex[i]]);
	            hull2.push_back((*filePoints)[frame][(fileonly ? 0 : gezicht)][hullIndex[i]]);
	        }


	        // Find delaunay triangulation for points on the convex hull
	        std::vector< std::vector<int> > dt;
	        cv::Rect rect(0, 0, resultaat.cols, resultaat.rows);
	        try
	        {
	         calculateDelaunayTriangles(rect, hull2, dt);
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	   	     (*syslog) << "exception caught in calculateDelauneyTriangles: " << err_msg << std::endl;
	     	 return totaal;
	      	}

	        // Apply affine transformation to Delaunay triangles
	        for(size_t i = 0; i < dt.size(); i++)
	        {
	           std::vector<Point2f> t1, t2;
	           // Get points for img1, img2 corresponding to the triangles
	           for(size_t j = 0; j < 3; j++)
	           {
	        	  t1.push_back(hull1[dt[i][j]]);
	        	  t2.push_back(hull2[dt[i][j]]);
	        	}
	           try
	           {
                warpTriangle(img_cam, resultaat, t1, t2);
	           }
	           catch( cv::Exception& e )
		       {
		   	    const char* err_msg = e.what();
		   	    (*syslog) << "exception caught in warpTriangle: " << err_msg << std::endl;
		    	return totaal;
		       }
	       	}

	        resultaat.convertTo(resultaat, CV_8UC3);


	        // Calculate mask
	        std::vector<Point> hull8U;
	        for(int i = 0; i < (int)hull2.size(); i++)
	        {
	            Point pt(hull2[i].x, hull2[i].y);
	            hull8U.push_back(pt);
	        }

	        cv::Mat mask = Mat::zeros(img_file.rows, img_file.cols, img_file.depth());

	        try
	        {
	         fillConvexPoly(mask,&hull8U[0], hull8U.size(), Scalar(255,255,255));
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	   	     (*syslog) << "exception caught in fillConvexPoly: " << err_msg << std::endl;
          	 return totaal;
	      	}

            feather_amount.width = feather_amount.height = (int)cv::norm((*filePoints)[frame][(fileonly ? 0 : gezicht)][0] - (*filePoints)[frame][(fileonly ? 0 : gezicht)][16]) / 8;
            featherMask(mask);

	        // Clone seamlessly.
	        cv::Rect r = boundingRect(hull2);
	        //Point center = (r.tl() + r.br()) / 2;
	        //Point centertest = Point(img_file(r).cols / 2,img_file(r).rows / 2);

	        //cv::Mat orig = img_file(r).clone();

	        try
	        {
	        /*
	         cv::UMat imgtest1 = img_file(r).getUMat(cv::ACCESS_READ);
	         cv::UMat imgtest2 = resultaat(r).getUMat(cv::ACCESS_READ);
	         cv::UMat masktest = mask(r).getUMat(cv::ACCESS_READ);
	         cv::UMat output; */
		     cv::Mat facefromcam = resultaat(r);
		     cv::Mat maskfromface = mask(r);
		     cv::Mat output = img_file(r);
		     specifyHistogram(output, facefromcam, maskfromface);
	         //cv::seamlessClone(imgtest2,imgtest1, masktest, centertest, output, NORMAL_CLONE);
	         pasteFacesOnFrame(output, facefromcam, maskfromface);
		     output.copyTo(resultaat(r));
             addWeighted(resultaat, mix_file / 100.0, img_file, 1.0 - mix_file / 100.0, 0.0, resultaat);
	        }
            catch( cv::Exception& e )
	      	{
             const char* err_msg = e.what();
	   	     (*syslog) << "exception caught in seamlessClone: " << err_msg << std::endl;
	     	 return totaal;
	      	}
      	    if (fileonly)
      	    {
      		  std::vector<int> compression_params;
      	      compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
      	      compression_params.push_back(95);
      	      std::stringstream filename;
      	      filename << CAPTURE_DIR << std::time(0) << gezicht << ".jpg";
      	      imwrite(filename.str().c_str(), resultaat, compression_params);
      		  //TODO Push back hier nodig??
      	      //totaal.push_back(matToJPG(&resultaat));
      	    }
		}

	/*
	  float per_frame = ((float) mix_to - (float) mix_from) / (float)(*filePoints).size();
			  float mix_frame = ((float) mix_from + (per_frame * (frame + 1))) / 100;
			  addWeighted(resultaat, mix_frame, img_file2, 1 - mix_frame, 0.0, resultaat);
	    {
		  //we fill out 40 frames by default
		  if ((*filePoints).size() == 1)
		  {
			  float per_frame = ((float) mix_to - (float) mix_from) / 40;
			  for (int i = 0; i < 40; i++)
			  {
				  float mix_frame = ((float) mix_from + (per_frame * (frame + 1 + i))) / 100;
                  addWeighted(img_file, mix_frame, img_file2, 1 - mix_frame, 0.0, resultaat);
				  record.write(resultaat);
			  }
			  for (int i = 0;i < 60; i++)
			  {
				  record.write(resultaat);
			  }
		  }
		  else
			  record.write(resultaat);
		  totaal.push_back(matToJPG(&resultaat));
	   }
	  }
	  */
	  totaal.push_back(matToJPG(&resultaat));
	}

	closeCap();

	return totaal;
}

void CaptureFactory::Capture::mergeFrames()
{

	std::vector<std::string> totaal;
	std::vector<std::string> file;
	std::vector<std::string> file2;
	std::vector<std::string> morph;

	high_resolution_clock::time_point t1, t2;

	//TODO look at scheduling to see if we can run and merge in background
	//we should not merge to a running video
	//TODO refactor into seperate method
	this->cf.clearScreen();

	t1 = high_resolution_clock::now();
	file = mergeFaces(CAP_FILE, this->mix_file, this->filePoints, this->fileonly);
	t2 = high_resolution_clock::now();

	auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	std::cout << "merging file took: " << int_ms.count() << " milliseconds" << endl;

	if (this->filename2.compare("") != 0)
	{
		t1 = high_resolution_clock::now();
		file2 = mergeFaces(CAP_FILE2, this->mix_file2, this->file2Points, false);
		t2 = high_resolution_clock::now();

		auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "merging file2 took: " << int_ms.count() << " milliseconds" << endl;

		t1 = high_resolution_clock::now();
		morph = this->morph(ImgToMat(&file[file.size()-1]),ImgToMat(&file2[0]),this->morphsteps);
		t2 = high_resolution_clock::now();

		int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "morphing file and file2 took: " << int_ms.count() << " milliseconds" << endl;

		while (file.size() % VIDEO_FPS != 0) file.push_back(file[(file.size()-1)]);
		std::copy(file.begin(), file.end(), std::back_inserter(totaal));
		while (morph.size() % VIDEO_FPS != 0) morph.push_back(morph[(morph.size()-1)]);
		std::copy(morph.begin(), morph.end(), std::back_inserter(totaal));
		while (file2.size() % VIDEO_FPS != 0) file2.push_back(file2[(file2.size()-1)]);
		std::copy(file2.begin(), file2.end(), std::back_inserter(totaal));
	}
	else
	{
		while (file.size() % VIDEO_FPS != 0) file.push_back(file[(file.size()-1)]);


		/*
		for (unsigned int i = 0; i < 10; i++)
		{
			file.push_back(file[(file.size()-1)]);
		}*/

		for (unsigned int i = 0; i < file.size(); i++)
		{
			std::copy(file.begin(), file.end(), std::back_inserter(totaal));
		}
	}

	fprintf(stderr,"starting encoding...\n");
    std::string recordname = TMP_DIR + this->getUuid() + ".mp4";
    /*
	  VideoWriter record(recordname, CV_FOURCC('M','P','4','V'),
			  MOVIE_FRAMERATE, cv::Size(1024,768), true); */

	t1 = high_resolution_clock::now();

    // initialize FFmpeg library
    av_register_all();
	av_log_set_level(AV_LOG_DEBUG);
    int ret;

    const int dst_width = VIDEO_WIDTH;
    const int dst_height = VIDEO_HEIGHT;
    const AVRational dst_fps = {VIDEO_FPS, 1};

    // allocate cv::Mat with extra bytes (required by AVFrame::data)
    std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16);
    cv::Mat image(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);

    // open output format context
    AVFormatContext* outctx = nullptr;
    ret = avformat_alloc_output_context2(&outctx, nullptr, nullptr, recordname.c_str());
    if (ret < 0) {
        std::cerr << "fail to avformat_alloc_output_context2(" << recordname.c_str() << "): ret=" << ret;
        return;
    }

    // open output IO context
    ret = avio_open2(&outctx->pb, recordname.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avio_open2: ret=" << ret;
        return;
    }

    // create new video stream
    AVCodec* vcodec = avcodec_find_encoder(outctx->oformat->video_codec);
    //AVCodec* vcodec = avcodec_find_encoder_by_name("h264_omx");
    if (!vcodec)
    {
    	fprintf(stderr,"Kan codec nie vinden nie \n");
    	return;
    }
    AVStream* vstrm = avformat_new_stream(outctx, vcodec);
    if (!vstrm) {
        std::cerr << "fail to avformat_new_stream";
        return;
    }

    avcodec_get_context_defaults3(vstrm->codec, vcodec);
    vstrm->codec->width = dst_width;
    vstrm->codec->height = dst_height;
    vstrm->codec->pix_fmt = vcodec->pix_fmts[0];
    vstrm->codec->time_base = vstrm->time_base = av_inv_q(dst_fps);
    vstrm->r_frame_rate = vstrm->avg_frame_rate = dst_fps;
    //vstrm->codec->delay = 0;
    //vstrm->codec->max_b_frames = 0;
    //vstrm->codec->thread_count = 1;

   	av_opt_set(vstrm->codec->priv_data, "preset", "ultrafast", 0);
   	av_opt_set(vstrm->codec->priv_data, "tune", "fastdecode", 0);

    //vstrm->codec->flags &= ~AV_CODEC_CAP_DELAY;
    fprintf(stderr,"Capabilities %i",vstrm->codec->codec->capabilities);
    if (outctx->oformat->flags & AVFMT_GLOBALHEADER)
        vstrm->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open video encoder
    ret = avcodec_open2(vstrm->codec, vcodec, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avcodec_open2: ret=" << ret;
        return;
    }

    std::cout
        << "outfile: " << recordname.c_str() << "\n"
        << "format:  " << outctx->oformat->name << "\n"
        << "vcodec:  " << vcodec->name << "\n"
        << "size:    " << dst_width << 'x' << dst_height << "\n"
        << "fps:     " << av_q2d(dst_fps) << "\n"
        << "pixfmt:  " << av_get_pix_fmt_name(vstrm->codec->pix_fmt) << "\n"
        << std::flush;

    // initialize sample scaler
    SwsContext* swsctx = sws_getCachedContext(
        nullptr, dst_width, dst_height, AV_PIX_FMT_BGR24,
        dst_width, dst_height, vstrm->codec->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx) {
        std::cerr << "fail to sws_getCachedContext";
        return;
    }

    // allocate frame buffer for encoding
    AVFrame* frame = av_frame_alloc();
    std::vector<uint8_t> framebuf(avpicture_get_size(vstrm->codec->pix_fmt, dst_width, dst_height));
    avpicture_fill(reinterpret_cast<AVPicture*>(frame), framebuf.data(), vstrm->codec->pix_fmt, dst_width, dst_height);
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = static_cast<int>(vstrm->codec->pix_fmt);

    // encoding loop
    avformat_write_header(outctx, nullptr);
    int64_t frame_pts = 0;
    unsigned nb_frames = 0;
    int got_pkt = 0;

    for (unsigned int i = 0; i < totaal.size(); i++) {
   		ImgToMat(&totaal[i]).copyTo(image);
        const int stride[] = { static_cast<int>(image.step[0]) };
        sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
        frame->pts = frame_pts++;
        // encode video frame
        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_video2(vstrm->codec, &pkt, frame, &got_pkt);
        fprintf(stderr, "image size is %i\n",pkt.size);
        if (ret < 0) {
            std::cerr << "fail to avcodec_encode_video2: ret=" << ret << "\n";
            break;
        }
        if (got_pkt) {
            // rescale packet timestamp
            pkt.duration = 1;
            av_packet_rescale_ts(&pkt, vstrm->codec->time_base, vstrm->time_base);
            // write packet
            av_write_frame(outctx, &pkt);
            std::cout << nb_frames << '\r' << std::flush;  // dump progress
            ++nb_frames;
            av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    do {
        // encode video frame
        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_video2(vstrm->codec, &pkt, NULL, &got_pkt);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_pkt) {
            // rescale packet timestamp
            pkt.duration = 1;
            av_packet_rescale_ts(&pkt, vstrm->codec->time_base, vstrm->time_base);
            // write packet
            av_write_frame(outctx, &pkt);
            std::cout << nb_frames << '\r' << std::flush;  // dump progress
            ++nb_frames;
            av_free_packet(&pkt);
        }
    } while (got_pkt);

    av_write_trailer(outctx);
    std::cout << nb_frames << " frames encoded" << std::endl;

    av_frame_free(&frame);
    avcodec_close(vstrm->codec);
    avio_close(outctx->pb);
    avformat_free_context(outctx);

    t2 = high_resolution_clock::now();
	int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	std::cout << "H264 encoding took: " << int_ms.count() << " milliseconds" << endl;
}


std::string CaptureFactory::Capture::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string CaptureFactory::Capture::getUrl(){
	return url;
}

CaptureFactory::Capture* CaptureFactory::addCapture(std::string naam, std::string omschrijving){
	CaptureFactory::Capture * capture = new CaptureFactory::Capture(*this, naam, omschrijving);
	std::string uuid_str = capture->getUuid();
	capturemap.insert(std::make_pair(uuid_str,capture));
	return capture;
}

void CaptureFactory::deleteCapture(std::string uuid){
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();
    it = capturemap.find(uuid);
	if (it != capturemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    capturemap.erase(it);
	}
}

void CaptureFactory::Capture::Stop(){
	delay(500);
}

void CaptureFactory::Capture::Start(){
	delay(500);
}

std::string CaptureFactory::Capture::getNaam(){
	return naam;
}

std::string CaptureFactory::Capture::getOmschrijving(){
	return omschrijving;
}

bool CaptureFactory::CaptureFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::CaptureFactoryHandler::handleAll("GET", server, conn);
	}

bool CaptureFactory::CaptureFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::CaptureFactoryHandler::handleAll("POST", server, conn);
	}

bool CaptureFactory::Capture::CaptureHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::Capture::CaptureHandler::handleAll("GET", server, conn);
	}

bool CaptureFactory::Capture::CaptureHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::Capture::CaptureHandler::handleAll("POST", server, conn);
	}

bool CaptureFactory::CaptureFactoryHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string dummy;
	std::string value;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "scaleFactor", dummy))
	{
		CivetServer::getParam(conn,"value", value);
		capturefactory.scaleFactor = atof(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "minNeighbors", dummy))
	{
		CivetServer::getParam(conn,"value", value);
		capturefactory.minNeighbors = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "minSizeX", dummy))
	{
		CivetServer::getParam(conn,"value", value);
		capturefactory.minSizeX = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "minSizeY", dummy))
	{
		CivetServer::getParam(conn,"value", value);
		capturefactory.minSizeY = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "delete", value))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/capturefactory\" />";
	   this->capturefactory.deleteCapture(value);
	}
	else
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
	   message = _("Saved!");
       this->capturefactory.save();
	}
	else
	if(CivetServer::getParam(conn, "load", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
	   message = _("Loaded!");
	   this->capturefactory.load();
	}
	else
	/* if parameter clear is present the clear screen button was pushed */
	if(CivetServer::getParam(conn, "clear", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
	   message = _("Screen empty!");
       this->capturefactory.clearScreen();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
	  CivetServer::getParam(conn, "naam", value);
	  std::string naam = value;
	  CivetServer::getParam(conn, "omschrijving", value);
	  std::string omschrijving = value;

      CaptureFactory::Capture* capture = capturefactory.addCapture(naam, omschrijving);

      meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + capture->getUrl() + "\" />";
	}

	std::stringstream ss;

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/capturefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << "</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << "</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" name=\"omschrijving\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";

	}
	/* initial page display */
	else
	{
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#scaleFactor').on('change', function() {";
		tohead << " $.get( \"captureFactory\", { scaleFactor: 'true', value: $('#scaleFactor').val() }, function( data ) {";
		tohead << "  $( \"#scaleFactor\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#minNeighbors').on('change', function() {";
		tohead << " $.get( \"captureFactory\", { minNeighbors: 'true', value: $('#minNeighbors').val() }, function( data ) {";
		tohead << "  $( \"#minNeighbors\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#minSizeX').on('change', function() {";
		tohead << " $.get( \"captureFactory\", { minSizeX: 'true', value: $('#minSizeX').val() }, function( data ) {";
		tohead << "  $( \"#minSizeX\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#minSizeY').on('change', function() {";
		tohead << " $.get( \"captureFactory\", { minSizeY: 'true', value: $('#minSizeY').val() }, function( data ) {";
		tohead << "  $( \"#minSizeY\" ).html( data );})";
	    tohead << "});";
		tohead << "});";
		tohead << "</script>";
		std::map<std::string, CaptureFactory::Capture*>::iterator it = capturefactory.capturemap.begin();
		for (std::pair<std::string, CaptureFactory::Capture*> element : capturefactory.capturemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"clear\" id=\"clear\">" << _("Clear Screen") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<br>";
	    ss << "<h2>";
	    ss << _("Capture sensitivity settings:");
	    ss << "</h2>";
		ss << "<label for=\"scaleFactor\">" << _("scaleFactor") << ":</label>"
					  "<input class=\"inside\" id=\"scaleFactor\" type=\"number\" min=\"0\" max=\"10\" placeholder=\"0.00\" step=\"any\" value=\"" <<
					  capturefactory.scaleFactor << "\" name=\"scaleFactor\"/>" << "<br>";
		ss << "<label for=\"minNeighbors\">" << _("minNeighbors") << ":</label>"
					  "<input class=\"inside\" id=\"minNeighbors\" type=\"number\" min=\"0\" max=\"10\" placeholder=\"0\" step=\"1\" value=\"" <<
					  capturefactory.minNeighbors << "\" name=\"minNeighbors\"/>" << "<br>";
		ss << "<label for=\"minSizeX\">" << _("minSizeX") << ":</label>"
					  "<input class=\"inside\" id=\"minSizeX\" type=\"number\" min=\"0\" max=\"100\" placeholder=\"0\" step=\"1\" value=\"" <<
					  capturefactory.minSizeX << "\" name=\"minSizeX\"/>" << "<br>";
		ss << "<label for=\"minSizeY\">" << _("minSizeY") << ":</label>"
					  "<input class=\"inside\" id=\"minSizeY\" type=\"number\" min=\"0\" max=\"100\" placeholder=\"0\" step=\"1\" value=\"" <<
					  capturefactory.minSizeY << "\" name=\"minSizeY\"/>" << "<br>";
	}

	ss = getHtml(meta, message, "capture",  ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn, ss.str().c_str(), "%s");

	return true;
}

bool CaptureFactory::Capture::CaptureHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string value;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		capture.naam = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "omschrijving", value))
	{
		CivetServer::getParam(conn,"value", value);
		capture.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "mix_file", value))
	{
		CivetServer::getParam(conn,"value", value);
		capture.mix_file = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "mix_file2", value))
	{
		CivetServer::getParam(conn,"value", value);
		capture.mix_file2 = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "filename", value))
	{
		CivetServer::getParam(conn,"value", value);
		capture.filename = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "filename2", value))
	{
		CivetServer::getParam(conn,"value", value);
		capture.filename2 = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "fileonly", value))
	{
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			capture.fileonly = true;
		else
			capture.fileonly = false;
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "faceonframe", value))
	{
		if (value.compare("true") == 0)
			{
				CivetServer::getParam(conn,"frames", value);
				capture.getFrame(atoi(value.c_str()),true);
			}
		else
			{
				CivetServer::getParam(conn,"frames", value);
				capture.getFrame(atoi(value.c_str()), false);
			}
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << "tmp/" << capture.getUuid() << "_file.jpg?t=" << std::time(0);
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "frames", value))
	{
		std::string faceonframe;
		CivetServer::getParam(conn,"faceonframe", faceonframe);
		if (faceonframe.compare("true") == 0)
					capture.getFrame(atoi(value.c_str()),true);
			else
					capture.getFrame(atoi(value.c_str()), false);
		capture.getFrame(atoi(value.c_str()),true);
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << "tmp/" << capture.getUuid() << "_file.jpg?t=" << std::time(0);
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "on_screen", dummy))
	{
		capture.onScreen();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("On Screen!");
	}
    if(CivetServer::getParam(conn, "merge", dummy))
	{
		std::unique_lock<std::mutex> l(m);
		capture.mergeFrames();
		l.unlock();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Merged!");
	}
	if(CivetServer::getParam(conn, "newmovie", value))
	{
		capture.filename = value;
		std::stringstream ss;

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + capture.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "newmovie2", value))
	{
		capture.filename2 = value;
		std::stringstream ss;

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + capture.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "save_video", dummy))
	{
		Size S = Size((*capture.cf.camMat)[0].cols,(*capture.cf.camMat)[0].rows);
		int codec = CV_FOURCC('H', '2', '6', '4');
		VideoWriter outputVideo(MOVIES_DIR "capture.mp4", codec, VIDEO_FPS, S, true);

		if (!(outputVideo.open(MOVIES_DIR "capture.mp4", codec, VIDEO_FPS, S, true)))
			(*syslog) << "Video open failure!" << endl;
		for (unsigned int i = 0; i < (*capture.cf.camMat).size(); ++i)
		{
			outputVideo << (*capture.cf.camMat)[i];
		}
		outputVideo.release();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Video saved!");
	}
	if(CivetServer::getParam(conn, "capture", dummy))
	{
		capture.openCap(CAP_CAM);
		capture.cf.camPoints->clear();
		capture.cf.camMat->clear();
		capture.cf.camMat->push_back(capture.captureFrame(CAP_CAM));
		capture.closeCap();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Frame Captured!");
	}
	if(CivetServer::getParam(conn, "capture_multi", value))
	{
		capture.openCap(CAP_CAM);
		capture.cf.camPoints->clear();
		capture.cf.camMat->clear();
		for (int i = 0; i < atoi(value.c_str()); ++i)
		{
			capture.cf.camMat->push_back(capture.captureFrame(CAP_CAM));
		}
		capture.closeCap();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Frames Captured!");
	}
	if(CivetServer::getParam(conn, "capture_next", dummy))
	{
		capture.openCap(CAP_CAM);
		capture.cf.camMat->push_back(capture.captureFrame(CAP_CAM));
		capture.closeCap();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Next Frame Captured!");
	}
	if(CivetServer::getParam(conn, "detect", dummy))
	{
		std::vector<std::vector<cv::Point2f>> points;
		std::unique_lock<std::mutex> l(m);

		for (unsigned int i = 0; i < capture.cf.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.cf.camMat)[i];
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&mat, CAP_CAM);
			if (points.size() == 0)
			{
				/* remove frame with no detected faces */
				(*capture.cf.camMat).erase((*capture.cf.camMat).begin() + i);
			}
			else
			{
				/* add points of found faces */
				capture.cf.camPoints->push_back(points);
			}
		}
		l.unlock();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Face detection done!");
	}
	if(CivetServer::getParam(conn, "detect_file", dummy))
	{
		std::vector<std::vector<cv::Point2f>> points;
		std::unique_lock<std::mutex> l(m);

		capture.openCap(CAP_FILE);
		cv::Mat frame;
		for (;;)
		{
			frame = capture.captureFrame(CAP_FILE);
			if (frame.empty()) break;
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&frame, CAP_FILE);
			(*syslog) << "Found " << std::dec << points.size() << " faces in fileframe. " << endl;
			capture.filePoints->push_back(points);
		}
		capture.closeCap();

		/* remove frame with no detected faces */
		//(*capture.fileMat).erase((*capture.fileMat).begin() + i);
		l.unlock();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Face detection done!");
	}
	if(CivetServer::getParam(conn, "detect_file2", dummy))
	{
		std::vector<std::vector<cv::Point2f>> points;
		std::unique_lock<std::mutex> l(m);

		capture.openCap(CAP_FILE2);
		cv::Mat frame;
		for (;;)
		{
			frame = capture.captureFrame(CAP_FILE2);
			if (frame.empty()) break;
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&frame, CAP_FILE2);
			(*syslog) << "Found " << std::dec << points.size() << " faces in fileframe. " << endl;
			capture.file2Points->push_back(points);
		}
		capture.closeCap();

		/* remove frame with no detected faces */
		//(*capture.fileMat).erase((*capture.fileMat).begin() + i);
		l.unlock();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = _("Face detection done!");
	}

	std::stringstream ss;

	if(CivetServer::getParam(conn, "movie", dummy))
	{
	   DIR *dirp;
	   struct dirent *dp;
	   ss << "<form action=\"" << capture.getUrl() << "\" method=\"POST\">";
	   if ((dirp = opendir(MOVIES_DIR)) == NULL) {
	          (*syslog) << "couldn't open " << MOVIES_DIR << endl;
	   }
       do {
	      errno = 0;
	      if ((dp = readdir(dirp)) != NULL) {
	    	 /*
	    	  * ignore . and ..
	    	  */
	    	if (std::strcmp(dp->d_name, ".") == 0) continue;
	    	if (std::strcmp(dp->d_name, "..") == 0) continue;
	    	ss << "<button type=\"submit\" name=\"newmovie\" value=\"" << MOVIES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newmovie\">" << _("File") << "</button>&nbsp;";
	    	ss << "<button type=\"submit\" name=\"newmovie2\" value=\"" << MOVIES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newmovie2\">" << _("File") << " 2</button>&nbsp;";
	    	ss << "&nbsp;" << dp->d_name << "<br>";
	        }
	   } while (dp != NULL);
       ss << "<button type=\"submit\" name=\"annuleren\" value=\"annuleren\" id=\"annuleren\">" << _("Cancel") << "</button>&nbsp;";
       ss << "</form>";
       (void) closedir(dirp);
	}
	else
	/* initial page display */
	{
		//tohead << "<script type=\"text/javascript\">";
		//tohead << " $(document).ready(function(){";
		//tohead << "  setInterval(function(){";
		//tohead << "  $.get( \"" << capture.getUrl() << "?streaming=true\", function( data ) {";
		//tohead << "  $( \"#capture\" ).html( data );";
		//tohead << " });},1000)";
		//tohead << "});";
		//tohead << "</script>";
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << capture.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << capture.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#filename').on('change', function() {";
		tohead << " $.get( \"" << capture.getUrl() << "\", { filename: 'true', value: $('#filename').val() }, function( data ) {";
		tohead << "  $( \"#filename\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#filename2').on('change', function() {";
		tohead << " $.get( \"" << capture.getUrl() << "\", { filename2: 'true', value: $('#filename2').val() }, function( data ) {";
		tohead << "  $( \"#filename2\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#mix_file').on('change', function() {";
		tohead << " $.get( \"" << capture.getUrl() << "\", { mix_file: 'true', value: $('#mix_file').val() }, function( data ) {";
		tohead << "  $( \"#mix_file\" ).html( data );})";
	    tohead << "});";
	    tohead << " $('#mix_file2').on('change', function() {";
   		tohead << " $.get( \"" << capture.getUrl() << "\", { mix_file2: 'true', value: $('#mix_file2').val() }, function( data ) {";
   		tohead << "  $( \"#mix_file2\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#fileonly').on('change', function() {";
   		tohead << " $.get( \"" << capture.getUrl() << "\", { fileonly: 'true', value: $('#fileonly').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#fileonly\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#frames').on('change', function() {";
   		tohead << " $.get( \"" << capture.getUrl() << "\", { frames: $('#frames').val(), faceonframe: $('#faceonframe').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#photo\" ).attr( 'src' , data );})";
  	    tohead << "});";
  	    tohead << " $('#faceonframe').on('change', function() {";
   		tohead << " $.get( \"" << capture.getUrl() << "\", { frames: $('#frames').val(), faceonframe: $('#faceonframe').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#photo\" ).attr( 'src' , data );})";
   		tohead << "});";
		tohead << "});";
		tohead << "</script>";
		ss << "<form action=\"" << capture.getUrl() << "\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">" << _("Refresh") << "</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"capture\" value=\"capture\" id=\"capture_button\">" << _("Input Cam") << "</button>";
	    ss << "<button type=\"submit\" name=\"capture_next\" value=\"capture_next\" id=\"capture_button_next\">" << _("Next Frame") << "</button>";
	    ss << "<button type=\"submit\" name=\"detect\" value=\"detect\" id=\"detect_button\">" << _("Detect") << "</button>";
	    ss << "<button type=\"submit\" name=\"save_video\" value=\"save_video\" id=\"save_video\">" << _("Save as Video") << "</button></br>";
	    ss << "<button type=\"submit\" name=\"movie\" id=\"movie\">" << _("Input File") << "</button>";
	    ss << "<button type=\"submit\" name=\"detect_file\" value=\"detect_file\" id=\"detect__file_button\">" << _("Detect File") << "</button>";
	    ss << "<button type=\"submit\" name=\"detect_file2\" value=\"detect_file2\" id=\"detect__file2_button\">" << _("Detect File") << " 2</button><br>";
	    ss << "<button type=\"submit\" name=\"merge\" id=\"merge\">" << _("Merge") << "</button>";
	    ss << "<button type=\"submit\" name=\"on_screen\" id=\"on_screen\">" << _("On Screen") << "</button>";
	    ss << "</form>";
	    ss << "<h2>";
		if (!((*capture.cf.camPoints).size() == 0))
		for (unsigned int i = 0; i < (*capture.cf.camPoints).size(); i++)
		{
			ss << _("Number of faces in frame ") << i+1 << ":&nbsp;" << (*capture.cf.camPoints)[i].size() << "<br>";
		}
		else
			ss << _("No faces found.") << "<br>";
		ss << "</h2>";
		ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" value=\"" <<
					  capture.naam << "\" name=\"naam\"/>" << "<br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" value=\"" <<
					  capture.omschrijving << "\" name=\"omschrijving\"/>" << "<br>";
		ss << "<label for=\"filename\">" << _("Filename") << ":</label>"
					  "<input class=\"inside\" id=\"filename\" type=\"text\" size=\"50\" value=\"" <<
					  capture.filename << "\" name=\"filename\"/>" << "</br>";
		ss << "<label for=\"filename2\">" << _("Filename") << "2:</label>"
					  "<input class=\"inside\" id=\"filename2\" type=\"text\" size=\"50\" value=\"" <<
					  capture.filename2 << "\" name=\"filename2\"/>" << "</br>";
		if (capture.fileonly)
		{
			ss << "<label for=\"fileonly\">" << _("File Only") << ":</label>"
			   	   	  "<input id=\"fileonly\" type=\"checkbox\" name=\"fileonly\" value=\"ja\" checked/>" << "</br>";
		}
		else
		{
			ss << "<label for=\"fileonly\">" << _("File Only") << ":</label>"
			   	   	  "<input id=\"fileonly\" type=\"checkbox\" name=\"fileonly\" value=\"ja\"/>" << "</br>";
		}
	    ss << "<br>";
		ss << "<label for=\"mix_file\">" << _("Mix file") << ":</label>";
		ss << "<td><input class=\"inside\" id=\"mix_file\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			   capture.mix_file << "\"" << " name=\"mix_file\" /><br>";
		ss << "<label for=\"mix_to\">" << _("Mix file") << "2:</label>";
		ss << "<td><input class=\"inside\" id=\"mix_file2\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			  capture.mix_file2 << "\"" << " name=\"mix_file2\" /><br>";
		ss << "</div>";
		ss << "<br>";
	    ss << "</br>";
	    /*
	    ss << "<div id=\"capture\">";
	    ss << "<img src=\"" << capture.url << "/?streaming=true\">";
	    ss << "</div>"; */
	    ss << "<h2>" << _("Photo") << ":</h2>";
	    ss << "<img src=\"" << "tmp/" << capture.getUuid() << "_photo.jpg?t=" << std::time(0) << "\"></img><br>";
		if (!capture.filename.empty())
	    {
	    	//capture.openCap(CAP_FILE);
	    	//int number_of_frames = capture.cap->get(CV_CAP_PROP_FRAME_COUNT);
	    	//capture.closeCap();
	    	//l.unlock();
			int number_of_frames = capture.filePoints->size();
	    	if (number_of_frames)
	    	{
	    		ss << "<h2>" << _("File") << ":</h2>";
	    		ss << "<form oninput=\"result.value=parseInt(frames.value)\">";
	    		ss << "<label for=\"frames\">" << _("Frame") << ":</label>";
	    		ss << "<output name=\"result\">1</output><br>";
	    		ss << "<input class=\"inside\" id=\"frames\" type=\"range\" min=\"1\" max=\"" << number_of_frames << "\" step=\"1\" value=\"1\" name=\"frames\" />";
	    		ss << "</form>";
				ss << "<label for=\"faceonframe\">" << _("Render faces on frame") << ":</label>"
				   	   	  "<input id=\"faceonframe\" type=\"checkbox\" name=\"faceonframe\" value=\"ja\"/>" << "</br>";
	    		ss << "<img id=\"photo\" src=\"" << "tmp/" << capture.getUuid() << "_file.jpg?t=" << std::time(0) << "\"></img><br>";
	    		ss << "<br>";
	    	}
	    }
	    ss << "<h2>" << _("Video") << ":</h2>";
	    ss << "<video width=\"" << VIDEO_WIDTH << "\" height=\"" << VIDEO_HEIGHT << "\" controls>";
	    ss << " <source src=\"" << "tmp/" << capture.getUuid() << ".mp4?t=" << std::time(0) << "\" type=\"video/mp4\">";
	    ss << "Your browser does not support the video tag";
		ss << "</video>";
		ss << "<br>";
	}

	ss = getHtml(meta, message, "capture", ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn, ss.str().c_str(), "%s");
	return true;
}

