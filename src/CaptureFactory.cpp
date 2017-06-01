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

cv::Size feather_amount;
uint8_t LUTJE[3][256];
int source_hist_int[3][256];
int target_hist_int[3][256];
float source_histogram[3][256];
float target_histogram[3][256];

std::mutex m;
std::mutex m_merging;
std::mutex m_pose;

class mythread : public std::thread
{
  public:
    mythread() {}
    static void setScheduling(std::thread &th, int policy, int priority) {
        sched_param sch_params;
        sch_params.sched_priority = priority;
        if(pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
            std::cerr << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
        }
    }
  private:
    sched_param sch_params;
};

/*
static cv::Rect dlibRectangleToOpenCV(dlib::rectangle r)
{
  return cv::Rect(cv::Point2i(r.left(), r.top()), cv::Point2i(r.right() + 1, r.bottom() + 1));
}
*/
static dlib::rectangle openCVRectToDlib(cv::Rect r)
{
  return dlib::rectangle((long)r.tl().x, (long)r.tl().y, (long)r.br().x - 1, (long)r.br().y - 1);
}

/*
static std::stringstream matToBase64PNG(cv::Mat* input)
{
    std::vector<uchar> buf;
    cv::imencode(".png", *input, buf, std::vector<int>() );

    // Base64 encode the stringstream
    base64::encoder E;
    stringstream encoded;
    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    E.encode(incoming, encoded);

    std::stringstream png;
    png << encoded.str();
    return png;
}
*/
static std::stringstream matToJPG(cv::Mat* input)
{
    std::vector<uchar> buf;
    cv::Mat resized;
    try
    {
        cv::resize((*input), resized, cv::Size(320,240), 0, 0);
        int params[3] = {0};
        params[0] = CV_IMWRITE_JPEG_QUALITY;
        params[1] = 60;
    	cv::imencode(".jpg", resized, buf, std::vector<int>(params, params+2) );
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        std::cout << "exception caught: " << err_msg << std::endl;
    }

    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    std::stringstream jpg;
    jpg << incoming.str();
    return jpg;
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
    points.push_back(cv::Point2f(points[17].x+(points[17].x-points[2].x),points[17].y - (points[2].y-points[17].y)));

    //calculate forehead middle (point index 69)
    points.push_back(cv::Point2f(points[27].x+(points[29].x-points[8].x), points[29].y - (points[8].y - points[29].y)));

    //calculate forehead right ((point index 70)
    points.push_back(cv::Point2f(points[26].x+(points[26].x-points[14].x),points[26].y - (points[14].y-points[26].y)));

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

static std::stringstream drawToJPG(cv::Mat* input, std::vector<std::vector<cv::Point2f>>* points)
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
        draw_polyline(output, (*points)[i], 68, 70);          // Forehead

        cv::circle(output, (*points)[i][8], 3, cv::Scalar(0,255,255), -1, 16);
        cv::circle(output, (*points)[i][29], 3, cv::Scalar(0,255,255), -1, 16);
        cv::circle(output, (*points)[i][69], 3, cv::Scalar(0,255,255), -1, 16);

    }

    return matToJPG(&output);
}

/*
 * CaptureFactory Constructor en Destructor
 */
CaptureFactory::CaptureFactory(){
    mfh = new CaptureFactory::CaptureFactoryHandler(*this);
    pose_model = new dlib::shape_predictor();
	cout << "Reading in shape predictor..." << endl;
    deserialize("shape_predictor_68_face_landmarks.dat") >> *pose_model;
    cout << "Done reading in shape predictor ..." << endl;
	server->addHandler("/capturefactory", mfh);
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
	    window->setFramerateLimit(10);
	    //window->setActive(false);
		renderingThread(window); } );
	//mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();
}

CaptureFactory::~CaptureFactory(){
	delete mfh;
	delete pose_model;
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
	this->off_screen = new std::vector<std::stringstream>();

	camMat = new std::vector<cv::Mat>();
	camPoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();
	filePoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();

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
		                         std::string omschrijving, std::string filmpje,
								 std::vector<std::vector<std::vector<cv::Point2f>>>* filepoints,
								 unsigned int mix_from,
								 unsigned int mix_to):cf(cf){
	mh = new CaptureFactory::Capture::CaptureHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->cap = new cv::VideoCapture();
	this->off_screen = new std::vector<std::stringstream>();
	this->filmpje = filmpje;
	this->filePoints = filepoints;
	this->mix_from = mix_from;
	this->mix_to = mix_to;

	camMat = new std::vector<cv::Mat>();
	camPoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();

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
	delete camMat;
	delete camPoints;
	delete filePoints;
	//delete detector;
	delete off_screen;
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
    				cout << "Videofile kon niet geopend worden!" << endl;
    			}
    		}
    		catch( cv::Exception& e )
    		{
    		  	cout << "Error loading merged mp4: " << e.msg << endl;
    		}

    	}
		window->clear(sf::Color::Black);
    	if (loaded)
    	{
        	if (movie.getStatus() == sfe::Status::Stopped)
        		movie.play();
			movie.update();
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
                    std::cout << "the escape key was pressed" << std::endl;
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
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		std::string filmpje = node[i]["filmpje"].as<std::string>();
		unsigned int mix_from = (unsigned int) node[i]["mix_from"].as<int>();
		unsigned int mix_to = (unsigned int) node[i]["mix_to"].as<int>();
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
		CaptureFactory::Capture * capture = new CaptureFactory::Capture(*this, uuidstr, naam, omschrijving, filmpje, filepoints, mix_from, mix_to);
		std::string uuid_str = capture->getUuid();
		capturemap.insert(std::make_pair(uuid_str,capture));
	}
}

void CaptureFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_CAPTURE);
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, CaptureFactory::Capture*> element  : capturemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "filmpje";
		emitter << YAML::Value << element.second->filmpje;
		emitter << YAML::Key << "mix_from";
    	emitter << YAML::Value << (int) element.second->mix_from;
    	emitter << YAML::Key << "mix_to";
		emitter << YAML::Value << (int) element.second->mix_to;
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
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void CaptureFactory::Capture::openCap(captureType type)
{
	if (type == CAP_CAM)
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(0);
			cap->set(CAP_PROP_FOURCC ,CV_FOURCC('M', 'J', 'P', 'G') );
			cap->set(CAP_PROP_FRAME_WIDTH,1920);   // width pixels
			cap->set(CAP_PROP_FRAME_HEIGHT,1080);   // height pixels
		}
	}
	else
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(filmpje);
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
	if( !(*face_cascade).load("haarcascade_frontalface_default.xml") ){ printf("--(!)Error loading face cascade\n"); return; };
    model_loaded = true;
}

void CaptureFactory::Capture::captureDetectAndMerge()
{
	std::thread t1( [this] {
	std::unique_lock<std::mutex> l2(m_merging);
	openCap(CAP_CAM);
	cv::Mat captured;
	std::unique_lock<std::mutex> l(m);
	camPoints->clear();
	camMat->clear();
	for (int i = 0; i < 6; i++)
	{
		captured = captureFrame();

		if (captured.empty()) continue;

		std::vector<std::vector<cv::Point2f>> points = detectFrame(&captured);
		if (points.size() == 0)
			continue;
		else
		{
			/* add points of found faces */
			camMat->push_back(captured);
			camPoints->push_back(points);
		}
	}

	closeCap();
	if (camPoints->empty())
	{
		delete off_screen;
		off_screen = new std::vector<std::stringstream>(loadFilmpje());
		l.unlock();
		l2.unlock();
		return;
	}

	delete off_screen;
	off_screen = new std::vector<std::stringstream>(mergeFrames());

	l.unlock();
	l2.unlock();
	return;
	});
	t1.detach();
}

std::vector<std::stringstream> CaptureFactory::Capture::loadFilmpje()
{
    openCap(CAP_FILE);
	cv::Mat frame;
	std::vector<std::stringstream> jpg;
	for (;;)
	{
	   frame = captureFrame();
	   if (frame.empty()) break;
	    jpg.push_back(matToJPG(&frame));
	}
	closeCap();
	return jpg;
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
	 cf.on_screen = "tmp/" + this->getUuid() + ".mp4";
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

cv::Mat CaptureFactory::Capture::captureFrame(){
    // Serialize the input image to a stringstream
	cout << "Grabbing a frame..." << endl;
    // Grab a frame
	cv::Mat input;
	try
	{
		*cap >> input;
		if (!input.empty())
			cv::resize(input,input,cv::Size(1024,768));
	}
	catch( cv::Exception& e )
	{
	    const char* err_msg = e.what();
	    std::cout << "exception caught: " << err_msg << std::endl;
	}
	return input;
    /*
     * <img alt="Embedded Image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA..." />
     */
}

std::vector<std::vector<cv::Point2f>> CaptureFactory::Capture::detectFrame(cv::Mat* input)
{
       std::vector<std::vector<cv::Point2f>> points;
       cout << "Image size: " << (*input).cols << " x " << (*input).rows << std::endl;
       cv::Mat im_small;
       // Resize image for face detection
       try
       {
        cv::resize(*input, im_small, cv::Size(), 1.0/FACE_DOWNSAMPLE_RATIO, 1.0/FACE_DOWNSAMPLE_RATIO);
       }
       catch( cv::Exception& e )
       {
  	    const char* err_msg = e.what();
        std::cout << "exception caught in resize: " << err_msg << std::endl;
        return points;
       }

       //std::vector<dlib::rectangle> faces;
       std::vector<cv::Rect> faces;
       cout << "Detecting faces..." << endl;
       // Detect faces
       cv_image<bgr_pixel> img(*input);
       cv_image<bgr_pixel> cimg(im_small);
       //faces = (*detector)(cimg);
       cv::UMat reeds;
       cvtColor( *input, reeds, CV_BGR2GRAY);
       equalizeHist( reeds, reeds );
       (*face_cascade).detectMultiScale( reeds, faces, 1.3, 6, 0|CV_HAAR_SCALE_IMAGE, Size(60, 60) );
       if (faces.size() == 0)
       {
    	 cout << "No faces detected." << endl;
         return points;
       }
       // Find the pose of each face.
       std::vector<full_object_detection> shapes;
       full_object_detection shape;
       cout << "There were " << faces.size() << " faces detected." << endl;
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

       return points;
}


std::vector<std::stringstream> CaptureFactory::Capture::mergeFrames()
{
	std::vector<std::stringstream> totaal;
	std::string recordname = "tmp/" + this->getUuid() + ".mp4";
	VideoWriter record(recordname, CV_FOURCC('H','2','6','4'),
	    10, cv::Size(1024,768), true);

	openCap(CAP_FILE);

	for (unsigned int frame = 0; frame < (*filePoints).size(); ++frame)
	{
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		//delete img_file;
		cv::Mat img_file = captureFrame();
		cv::Mat img_orig = img_file.clone();
		if (img_file.empty()) break;
		//cv::resize(img_file,img_file,cv::Size(1024,768));
		//remove frame to keep memory available
		//(*fileMat).erase((*fileMat).begin() + frame);
		cv::Mat img_cam = (*camMat)[frame % ((*camMat).size())].clone();
		cv::Mat resultaat = img_file.clone();
		for (unsigned int gezicht = 0; gezicht < (*filePoints)[frame].size(); ++gezicht)
		{
			// if we have less faces in the capture than in the img_file for this frame we do nothing
			if (gezicht >= (*camPoints)[frame % ((*camPoints).size())].size()) continue;
	        //convert Mat to float data type
	        img_cam.convertTo(img_cam, CV_32F);
	        resultaat.convertTo(resultaat, CV_32F);

	        // Find convex hull
	        std::vector<Point2f> hull1;
	        std::vector<Point2f> hull2;
	        std::vector<int> hullIndex;

	        try
	        {
	         convexHull((*filePoints)[frame][gezicht], hullIndex, false, false);
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	         std::cout << "exception caught in convexHull: " << err_msg << std::endl;
    	     return totaal;
	      	}

	        for(int i = 0; i < (int)hullIndex.size(); i++)
	        {
	            hull1.push_back((*camPoints)[frame % ((*camPoints).size())][gezicht % ((*camPoints)[frame % ((*camPoints).size())].size())][hullIndex[i]]);
	            hull2.push_back((*filePoints)[frame][gezicht][hullIndex[i]]);
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
	         std::cout << "exception caught in calculateDelauneyTriangles: " << err_msg << std::endl;
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
		        std::cout << "exception caught in warpTriangle: " << err_msg << std::endl;
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
	         std::cout << "exception caught in fillConvexPoly: " << err_msg << std::endl;
          	 return totaal;
	      	}

            feather_amount.width = feather_amount.height = (int)cv::norm((*filePoints)[frame][gezicht][0] - (*filePoints)[frame][gezicht][16]) / 8;
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
	        }
            catch( cv::Exception& e )
	      	{
             std::cout << "exception!" << endl;
	   	     const char* err_msg = e.what();
	         std::cout << "exception caught in seamlessClone: " << err_msg << std::endl;
	     	 return totaal;
	      	}
		}

	  float per_frame = ((float) mix_to - (float) mix_from) / (float)(*filePoints).size();
	  float mix_frame = ((float) mix_from + (per_frame * (frame + 1))) / 100;
	  addWeighted(resultaat, mix_frame, img_orig, 1 - mix_frame, 0.0, resultaat);
      //we should fill out at least one 10 fps period
      if ((*filePoints).size() == 1)
      for (int i = 0; i < 10; i++)
      {
      	 record.write(resultaat);
      }
      else
        record.write(resultaat);
	  totaal.push_back(matToJPG(&resultaat));
	  high_resolution_clock::time_point t2 = high_resolution_clock::now();
	  auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
      cout << "merging frame took: " << int_ms.count() << " milliseconds" << endl;
	}
	closeCap();
	return totaal;
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

	if(CivetServer::getParam(conn, "delete", value))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
	   message = "Verwijderd!";
	   this->capturefactory.deleteCapture(value);
	}
	else
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
	   message = "Opgeslagen!";
       this->capturefactory.save();
	}
	else
	if(CivetServer::getParam(conn, "load", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
	   message = "Geladen!";
	   this->capturefactory.load();
	}
	else
	/* if parameter clear is present the clear screen button was pushed */
	if(CivetServer::getParam(conn, "clear", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" />";
       message = "Scherm leeg gemaakt!";
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
	  message = "Aangemaakt!";
	}

	std::stringstream ss;

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/capturefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";

	}
	/* initial page display */
	else
	{
		std::map<std::string, CaptureFactory::Capture*>::iterator it = capturefactory.capturemap.begin();
		for (std::pair<std::string, CaptureFactory::Capture*> element : capturefactory.capturemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << "Naam:&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"clear\" id=\"clear\">Clear Screen</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "capture",  ss.str().c_str());
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

	if(CivetServer::getParam(conn, "streaming", dummy))
	{
		mg_printf(conn,
			          "HTTP/1.1 200 OK\r\nContent-Type: "
			          "multipart/x-mixed-replace; boundary=frame\r\n\r\n");
		//std::unique_lock<std::mutex> l(m);
		//ss << capture.manipulated.str();
		//l.unlock();
		//ss.seekp(0, ios::end);
		//stringstream::pos_type offset = ss.tellp();
		//for(;;)
		for (unsigned int i = 0; i < capture.off_screen->size(); ++i)
		{
			std::stringstream ss;
			ss << "Content-Type: image/jpeg\r\n\r\n" << (*capture.off_screen)[i].str() << "\r\n--frame\r\n";
			ss.seekp(0, ios::end);
			stringstream::pos_type offset = ss.tellp();
			ss.seekp(0, ios::beg);
			mg_write(conn, ss.str().c_str(), offset);
			delay(100);
		}
		return true;
	}

	/* if parameter submit is present the submit button was pushed */
	if(CivetServer::getParam(conn, "submit", dummy))
	{
	   CivetServer::getParam(conn,"naam", s[0]);
	   capture.naam = s[0].c_str();
	   CivetServer::getParam(conn,"omschrijving", s[1]);
	   capture.omschrijving = s[1].c_str();
       CivetServer::getParam(conn,"mix_from", s[2]);
	   capture.mix_from = atoi(s[2].c_str());
       CivetServer::getParam(conn,"mix_to", s[3]);
	   capture.mix_to = atoi(s[3].c_str());

	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
	   message = "Opgeslagen!";
	}
	else if(CivetServer::getParam(conn, "on_screen", dummy))
	{
		capture.onScreen();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Op het scherm!";
	}
	else if(CivetServer::getParam(conn, "merge", dummy))
	{
		std::unique_lock<std::mutex> l(m);
		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>(capture.mergeFrames());

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Samengevoegd!";
	}
	else if(CivetServer::getParam(conn, "newmovie", value))
	{
		//capture.fileMat->clear();
		//capture.filePoints->clear();
		std::unique_lock<std::mutex> l(m);
		capture.filmpje = value;
		std::stringstream ss;
		capture.openCap(CAP_FILE);
		cv::Mat frame;
		std::stringstream jpg;

		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>();

		for (;;)
		{
			frame = capture.captureFrame();
			if (frame.empty()) break;
			capture.off_screen->push_back(matToJPG(&frame));
		}
		capture.closeCap();

		l.unlock();

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + capture.getUrl() + "\"/>";
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "save_video", dummy))
	{
		Size S = Size((*capture.camMat)[0].cols,(*capture.camMat)[0].rows);
		int codec = CV_FOURCC('H', '2', '6', '4');
		VideoWriter outputVideo("resources/capture.mp4", codec, 10.0, S, true);

		if (outputVideo.open("resources/capture.mp4", codec, 10.0, S, true))
			cout << "Video open success!" << endl;
		else
			cout << "Video open failure!" << endl;
		for (unsigned int i = 0; i < (*capture.camMat).size(); ++i)
		{
			outputVideo << (*capture.camMat)[i];
		}
		outputVideo.release();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Video opgeslagen!";
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "capture", dummy))
	{
		capture.openCap(CAP_CAM);
		capture.camPoints->clear();
		capture.camMat->clear();
		capture.camMat->push_back(capture.captureFrame());
		capture.closeCap();
		std::unique_lock<std::mutex> l(m);

		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>();

		for (unsigned int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			capture.off_screen->push_back(matToJPG(&mat));
		}

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Frame gefotografeerd!";
	}
	else if(CivetServer::getParam(conn, "capture_multi", value))
	{
		capture.openCap(CAP_CAM);
		capture.camPoints->clear();
		capture.camMat->clear();
		for (int i = 0; i < atoi(value.c_str()); ++i)
		{
			capture.camMat->push_back(capture.captureFrame());
		}
		capture.closeCap();
		std::unique_lock<std::mutex> l(m);

		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>();

		for (unsigned int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			/* add points of found faces */
			cout << "adding frame " << i << endl;
			capture.off_screen->push_back(matToJPG(&mat));
		}

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Nieuwe frames gefotografeerd!";
	}
	else if(CivetServer::getParam(conn, "capture_next", dummy))
	{
		capture.openCap(CAP_CAM);
		capture.camMat->push_back(capture.captureFrame());
		capture.closeCap();
		std::unique_lock<std::mutex> l(m);

		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>();

		for (unsigned int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			/* add points of found faces */
			capture.off_screen->push_back(matToJPG(&mat));
		}

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Volgend frame gefotografeerd!";
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "detect", dummy))
	{
		std::thread t1( [this] {

		std::vector<std::vector<cv::Point2f>> points;
		std::unique_lock<std::mutex> l(m);

		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>();

		for (unsigned int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&mat);
			if (points.size() == 0)
			{
				/* remove frame with no detected faces */
				(*capture.camMat).erase((*capture.camMat).begin() + i);
			}
			else
			{
				/* add points of found faces */
				capture.camPoints->push_back(points);
				capture.off_screen->push_back(drawToJPG(&mat, &points));
			}
		}
		l.unlock();
		} );

		mythread::setScheduling(t1, SCHED_IDLE, 0);

		t1.detach();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Gezichtsdetectie wordt op de achtergrond uitgevoerd!";
	}
	else if(CivetServer::getParam(conn, "detect_file", dummy))
	{
		std::thread t1( [this] {

		std::vector<std::vector<cv::Point2f>> points;
		std::unique_lock<std::mutex> l(m);

		delete capture.off_screen;
		capture.off_screen = new std::vector<std::stringstream>();

		capture.openCap(CAP_FILE);
		cv::Mat frame;
		for (;;)
		{
			frame = capture.captureFrame();
			if (frame.empty()) break;
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&frame);
			capture.off_screen->push_back(drawToJPG(&frame, &points));
			capture.filePoints->push_back(points);
		}
		capture.closeCap();

		/* remove frame with no detected faces */
		//(*capture.fileMat).erase((*capture.fileMat).begin() + i);
		l.unlock();
		} );

		mythread::setScheduling(t1, SCHED_IDLE, 0);

		t1.detach();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + capture.getUrl() + "\"/>";
		message = "Gezichtsdetectie wordt op de achtergrond uitgevoerd!";
	}

	std::stringstream ss;
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "movie", dummy))
	{
	   DIR *dirp;
	   struct dirent *dp;
	   ss << "<form action=\"" << capture.getUrl() << "\" method=\"POST\">";
	   if ((dirp = opendir(RESOURCES_DIR)) == NULL) {
	          fprintf(stderr,"couldn't open %s.\n",RESOURCES_DIR);
	   }
       do {
	      errno = 0;
	      if ((dp = readdir(dirp)) != NULL) {
	    	 /*
	    	  * ignore . and ..
	    	  */
	    	if (std::strcmp(dp->d_name, ".") == 0) continue;
	    	if (std::strcmp(dp->d_name, "..") == 0) continue;
	    	ss << "<button type=\"submit\" name=\"newmovie\" value=\"" << RESOURCES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newmovie\">Selecteren</button>&nbsp;";
	    	ss << "&nbsp;" << dp->d_name << "<br>";
	        }
	   } while (dp != NULL);
       ss << "<button type=\"submit\" name=\"annuleren\" value=\"annuleren\" id=\"annuleren\">Annuleren</button>&nbsp;";
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
		ss << "<form action=\"" << capture.getUrl() << "\" method=\"POST\">";
		ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  capture.naam << "\" name=\"naam\"/>" << "<br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  capture.omschrijving << "\" name=\"omschrijving\"/>" << "<br>";
		ss << "<label for=\"mix_from\">Mix van:</label>";
		ss << "<td><input class=\"inside\" id=\"mix_from\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			   capture.mix_from << "\"" << " name=\"mix_from\" /><br>";
		ss << "<label for=\"mix_to\">Mix tot:</label>";
		ss << "<td><input class=\"inside\" id=\"mix_to\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			  capture.mix_to << "\"" << " name=\"mix_to\" /><br>";
		ss << "</div>";
		ss << "<br>";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">Refresh</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"capture\" value=\"capture\" id=\"capture_button\">Capture</button>";
	    ss << "<button type=\"submit\" name=\"capture_next\" value=\"capture_next\" id=\"capture_button_next\">Volgende Frame</button><br>";
	    ss << "<button type=\"submit\" name=\"capture_multi\" value=\"30\" id=\"capture_button_30\">Capture 30</button>";
	    ss << "<button type=\"submit\" name=\"capture_multi\" value=\"60\" id=\"capture_button_60\">Capture 60</button>";
	    ss << "<button type=\"submit\" name=\"capture_multi\" value=\"120\" id=\"capture_button_120\">Capture 120</button></br>";
	    ss << "<button type=\"submit\" name=\"save_video\" value=\"save_video\" id=\"save_video\">Save as Video</button></br>";
	    ss << "<button type=\"submit\" name=\"detect\" value=\"detect\" id=\"detect_button\">Detect</button>";
	    ss << "<button type=\"submit\" name=\"detect_file\" value=\"detect_file\" id=\"detect__file_button\">Detect File</button>";
	    ss << "<button type=\"submit\" name=\"movie\" id=\"movie\">Filmpje</button>";
	    ss << "<button type=\"submit\" name=\"merge\" id=\"merge\">Merge</button>";
	    ss << "<button type=\"submit\" name=\"on_screen\" id=\"on_screen\">On Screen</button>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<div id=\"capture\">";
	    ss << "<img src=\"" << capture.url << "/?streaming=true\">";
	    ss << "</div>";
	    ss << "<video width\"1024\" height=\"768\" controls>";
	    ss << " <source src=\"" << "tmp/" << capture.getUuid() << ".mp4" << "\" type=\"video/mp4\">";
	    ss << "Your browser does not support the video tag";
		ss << "</video>";
	}

	ss = getHtml(meta, message, "capture", ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn, ss.str().c_str(), "%s");
	return true;
}

