/*
* ######################## for Facial Recognition feature ########################
* Copyright (c) 2011. Philipp Wagner <bytefish[at]gmx[dot]de>.
* Released to public domain under terms of the BSD Simplified license.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of the organization nor the names of its contributors
*     may be used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*   See <http://www.opensource.org/licenses/bsd-license>
* ###############################################################################
*/

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// Include standard libraries
#include <cstdio>	// Used for "printf"
#include <string>	// Used for C++ strings
#include <stdlib.h>
#include <stdio.h>
#include <iostream>	// Used for C++ cout print statements
#include <fstream>
#include <sstream>

// User libraries included here.
#include "HSVColorWheel.h"

// Include OpenCV libraries
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/cvaux.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv2/core/core.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/nonfree/features2d.hpp>

// For curl library to read from http site for data from Pebble
#include <curl/curl.h>

// Namespaces
using namespace cv;
using namespace std;

/// Function Prototypes
// This function is automatically called whenever the user clicks the mouse in the window.
void mouseEvent(int ievent, int x, int y, int flags, void* param);
// Used for creating the red outlines.
void trackFilteredObject(Mat threshold, Mat &cameraFeed);
// Calculates image for COLOR_PICK
Mat calcColorPick(Mat imgOriginal);
// Calculates image for OUTLINE
Mat calcOutline(Mat imgOriginal);
// Used for facial rec data
static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';');
// Calculates facial rec frame
Mat calcFace(Mat imgOriginal, CascadeClassifier haar_cascade, int im_width, int im_height, Ptr<FaceRecognizer> model);
// Some curl function
size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream);
// Handling Pebble app string
int getMode(std::string buf);
// facial detect frame
Mat calcFaceDetect(Mat imgOriginal, CascadeClassifier haar_cascade, int im_width, int im_height, Ptr<FaceRecognizer> model);

/* ADDED FOR OBJECT DETECTION: read an image of object, detect presence of that object in live video feed.
// Calculates image for Object Detection by SURF
Mat calcObjectDetect(Mat imgOriginal);
// get min element of array of size 4
int minElement(int arr[]);
// get max element of array of size 4
int maxElement(int arr[]);
*/

// Globals

enum CONSTANTS{
	WIDTH = 361,        // Window Size
	HEIGHT = 306,       //     "
	HUE_RANGE = 180,    // OpenCV just uses Hues between 0 to 179.
	HUE_HEIGHT = 25,    // Thickness of Hue chart.
	WHEEL_TOP = 45,     // y position for top of color wheel (= Hue height + 20)
	WHEEL_BOTTOM = 300, // y position for bottom of color wheel (= WHEEL_TOP + 255)
	TILE_LEFT = 280,    // Position of small tile showing highlighted color
	TILE_TOP = 140,     //     "
	TILE_W = 60,        //     "
	TILE_H = 60,        //     "
};

enum MODES{
	MODE_ERROR = 0,
	ORIGINAL,
	OUTLINE,
	GRAY,
	BW,
	SEPIA,
	HUE,
	//More filters go here.
	COLOR_PICK,
	FACE,
	FACE_DETECT,
};

int last_mode = 1;

char *colorWheelTitle = "HSV Color Wheel";	// title of the window

int framewidth = 680;
int frameheight = 480;

int hue = 90;			// This variable is adjusted by the user's trackbar at runtime.
int saturation = 240;	//		"
int brightness = 200;	//		"

int mouseX = -1;	// Position in the window that a user clicked the mouse button.
int mouseY = -1;	//		"

// Matrices for calculations
Mat img_gray;
Mat imgHSV;
Mat imgThresholded;
Mat img_invertThreshold;
Mat img_grayRGB;
Mat img_obj;

Mat dst, detected_edges;
Mat kern = (cv::Mat_<float>(4, 4) << 0.272, 0.534, 0.131, 0,
	0.349, 0.686, 0.168, 0,
	0.393, 0.769, 0.189, 0,
	0, 0, 0, 1);

vector<Mat> hsv_planes;
int edgeThresh = 1;
int lowThreshold = 33;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
int hueUpdate = 10;
bool bounce = false;

// HTML String
std::string buffer;
String h;
String s;
String v;
char separator = ',';
char beginning = ':';
bool start = false;
bool nextNum = false;
bool next2Num = false;

int mode = 1;
int counter = 0;

int main(int argc, char** argv)
{
	// START TRAINING
	std::cout << "START TRAINING" << endl;
	// Get the path to your CascadeClassifier and CSV:
	string fn_haar = "C:/Users/Alvin/Desktop/opencv/sources/data/haarcascades/haarcascade_frontalface_default.xml";
	string fn_csv = "C:/Users/Alvin/Desktop/findAR/facescsv.txt"; // Change to work
	int deviceId = 0;
	// These vectors hold the images and corresponding labels:
	vector<Mat> images;
	vector<int> labels;
	// Read in the data (fails if no valid input filename is given, but you'll get an error message):
	try {
		read_csv(fn_csv, images, labels);
	}
	catch (cv::Exception& e) {
		cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
		// nothing more we can do
		exit(1);
	}
	// Get the height from the first image. We'll need this
	// later in code to reshape the images to their original
	// size AND we need to reshape incoming faces to this size:
	int im_width = images[0].cols;
	int im_height = images[0].rows;
	// The following lines create an LBPH model for
	// face recognition and train it with the images and
	// labels read from the given CSV file.
	//
	// The LBPHFaceRecognizer uses Extended Local Binary Patterns
	// (it's probably configurable with other operators at a later
	// point), and has the following default values
	//
	//      radius = 1
	//      neighbors = 8
	//      grid_x = 8
	//      grid_y = 8
	//
	// So if you want a LBPH FaceRecognizer using a radius of
	// 2 and 16 neighbors, call the factory method with:
	//
	//      cv::createLBPHFaceRecognizer(2, 16);
	//
	// And if you want a threshold (e.g. 123.0) call it with its default values:
	//
	//      cv::createLBPHFaceRecognizer(1,8,8,8,123.0)
	//
	Ptr<FaceRecognizer> model = createEigenFaceRecognizer();
	model->train(images, labels);
	// That's it for learning the Face Recognition model. You now
	// need to create the classifier for the task of Face Detection.
	// We are going to use the haar cascade you have specified in the
	// command line arguments:
	//
	CascadeClassifier haar_cascade;
	haar_cascade.load(fn_haar);
	std::cout << "END TRAINING" << endl;
	// END TRAINING

	// Create a GUI window
	cvNamedWindow(colorWheelTitle, 1);
	
	VideoCapture cap(0); //capture the video from webcam
    
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    
	frameheight = int(cap.get(3));
	framewidth = int(cap.get(4));
    
	if (!cap.isOpened())  // if not success, exit program
	{
		std::cout << "Cannot open the web cam" << endl;
		return -1;
	}
    
	Mat imgOriginal;
	Mat img_final;

	while (true)
	{
		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			std::cout << "Cannot read a frame from video stream" << endl;
			break;
		}
		counter++;
		if (counter == 100 || (mode == FACE && counter >= 10) || (mode == COLOR_PICK && counter >= 20)) //delay to reduce latency
		{
			//curl request from web server
			CURL *curl = curl_easy_init();
			curl_easy_setopt(curl, CURLOPT_URL, "http://dev.quasi.co/findar/");
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			counter = 0;
		}

		mode = getMode(buffer); //get mode from pebble

		if (!mode)
		{
			std::cout << "Cannot get mode from pebble" << endl;
			break;
		}

		switch (mode)
		{
		case ORIGINAL:
			img_final = imgOriginal;
			last_mode = ORIGINAL;
			break;
		case OUTLINE:
			img_final = calcOutline(imgOriginal);
			last_mode = OUTLINE;
			break;
		case GRAY:
			// Convert the image to grayscale
			cv::cvtColor(imgOriginal, img_gray, CV_BGR2GRAY);
			cv::cvtColor(img_gray, img_final, CV_GRAY2BGR);
			last_mode = GRAY;
			break;
		case BW:
			cv::cvtColor(imgOriginal, img_gray, CV_RGB2GRAY);
			img_final = img_gray > 128;
			last_mode = BW;
			break;
		case SEPIA:
			transform(imgOriginal, img_final, kern);
			last_mode = SEPIA;
			break;
		case HUE:
			cv::cvtColor(imgOriginal, imgHSV, CV_RGB2HSV);
			split(imgHSV, hsv_planes);
			hsv_planes[0] += hueUpdate; // H channel
			if (!bounce)
				hueUpdate += 10;
			else
				hueUpdate -= 10;
			if (hueUpdate == 180)
				bounce = true;
			if (hueUpdate == 0)
				bounce = false;
			merge(hsv_planes, imgOriginal);
			img_final = imgOriginal;
			last_mode = HUE;
			break;
		//More filters go here.
		case COLOR_PICK:
			img_final = calcColorPick(imgOriginal);
			last_mode = COLOR_PICK;
			break;
		case FACE:
			img_final = calcFace(imgOriginal, haar_cascade, im_width, im_height, model);
			last_mode = FACE;
			break;
		case FACE_DETECT:
			img_final = calcFaceDetect(imgOriginal, haar_cascade, im_width, im_height, model);
			break;
		case MODE_ERROR:
		default:
			cout << "default break ERROR" << endl;
			exit(1);
			break;
		}
		
		cv::imshow("Final", img_final); //show the chosen image
		buffer = ""; //reset buffer to get new input from Pebble

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			std::cout << "esc key is pressed by user" << endl;
			break;
		}
        
		// Allow the user to click on Hue chart to change the hue, or click on the color wheel to see a value.
		cvSetMouseCallback(colorWheelTitle, &mouseEvent, 0);
        
		displayColorWheelHSV(hue, saturation, brightness, colorWheelTitle);
	}
	return 0;
}

// Used for creating the red oulines.
void trackFilteredObject(Mat threshold, Mat &cameraFeed)
{
	Mat temp;
	threshold.copyTo(temp);
    
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
    
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	drawContours(cameraFeed, contours, -1, cv::Scalar(0, 0, 255), 3);
}

// Used to get the HSV values when the mouse is moved.
void mouseEvent(int ievent, int x, int y, int flags, void* param)
{
	// Check if they clicked or dragged a mouse button or not.
	if (flags & CV_EVENT_FLAG_LBUTTON) {
		mouseX = x;
		mouseY = y;
		//cout << mouseX << "," << mouseY << endl;
        
		// If they clicked on the Hue chart, select the new hue.
		if (mouseY < HUE_HEIGHT) {
			if (mouseX / 2 < HUE_RANGE) {	// Make sure its a valid Hue
				hue = mouseX / 2;
			}
		}
		// If they clicked on the Color wheel, select the new value.
		else if (mouseY >= WHEEL_TOP && mouseY <= WHEEL_BOTTOM) {
			if (mouseX < 256) {	// Make sure its a valid Saturation & Value
				saturation = mouseX;
				brightness = 255 - (mouseY - WHEEL_TOP);
			}
		}
	}
}

Mat calcColorPick(Mat imgOriginal)
{
	float hsv[3] = { hue / 179.0f, saturation / 255.0f, brightness / 255.0f };

	float hLow = hsv[0] - 0.10f;
	if (hLow < 0) {
		hLow = 0;
	}
	float hHigh = hsv[0] + 0.10f;
	if (hHigh > 1) {
		hHigh = 1;
	}

	float sLow = hsv[1] - 0.3f;
	if (sLow < 0) {
		sLow = 0;
	}
	float sHigh = hsv[1] + 0.3f;
	if (sHigh > 1) {
		sHigh = 1;
	}

	float vLow = hsv[2] - 0.48f;
	if (vLow < 0) {
		vLow = 0;
	}
	float vHigh = hsv[2] + 0.48f;
	if (vHigh > 1) {
		vHigh = 1;
	}
	float ranges[6] = { hLow, sLow, vLow, hHigh, sHigh, vHigh };

	// Create arbitrary ranges of HSV for detection
	int iLowH = int(ranges[0] * 179);
	int iHighH = int(ranges[3] * 179);
	int iLowS = int(ranges[1] * 255);
	int iHighS = int(ranges[4] * 255);
	int iLowV = int(ranges[2] * 255);
	int iHighV = int(ranges[5] * 255);

	//Create grayscale image
	cvtColor(imgOriginal, img_gray, CV_RGB2GRAY);

	//Convert the captured frame from BGR to HSV
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);

	//Threshold the image
	inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded);

	//morphological opening (removes small objects from the foreground)
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(10, 10)));
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(10, 10)));

	//morphological closing (removes small holes from the foreground)
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(10, 10)));
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(10, 10)));

	//Creating final filtered image
	bitwise_not(imgThresholded, img_invertThreshold);
	cvtColor(img_invertThreshold, img_invertThreshold, CV_GRAY2RGB);
	img_gray = img_gray - imgThresholded;
	cvtColor(img_gray, img_grayRGB, CV_GRAY2RGB);
	img_obj = imgOriginal - img_invertThreshold;
	Mat img_temp = img_grayRGB + img_obj;

	//Add indicator lines.
	trackFilteredObject(imgThresholded, img_temp);
	return img_temp;
}

Mat calcOutline(Mat imgOriginal)
{
	// Create a matrix of the same type and size as src (for dst)
	dst.create(imgOriginal.size(), imgOriginal.type());

	// Convert the image to grayscale
	cvtColor(imgOriginal, img_gray, CV_BGR2GRAY);

	// Reduce noise with a kernel 3x3
	blur(img_gray, detected_edges, Size(3, 3));

	// Canny detector
	Canny(detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);

	// Using Canny's output as a mask, we display our result
	dst = Scalar::all(0);

	imgOriginal.copyTo(dst, detected_edges);
	return dst;
}

static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator) {
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file) {
		string error_message = "No valid input file was given, please check the given filename.";
		CV_Error(CV_StsBadArg, error_message);
	}
	string line, path, classlabel;
	while (getline(file, line)) {
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty()) {
			images.push_back(imread(path, 0));
			labels.push_back(atoi(classlabel.c_str()));
		}
	}
}

Mat calcFace(Mat imgOriginal, CascadeClassifier haar_cascade, int im_width, int im_height, Ptr<FaceRecognizer> model)
{
	// Convert the current frame to grayscale:
	cvtColor(imgOriginal, img_gray, CV_BGR2GRAY);
	// Find the faces in the frame:
	vector< Rect_<int> > faces;
	haar_cascade.detectMultiScale(img_gray, faces);
	// At this point you have the position of the faces in
	// faces. Now we'll get the faces, make a prediction and
	// annotate it in the video. Cool or what?
	for (int i = 0; i < faces.size(); i++) {
		// Process face by face:
		Rect face_i = faces[i];
		// Crop the face from the image. So simple with OpenCV C++:
		Mat face = img_gray(face_i);
		// Resizing the face is necessary for Eigenfaces and Fisherfaces. You can easily
		// verify this, by reading through the face recognition tutorial coming with OpenCV.
		// Resizing IS NOT NEEDED for Local Binary Patterns Histograms, so preparing the
		// input data really depends on the algorithm used.
		//
		// I strongly encourage you to play around with the algorithms. See which work best
		// in your scenario, LBPH should always be a contender for robust face recognition.
		//
		// Since I am showing the Fisherfaces algorithm here, I also show how to resize the
		// face you have just found:
		Mat face_resized;
		cv::resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);
		// Now perform the prediction, see how easy that is:
		double predict_confidence = 0.0;
		int prediction = -1;
		model->predict(face_resized, prediction, predict_confidence);
		string name;
		string box_text;

		//cout << prediction << endl;
		//cout << predict_confidence << endl;

		// Calculate the position for annotated text (make sure we don't
		// put illegal values in there):
		int pos_x = face_i.tl().x - 10;
		int pos_y = face_i.tl().y - 10;

		// And finally write all we've found out to the original image!
		// First of all draw a green rectangle around the detected face:
		rectangle(imgOriginal, face_i, CV_RGB(0, 255, 0), 1);

		if (predict_confidence > 0) {
			if (prediction == 0) {
				name = "Yuki";
			}
			else if (prediction == 1) {
				name = "Alvin";
			}
			else if (prediction == 2) {
				name = "Ethan";
			}
			else if (prediction == 3) {
				name = "Mike";
			} else {
				name = "NOT RECOGNIZED";
			}
			// Create the text we will annotate the box with:
			box_text = "Prediction: " + name;
		}
		else {
			box_text = "???";
		}
		putText(imgOriginal, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0), 2.0);
	}
	// Show the result:
	return imgOriginal;
}

Mat calcFaceDetect(Mat imgOriginal, CascadeClassifier haar_cascade, int im_width, int im_height, Ptr<FaceRecognizer> model)
{
	// Convert the current frame to grayscale:
	cvtColor(imgOriginal, img_gray, CV_BGR2GRAY);
	// Find the faces in the frame:
	vector< Rect_<int> > faces;
	haar_cascade.detectMultiScale(img_gray, faces);
	// At this point you have the position of the faces in
	// faces. Now we'll get the faces, make a prediction and
	// annotate it in the video. Cool or what?
	for (int i = 0; i < faces.size(); i++) {
		// Process face by face:
		Rect face_i = faces[i];
		// And finally write all we've found out to the original image!
		// First of all draw a green rectangle around the detected face:
		rectangle(imgOriginal, face_i, CV_RGB(0, 255, 0), 1);
	}
	// Show the result:
	return imgOriginal;
}

/*  
 *  FOR OBJECT DETECTION
 *  Literally copy-pasted from main() of test .cpp file
 *  WILL NOT WORK AS-IS
 *  based on code from: https://github.com/doczhivago/rtObjectRecognition
 *
 */
/*
Mat calcObjectDetect(Mat imgOriginal)
{
    Mat object = imread( "example.jpg", CV_LOAD_IMAGE_GRAYSCALE );
    string objectName = "object title";
    
    if(!object.data) {
        cout<< "Base image cannot be read." << endl;
        return -1;
    }
    
    //Detect the keypoints using SURF Detector
    int minHessian = 500;
    
    SurfFeatureDetector detector(minHessian);
    vector<KeyPoint> kp_object;
    
    detector.detect( object, kp_object );
    
    //Calculate descriptors (feature vectors)
    SurfDescriptorExtractor extractor;
    Mat des_object;
    
    extractor.compute(object, kp_object, des_object);
    
    FlannBasedMatcher matcher;

    // REPLACE WITH imgOriginal
    // VideoCapture cap(0);
    // cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	// cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    
    std::vector<Point2f> obj_corners(4);
    
    //Get the corners from the object
    obj_corners[0] = (cvPoint(0, 0));
    obj_corners[1] = (cvPoint(object.cols, 0));
    obj_corners[2] = (cvPoint(object.cols, object.rows));
    obj_corners[3] = (cvPoint(0, object.rows));
    
    char key = 'a';
    int framecount = 0;
    while (key != 27)
    {
        Mat frame;
        cap >> frame;
        
        if (framecount < 5) {
            framecount++;
            continue;
        }
        
        Mat des_image, img_matches;
        std::vector<KeyPoint> kp_image;
        std::vector<vector<DMatch > > matches;
        std::vector<DMatch > good_matches;
        std::vector<Point2f> obj;
        std::vector<Point2f> scene;
        std::vector<Point2f> scene_corners(4);
        Mat H;
        Mat image;
        
        cvtColor(frame, image, CV_RGB2GRAY);
        
        detector.detect(image, kp_image);
        extractor.compute(image, kp_image, des_image);
        
        matcher.knnMatch(des_object, des_image, matches, 2);
        
        //THIS LOOP IS SENSITIVE TO SEGFAULTS
        for(int i = 0; i < min(des_image.rows-1,(int) matches.size()); i++) {
            if((matches[i][0].distance < 0.6*(matches[i][1].distance)) && ((int) matches[i].size()<=2 && (int) matches[i].size()>0)) {
                good_matches.push_back(matches[i][0]);
            }
        }
        
        if (good_matches.size() >= 4) {
            for ( int i = 0; i < good_matches.size(); i++ ) {
                //Get the keypoints from the good matches
                obj.push_back(kp_object[ good_matches[i].queryIdx ].pt);
                scene.push_back(kp_image[ good_matches[i].trainIdx ].pt);
            }
            
            H = findHomography(obj, scene, CV_RANSAC);
            
            perspectiveTransform(obj_corners, scene_corners, H);
            
            int xValues[] = {scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x};
            int yValues[] = {scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y};
            
            // finding top-left corner coordinates
            int leftCornerX = minElement(xValues);
            int leftCornerY = minElement(yValues);
            
            // finding bottom-right corner coordinates
            int rightCornerX = maxElement(xValues);
            int rightCornerY = maxElement(yValues);
            
            int width = abs (rightCornerX - leftCornerX);
            int height = abs (rightCornerY - leftCornerY);
            
            // finding center between corners
            int pos_x = (width / 2) + leftCornerX;
            int pos_y = (height / 2) + leftCornerY;
            
            int rad = width/2;
            
            // drawing bounding circle and object label
            circle(frame, Point(pos_x, pos_y), rad, Scalar(255, 0, 0), 4, 8, 0);
            putText(frame, objectName, Point(pos_x, pos_y - rad - 15), FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 0, 0), 2.0);
        }
        
        //Show detected matches
        imshow( "Object Recognition", frame );
        
        key = waitKey(1);
    }
    return 0;
}

// finds min element in array of size 4
int minElement(int arr[]) {
    int curr = arr[0];
    for (int i = 1; i < 4; i++) {
        if (arr[i] < curr) {
            curr = arr[i];
        }
    }
    return curr;
}

// finds max element in array of size 4
int maxElement(int arr[]) {
    int curr = arr[0];
    for (int i = 1; i < 4; i++) {
        if (arr[i] > curr) {
            curr = arr[i];
        }
    }
    return curr;
}
*/

// This is the callback function that is called by curl_easy_perform(curl) 
size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
	buffer.append((char*)ptr, size*nmemb);
	return size*nmemb;
}

int getMode(std::string buf)
{
	std::cout << buf << endl;
	if (!buf.compare("null") || buf == "")
		return mode;
	if (!buf.compare("original"))
		mode = ORIGINAL;
	else if (!buf.compare("outline"))
		mode = OUTLINE;
	else if (!buf.compare("grayscale"))
		mode = GRAY;
	else if (buf == "b/w")
		mode = BW;
	else if (buf == "sepia")
		mode = SEPIA;
	else if (buf == "hue scan")
		mode = HUE;
	else if (buf == "face scan")
		mode = FACE;
	else if (buf == "face detect")
		mode = FACE_DETECT;
	else if (buf.size() > 0)
	{
		mode = COLOR_PICK;
		char first = buf[0];
		if (first == '+' || first == '-')
		{
			if (buf == "+ hue")
			{
				hue += 12;
				if (hue > 179)
					hue = 179;
			}
			else if (buf == "+ saturation")
			{
				saturation += 16;
				if (saturation > 255)
					saturation = 255;
			}
			else if (buf == "+ lightness")
			{
				brightness += 16;
				if (brightness > 255)
					brightness = 255;
			}
			else if (buf == "- hue")
			{
				hue -= 12;
				if (hue < 0)
					hue = 0;
			}
			else if (buf == "- saturation")
			{
				saturation -= 16;
				if (saturation < 0)
					saturation = 0;
			}
			else if (buf == "- lightness")
			{
				brightness -= 16;
				if (brightness < 0)
					brightness = 0;
			}
		}
		else
		{
			for (int i = 0; i < buf.size(); i++)
			{
				if (buf[i] == beginning)
					start = true;
				else if (start && !nextNum && !next2Num)
				{
					if (buf[i] == separator)
						nextNum = true;
					else
						h += buf[i];
				}
				else if (nextNum && !next2Num)
				{
					if (buf[i] == separator)
						next2Num = true;
					else
						s += buf[i];
				}
				else if (next2Num)
				{
					v += buf[i];
				}
			}
			hue = (((double)(atoi(h.c_str())+1.0)/360.0)*180.0);
			saturation = ((double)(atoi(s.c_str())/100.0)*255.0);
			brightness = ((double)(atoi(v.c_str())/100.0)*255.0);
			cout << "h: " << h << " s: " << s << " v: " << v;
			cout << "hue: " << hue << " sat: " << saturation << " val: " << brightness;
			h = "";
			s = "";
			v = "";
			start = false;
			nextNum = false;
			next2Num = false;
		}
	}
	else
		mode = last_mode;
	std::cout << mode;
	return mode;
}