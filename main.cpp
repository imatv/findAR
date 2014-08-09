#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#include <cstdio>	// Used for "printf"
#include <string>	// Used for C++ strings
#include <iostream>	// Used for C++ cout print statements

// User libraries included here.
#include "HSVColorWheel.h"

// Include OpenCV libraries
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>

using namespace cv;
using namespace std;

/// Function Prototypes
// This function is automatically called whenever the user clicks the mouse in the window.
void mouseEvent(int ievent, int x, int y, int flags, void* param);
// Used for creating the red outlines.
void trackFilteredObject(Mat threshold, Mat &cameraFeed);

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

char *colorWheelTitle = "HSV Color Wheel";	// title of the window

int framewidth = 680;
int frameheight = 480;

int hue = 90;			// This variable is adjusted by the user's trackbar at runtime.
int saturation = 240;	//		"
int brightness = 200;	//		"

int mouseX = -1;	// Position in the window that a user clicked the mouse button.
int mouseY = -1;	//		"


int main(int argc, char** argv)
{
	// Create a GUI window
	cvNamedWindow(colorWheelTitle, 1);
	
	VideoCapture cap(0); //capture the video from webcam
    
	//cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	//cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    
	frameheight = int(cap.get(3));
	framewidth = int(cap.get(4));
    
	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the web cam" << endl;
		return -1;
	}
    
	Mat imgOriginal;
	Mat img_gray;
	Mat imgHSV;
	Mat imgThresholded;
	Mat img_invertThreshold;
	Mat img_grayRGB;
	Mat img_obj;
	Mat img_final;
    
	while (true)
	{
		float hsv[3] = { hue/179.0f, saturation/255.0f, brightness/255.0f };
		
		float hLow = hsv[0] - 0.12f;
		if (hLow < 0) {
			hLow = 0;
		}
		float hHigh = hsv[0] + 0.12f;
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
        
		float vLow = hsv[2] - 0.4f;
		if (vLow < 0) {
			vLow = 0;
		}
		float vHigh = hsv[2] + 0.4f;
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
        
		bool bSuccess = cap.read(imgOriginal); // read a new frame from video
        
		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}
        
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
        img_final = img_grayRGB + img_obj;
        
		//Add indicator lines.
		trackFilteredObject(imgThresholded, img_final);
        imshow("Final", img_final);
        
		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
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