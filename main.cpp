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
#include "ImageUtils.h"		// easy image cropping, resizing, rotating, etc

using namespace cv;
using namespace std;

/// Function Prototypes
// This function is automatically called whenever the user clicks the mouse in the window.
void mouseEvent(int ievent, int x, int y, int flags, void* param);

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

char *windowMain = "HSV Color Wheel. Click a color, or press ESC to quit";	// title of the window

int framewidth = 680;
int frameheight = 480;

int hue = 90;			// This variable is adjusted by the user's trackbar at runtime.
int saturation = 240;	//		"
int brightness = 200;	//		"

int mouseX = -1;	// Position in the window that a user clicked the mouse button.
int mouseY = -1;	//		"


float* hexToHSV(int hexValue) {
	float r, g, b, h, s, v;
	r = ((hexValue >> 16) & 0xFF) / 255.0f;
	g = ((hexValue >> 8) & 0xFF) / 255.0f;
	b = ((hexValue)& 0xFF) / 255.0f;

	cout << "Original hex value " << hexValue << endl;
	cout << "Red: " << r * 255 << endl;
	cout << "Green: " << g * 255 << endl;
	cout << "Blue: " << b * 255 << endl;

	float K = 0.f;

	if (g < b)
	{
		std::swap(g, b);
		K = -1.f;
	}

	if (r < g)
	{
		std::swap(r, g);
		K = -2.f / 6.f - K;
	}

	float chroma = r - std::min(g, b);
	h = fabs(K + (g - b) / (6.f * chroma + 1e-20f));
	s = chroma / (r + 1e-20f);
	v = r;

	cout << "on GIMP scale" << endl;
	cout << "Hue: " << h * 360 << endl;
	cout << "Saturation: " << s * 100 << endl;
	cout << "Value: " << v * 100 << endl;

	static float hsv[3];

	hsv[0] = h;
	hsv[1] = s;
	hsv[2] = v;

	return hsv;
}

string intToString(int number){

	std::stringstream ss;
	ss << number;
	return ss.str();
}

void drawObject(int x, int y, Mat &frame){

	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!

	//UPDATE:JUNE 18TH, 2013
	//added 'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)

	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25>0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25<frameheight)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, frameheight), Scalar(0, 255, 0), 2);
	if (x - 25>0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25<framewidth)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(framewidth, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);

}

void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed){
	Mat temp;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	drawContours(cameraFeed, contours, -1, cv::Scalar(0, 0, 255), 3);
}

int main(int argc, char** argv)
{
	// Create a GUI window
	cvNamedWindow(windowMain, 1);
	
	VideoCapture cap(0); //capture the video from webcam

	cap.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 720);

	frameheight = cap.get(3);
	framewidth = cap.get(4);

	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the web cam" << endl;
		return -1;
	}

	//int hexValue;

	//cout << "Enter a hex color code: ";
	//cin >> hexValue;

	int iLastX = -1;
	int iLastY = -1;

	//Capture a temporary image from the camera
	Mat imgTmp;
	cap.read(imgTmp);

	//Create a black image with the size as the camera output
	Mat imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);;


	while (true)
	{
		// Etiher call to hexToHSV() or give hsv float array.
		//float *ranges = HSVRanges(hexToHSV(0x603B15));
		float hsv[3] = { hue/179.0f, saturation/255.0f, brightness/255.0f };
		
		float hLow = hsv[0] - 0.07f;
		if (hLow < 0) {
			hLow = 0;
		}
		float hHigh = hsv[0] + 0.07f;
		if (hHigh > 1) {
			hHigh = 1;
		}

		float sLow = hsv[1] - 0.25f;
		if (sLow < 0) {
			sLow = 0;
		}
		float sHigh = hsv[1] + 0.25f;
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
		int iLowH = ranges[0] * 179;
		int iHighH = ranges[3] * 179;
		int iLowS = ranges[1] * 255;
		int iHighS = ranges[4] * 255;
		int iLowV = ranges[2] * 255;
		int iHighV = ranges[5] * 255;

		Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		Mat imgHSV;

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

		Mat imgThresholded;

		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

		//morphological opening (removes small objects from the foreground)
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		//morphological closing (removes small holes from the foreground)
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		//Calculate the moments of the thresholded image
		/*Moments oMoments = moments(imgThresholded);

		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero
		if (dArea > 10000)
		{
			//calculate the position of the ball
			int posX = dM10 / dArea;
			int posY = dM01 / dArea;

			if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0)
			{
				//Draw a red line from the previous point to the current point
				line(imgLines, Point(posX, posY), Point(iLastX, iLastY), Scalar(0, 0, 255), 2);
			}

			iLastX = posX;
			iLastY = posY;
		}*/

		imshow("Thresholded Image", imgThresholded); //show the thresholded image

		imgOriginal = imgOriginal + imgLines;

		int x = 0, y = 0;

		trackFilteredObject(x, y, imgThresholded, imgOriginal);
		imshow("Original", imgOriginal); //show the original image

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}

		// Allow the user to click on Hue chart to change the hue, or click on the color wheel to see a value.
		cvSetMouseCallback(windowMain, &mouseEvent, 0);

		displayColorWheelHSV(hue, saturation, brightness, windowMain);

		//	cvShowImage(windowMain, imageIn);
		//cvWaitKey();
		//cvDestroyWindow(windowMain);
	}
	return 0;
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