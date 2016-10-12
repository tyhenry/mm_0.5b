#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"
#include "ofxKinectForWindows2.h"
#include "Kinect.h"
#include "User.h"
#include "VertScreen.h"
#include "MenuSystem.h"
#include "Button.h"
#include "ofxQuad.h"

#define HOVER_WAIT 1.0

class ofApp : public ofBaseApp {

public:

	struct HeadCam {
		ofVec3f headPos = ofVec3f(1.337);
		ofVec3f camPos = ofVec3f(1.337);
		float orbitLong, orbitLat, orbitRad = 1.337;
		ofVec2f camOffset = ofVec2f(1.337);
	};

	void setup();
	void update();
	void draw();
	void exit();

	void setupMenus();
	void moveMenus(float offset); // based on shoulder height
	void buildShirtMesh(float expandPct = 1); // < 1 = shrink, 1 = none, > 1 = expand

	void saveSettings();
	void loadSettings();

	void saveHeadCam();
	int loadHeadCamFolder(string path);
	bool loadHeadCamFile(string path);
	void moveHeadCam(ofVec3f headPos, float pct = 1.0);

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	Kinect kinect;
	User user;

	ofCamera kinectCam; // the viewpoint of the kinect depth sensor
	ofCamera mirrorCam; // the mirror display
	ofEasyCam viewCam; // for viewing 3d scene

	vector<HeadCam> headCams;

	ofVec3f camTarget; // focus of kinectCam & mirrorCam

	VertScreen vertScreen;
	float width = 1920;
	float height = 1080;
	ofRectangle vCrop;

	MenuSystem menus;

	ofImage hover;
	ofImage tshirtImg;
	ofxQuad tshirtQuad;

	vector<ofVec2f> handPos;

	// GUI / SETTINGS

	ofxPanel gui;

	ofParameterGroup drawSettings;
	//----------------------------
	enum CamMode {
		VIEW_MIRROR,
		VIEW_KINECT,
		VIEW_EASY
	};
	ofParameter<bool> bVertScreen;
	ofParameter<int> camMode;
	ofParameter<bool> bDrawTarget, bDrawColor, bDrawSkeleton, bDrawMesh, bMoveMenus, bDrawShirt, bDrawHeadBox, bDrawHeadCams;
	ofParameter<float> maxMeshDepth, meshAlpha, menuOffset, menuMinY, menuMaxY, shirtExpand, uiEasing;

	ofParameterGroup camSettings;
	//---------------------------
	ofParameter<bool> bFlipX;
	ofParameter<ofVec3f> worldTranslate;
	ofParameter<float> worldScale, mirrorLong, mirrorLat, mirrorRad;
	ofParameter<ofVec2f> mirrorLensOffset;
	ofParameter<bool> bUseHeadCams; // moves camera per sample data
	ofParameter<float> headCamEasing; // 0-1, lower eases camera movement

	ofxButton saveButton, loadButton, saveHeadCamButton;
	bool bShowGui = true;
};
