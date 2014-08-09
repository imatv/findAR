#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#include <cstdio>	// Used for "printf"
#include <string>	// Used for C++ strings
#include <stdlib.h>
#include <stdio.h>
#include <iostream>	// Used for C++ cout print statements

// User libraries included here.
#include "HSVColorWheel.h"

// Include OpenCV libraries
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
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
// Calculates image for COLOR_PICK
Mat calcColorPick(Mat imgOriginal);
// Calculates image for OUTLINE
Mat calcOutline(Mat imgOriginal);

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
	ERROR = 0,
	ORIGINAL,
	OUTLINE,
	GRAY,
	BW,
	SEPIA,
	CENSOR,
	HUE,
	//More filters go here.
	COLOR_PICK,
	FACE,
};

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

int main(int argc, char** argv)
{
	// Create a GUI window
	cvNamedWindow(colorWheelTitle, 1);
	
	VideoCapture cap(0); //capture the video from webcam
    
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    
	frameheight = int(cap.get(3));
	framewidth = int(cap.get(4));
    
	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the web cam" << endl;
		return -1;
	}
    
	Mat imgOriginal;
	Mat img_final;
    
	while (true)
	{
		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		int mode = 7;
		/*
		mode = getMode(); //get mode from pebble

		if (!mode &&)
		{
			cout << "Cannot get mode from pebble" << endl;
			break;
		}
		*/

		switch (mode)
		{
		case ORIGINAL:
			img_final = imgOriginal;
			break;
		case OUTLINE:
			img_final = calcOutline(imgOriginal);
			break;
		case GRAY:
			// Convert the image to grayscale
			cvtColor(imgOriginal, img_gray, CV_BGR2GRAY);
			cvtColor(img_gray, img_final, CV_GRAY2BGR);
			break;
		case BW:
			cvtColor(imgOriginal, img_gray, CV_RGB2GRAY);
			img_final = img_gray > 128;
			break;
		case SEPIA:
			transform(imgOriginal, img_final, kern);
			break;
		case CENSOR:
			break;
		case HUE:
			cvtColor(imgOriginal, imgHSV, CV_RGB2HSV);
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
			break;
		//More filters go here.
		case COLOR_PICK:
			img_final = calcColorPick(imgOriginal);
			break;
		case FACE:
			break;
		case ERROR:
		default:
			cout << "Hit break statement ERROR" << endl;
			exit(1);
			break;
		}
		
		imshow("Final", img_final); //show the chosen image
        
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