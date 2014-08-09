#include "HSVColorWheel.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <iostream>

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

void displayColorWheelHSV(int hue, int saturation, int brightness, char *windowMain)
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

	// Display the RGB image
	cvShowImage(windowMain, imageRGB);
	cvReleaseImage(&imageRGB);
	cvReleaseImage(&imageHSV);
}



