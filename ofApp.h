#pragma once

#define MAX_BOLBS 10
#define MAX_WRITE_COUNT 80
#define BETWEEN_HANDS 15

#define ERROR_CHECK( ret )  \
if ((ret) != S_OK) {\
	std::stringstream ss;	\
	ss << "failed " #ret " " << std::hex << ret << std::endl;			\
	throw std::runtime_error(ss.str().c_str());			\
}

#include "ofMain.h"
#include "../apps/myApps/mySketch/ComPtr.h"
#include "ofxBox2d.h"
#include "ofxOpenCv.h"
#include <kinect.h>

class CustomParticle : public ofxBox2dCircle {

public:
	CustomParticle() {
	}
	ofColor color;
	void draw() {
		float radius = getRadius();

		glPushMatrix();
		glTranslatef(getPosition().x, getPosition().y, 0);

		ofSetColor(color.r, color.g, color.b);
		ofFill();
		ofCircle(0, 0, radius);

		glPopMatrix();

	}
};

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	bool initKinect();
	void depthUpdate();
	void colorUpdate();
	void bodyUpdate();
	void setColorFromDepth(ofPoint p);
	ofPoint getColorFromDepth(ofPoint p);

	IKinectSensor *sensor;
	IColorFrameSource *colorSource;
	IDepthFrameSource *depthSource;
	IBodyFrameSource *bodySource;
	IColorFrameSource *colorFrameSource;
	IColorFrameReader *colorReader;
	IDepthFrameReader *depthReader;
	IBodyFrameReader *bodyReader;
	IFrameDescription* colorDescription;
	IFrameDescription* depthDescription;
	IBody* bodies[6];

	vector<UINT16> depthBuffer;
	int colorWidth, colorHeight;
	int depthWidth, depthHeight;
	unsigned int colorBytesPerPixels;
	unsigned int depthBytesPerPixels;
	ComPtr<ICoordinateMapper> mapper;

	ofImage of_colorImage;
	ofxCvColorImage colorImage;
	ofxCvGrayscaleImage depthImage;
	ofxCvGrayscaleImage depthImage_bin;
	ofxCvGrayscaleImage gray;
	int nearThreshold;
	int farThreshold;
	vector<ofPoint> points;
	vector<ofPoint> cPoints;

	ofxBox2d box2d;
	vector<ofPtr<ofxBox2dCircle>> circles;
	vector<int> red;
	vector<int> green;
	vector<int> blue;
	vector<vector<ofPoint>> contours;
	vector<ofPoint> cContours;
	int num_circle;

	ofxCvContourFinder contourFinder;
	ofPolyline shadowLines[MAX_BOLBS];
	ofPtr<ofxBox2dPolygon> shadows[MAX_BOLBS];
	ofImage myImage;

	int frame_count;
	int start_x;
	int start_y;
	int force_x;
	int force_y;
	int force;

	float scale;
	float offsetX;
	float offsetY;

	ofImage sampleImage;
	bool image_write;

	int minDepth;
	int maxDepth;

	unsigned char* colorPixels;

	vector<vector<ofPoint>> colorBodyPoints;
	vector<ofPtr<ofxBox2dCircle>> colorBodyCircles;
	int colorBodyCirclesSize;

	vector<ofPtr<ofxBox2dPolygon>> polyLines;
	vector<ofPolyline> drawings;
	ofPtr<ofxBox2dPolygon> polyLine;

	vector<ofPoint> headPoints;
	vector<BOOL> depthChange;
	int depthChangeThreshold = 200;
	int depthChangeCount = 0;
	BOOL changeToReleased = false;

	ofPoint skeltons[10];
	vector<vector<ofPoint>> skeltonPoints;

	const float range = 0.15;

	vector<vector<ofPoint>> handPoints;
	vector<vector<ofPoint>> colorHandPoints;

	vector<ofPoint> colorMinDepthPoint;

	BOOL write[1920][1080];
	BOOL writeLeft[1920][1080];
	int writeCount[1920][1080];
	int _r, _g, _b;

	ofPoint setColorRGB();
	ofPoint changeColorRGB;
	const int move_value = 5;

	vector<vector<ofPoint>> handPointsLeft;

};