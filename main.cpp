#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#include <cstdio>	// Used for "printf"
#include <string>	// Used for C++ strings
#include <iostream>	// Used for C++ cout print statements

// Include OpenCV libraries
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>

#include "ImageUtils.h"		// easy image cropping, resizing, rotating, etc

using namespace cv;
using namespace std;

// Globals
const int HUE_RANGE = 180;	// OpenCV just uses Hues between 0 to 179!

const int WIDTH = 361;		// Window size
const int HEIGHT = 306;		//		"
const int HUE_HEIGHT = 25;		// thickness of Hue chart
const int WHEEL_TOP = HUE_HEIGHT + 20;		// y position for top of color wheel (= Hue height + gap)
const int WHEEL_BOTTOM = WHEEL_TOP + 255;	// y position for bottom of color wheel
const int TILE_LEFT = 280;	// Position of small tile showing highlighted color
const int TILE_TOP = 140;	//		"
const int TILE_W = 60;		//		"
const int TILE_H = 60;		//		"

char *windowMain = "HSV Color Wheel. Click a color, or press ESC to quit";	// title of the window

int FRAME_WIDTH = 680;
int FRAME_HEIGHT = 480;

int hue = 90;			// This variable is adjusted by the user's trackbar at runtime.
int saturation = 240;	//		"
int brightness = 200;	//		"

int mouseX = -1;	// Position in the window that a user clicked the mouse button.
int mouseY = -1;	//		"

void displayColorWheelHSV(void)
{
	IplImage *imageHSV = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 3);
	int h = imageHSV->height;			// Pixel height
	int w = imageHSV->width;			// Pixel width
	int rowSize = imageHSV->widthStep;	// Size of row in bytes, including extra padding
	char *imOfs = imageHSV->imageData;	// Pointer to the start of the image HSV pixels.

	// Clear the image to grey (Saturation=0)
	cvSet(imageHSV, cvScalar(0, 0, 210, 0));

	// Draw the hue chart on the top, at double width.
	for (int y = 0; y<HUE_HEIGHT; y++) {
		for (int x = 0; x<HUE_RANGE; x++) {
			uchar h = x;		// Hue (0 - 179)
			uchar s = 255;		// max Saturation => most colorful
			uchar v = 255;		// max Value => brightest color
			// Highlight the current value
			if ((h == hue - 2 || h == hue + 2) && (y < HUE_HEIGHT / 2)) {
				s = 0;	// make it white instead of the color
			}
			// Set the HSV pixel components
			*(uchar*)(imOfs + y*rowSize + (x * 2 + 0) * 3 + 0) = h;
			*(uchar*)(imOfs + y*rowSize + (x * 2 + 0) * 3 + 1) = s;
			*(uchar*)(imOfs + y*rowSize + (x * 2 + 0) * 3 + 2) = v;
			*(uchar*)(imOfs + y*rowSize + (x * 2 + 1) * 3 + 0) = h;
			*(uchar*)(imOfs + y*rowSize + (x * 2 + 1) * 3 + 1) = s;
			*(uchar*)(imOfs + y*rowSize + (x * 2 + 1) * 3 + 2) = v;
		}
	}

	// Draw the color wheel: Saturation on the x-axis and Value (brightness) on the y-axis.
	for (int y = 0; y<255; y++) {
		for (int x = 0; x<255; x++) {
			uchar h = hue;		// Hue (0 - 179)
			uchar s = x;		// Saturation (0 - 255)
			uchar v = (255 - y);	// Value (Brightness) (0 - 255)
			// Highlight the current value
			if ((s == saturation - 2 || s == saturation - 3 || s == saturation + 2 || s == saturation + 3) && (v == brightness - 2 || v == brightness - 3 || v == brightness + 2 || v == brightness + 3)) {
				s = 0;	// make it white instead of the color
				v = 0;	// bright white
			}
			// Set the HSV pixel components
			*(uchar*)(imOfs + (y + WHEEL_TOP)*rowSize + x * 3 + 0) = h;
			*(uchar*)(imOfs + (y + WHEEL_TOP)*rowSize + x * 3 + 1) = s;
			*(uchar*)(imOfs + (y + WHEEL_TOP)*rowSize + x * 3 + 2) = v;
		}
	}

	// Draw a small tile of the highlighted color.
	for (int y = 0; y<TILE_H; y++) {
		for (int x = 0; x<TILE_W; x++) {
			// Set the HSV pixel components
			*(uchar*)(imOfs + (y + TILE_TOP)*rowSize + (x + TILE_LEFT) * 3 + 0) = hue;
			*(uchar*)(imOfs + (y + TILE_TOP)*rowSize + (x + TILE_LEFT) * 3 + 1) = saturation;
			*(uchar*)(imOfs + (y + TILE_TOP)*rowSize + (x + TILE_LEFT) * 3 + 2) = brightness;
		}
	}

	// Convert the HSV image to RGB (BGR) for displaying
	IplImage *imageRGB = cvCreateImage(cvSize(imageHSV->width, imageHSV->height), 8, 3);
	cvCvtColor(imageHSV, imageRGB, CV_HSV2BGR);	// (note that OpenCV stores RGB images in B,G,R order.

	// Get the highlighted color's RGB (BGR) values
	h = imageRGB->height;			// Pixel height
	w = imageRGB->width;			// Pixel width
	rowSize = imageRGB->widthStep;	// Size of row in bytes, including extra padding
	imOfs = imageRGB->imageData;	// Pointer to the start of the image HSV pixels.
	uchar R = *(uchar*)(imOfs + (255 - brightness + WHEEL_TOP)*rowSize + (saturation)* 3 + 2);	// Red
	uchar G = *(uchar*)(imOfs + (255 - brightness + WHEEL_TOP)*rowSize + (saturation)* 3 + 1);	// Green
	uchar B = *(uchar*)(imOfs + (255 - brightness + WHEEL_TOP)*rowSize + (saturation)* 3 + 0);	// Blue
	cout << "H:" << hue << ", S:" << saturation << ", V:" << brightness << "  ->  R:" << (int)R << ", G:" << (int)G << ", B:" << (int)B << endl;

	// Display the RGB image
	cvShowImage(windowMain, imageRGB);
	cvReleaseImage(&imageRGB);
	cvReleaseImage(&imageHSV);
}

// This function is automatically called whenever the user changes the trackbar value.
void hue_trackbarWasChanged(int)
{
	displayColorWheelHSV();
}

// This function is automatically called whenever the user clicks the mouse in the window.
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
				cvSetTrackbarPos("Hue", windowMain, hue);	// update the GUI Trackbar
				// Note that "cvSetTrackbarPos()" will implicitly call "displayColorWheelHSV()" for a changed hue.
				//displayColorWheelHSV();
			}
		}
		// If they clicked on the Color wheel, select the new value.
		else if (mouseY >= WHEEL_TOP && mouseY <= WHEEL_BOTTOM) {
			if (mouseX < 256) {	// Make sure its a valid Saturation & Value
				saturation = mouseX;
				brightness = 255 - (mouseY - WHEEL_TOP);
				cvSetTrackbarPos("Saturation", windowMain, saturation);	// update the GUI Trackbar
				cvSetTrackbarPos("Brightness", windowMain, brightness);	// update the GUI Trackbar
				// Note that "cvSetTrackbarPos()" will implicitly call "displayColorWheelHSV()" for saturation or brightness.
				//displayColorWheelHSV();
			}
		}
	}
}

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
	if (y + 25<FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25>0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25<FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

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

	cap.set(CV_CAP_PROP_FRAME_WIDTH, 1280);	//TODO: This res might be too high.
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 720);

	FRAME_HEIGHT = cap.get(3);
	FRAME_WIDTH = cap.get(4);

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

		// Allow the user to change the Hue value upto 179, since OpenCV uses Hues upto 180.
		cvCreateTrackbar("Hue", windowMain, &hue, HUE_RANGE - 1, &hue_trackbarWasChanged);
		cvCreateTrackbar("Saturation", windowMain, &saturation, 255, &hue_trackbarWasChanged);
		cvCreateTrackbar("Brightness", windowMain, &brightness, 255, &hue_trackbarWasChanged);
		// Allow the user to click on Hue chart to change the hue, or click on the color wheel to see a value.
		cvSetMouseCallback(windowMain, &mouseEvent, 0);

		displayColorWheelHSV();

		//	cvShowImage(windowMain, imageIn);
		//cvWaitKey();
		//cvDestroyWindow(windowMain);
	}
	return 0;
}
