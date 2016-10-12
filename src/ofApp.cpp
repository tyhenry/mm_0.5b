#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	//ofLogToFile("log.txt");
	//ofSetLogLevel(OF_LOG_VERBOSE);

	ofLog() << ofGetWindowWidth() << "," << ofGetWindowHeight();

	ofBackground(0);
	width = ofGetWidth();
	height = ofGetHeight();

	// get extents of vertical screen
	vCrop.setWidth(height*height / width);
	vCrop.setPosition(ofVec2f(width*0.5 - vCrop.width*0.5, 0));
	vCrop.setHeight(height);

	// init kinect
	kinect.open();
	kinect.init(true, true, true, true); // color, body, depth, body idx
	user = User(kinect.getCoordinateMapper());

	// ui
	hover.load(ofToDataPath("hover.png"));

	// shirt
	tshirtImg.load(ofToDataPath("tshirt.png"));
	tshirtQuad = ofxQuad(ofPoint(0), ofPoint(0), ofPoint(0), ofPoint(0), 10, 10, tshirtImg.getWidth(), tshirtImg.getHeight(), OF_PRIMITIVE_TRIANGLES);

	// ui -- menus
	setupMenus();

	// cameras

	camTarget = ofVec3f(0, 0, kinectCam.getImagePlaneDistance());
	ofLogNotice("ofApp::setup") << " cam target: " << camTarget;

	kinectCam.setNearClip(0);
	kinectCam.setFarClip(10000);
	kinectCam.setPosition(0, 0, 0);
	kinectCam.lookAt(camTarget);

	mirrorCam.setNearClip(0);
	mirrorCam.setFarClip(10000);
	mirrorCam.setPosition(0, 0, 0);
	mirrorCam.lookAt(camTarget);

	viewCam.setNearClip(0);
	viewCam.setFarClip(10000);
	viewCam.setPosition(0, 0, 0);
	viewCam.setTarget(camTarget);

	// load cam sample data
	loadHeadCamFolder("headcams");

	// two hands!
	handPos.resize(2);

	// gui / settings

	drawSettings.setName("draw settings");
	drawSettings.add(bVertScreen.set("vert screen", false));
	drawSettings.add(camMode.set("camera", 0, 0, 2));
	drawSettings.add(bDrawTarget.set("draw [t]arget", false));
	drawSettings.add(bDrawColor.set("draw color [i]mg", false));
	drawSettings.add(bDrawSkeleton.set("draw [u]ser skeleton", true));
	drawSettings.add(bDrawMesh.set("draw user [m]esh", true));
	drawSettings.add(maxMeshDepth.set("max mesh depth", 0.02, 0.0, 0.1));
	drawSettings.add(meshAlpha.set("mesh alpha", 1, 0, 1));
	drawSettings.add(bMoveMenus.set("move menus", false));
	drawSettings.add(menuOffset.set("menu y offset", 0, -600, 600));
	drawSettings.add(menuMinY.set("menu min y", 0, -vCrop.getBottom(), 0));
	drawSettings.add(menuMaxY.set("menu max y", vCrop.getBottom(), 0, vCrop.getBottom()));
	drawSettings.add(bDrawShirt.set("draw shirt", true));
	drawSettings.add(shirtExpand.set("shirt expand", 1.0, 0.0, 2.0));
	drawSettings.add(uiEasing.set("motion ease", 0.5, 0.0, 1.0));
	drawSettings.add(bDrawHeadBox.set("draw head box", false));

	camSettings.setName("cam settings");
	camSettings.add(bFlipX.set("[f]lip x", true));
	camSettings.add(worldTranslate.set("translate world", ofVec3f(0), ofVec3f(-3), ofVec3f(3)));
	camSettings.add(worldScale.set("scale world", 500, 1, 1000));
	camSettings.add(mirrorLong.set("mirror cam long", 0, -180, 180));
	camSettings.add(mirrorLat.set("mirror cam lat", 0, -180, 180));
	camSettings.add(mirrorRad.set("mirror cam rad", 0, -1000, 1000));
	camSettings.add(mirrorLensOffset.set("mirror cam offset", ofVec2f(0), ofVec2f(-1), ofVec2f(1)));
	camSettings.add(bUseHeadCams.set("use sample cams", true));
	camSettings.add(headCamEasing.set("cam move easing", 1, 0, 1));

	saveButton.addListener(this, &ofApp::saveSettings);
	loadButton.addListener(this, &ofApp::loadSettings);
	saveHeadCamButton.addListener(this, &ofApp::saveHeadCam);

	gui.setup();
	gui.add(drawSettings);
	gui.add(camSettings);
	gui.add(saveButton.setup("save settings"));
	gui.add(loadButton.setup("load settings"));
	gui.add(saveHeadCamButton.setup("save head cam pos"));
	gui.loadFromFile("settings.xml");
	gui.setPosition(width*0.5 - gui.getWidth()*0.5, 10); // center top
}

//--------------------------------------------------------------
void ofApp::update() {

	// update vert screen
	if (bVertScreen) vertScreen.setScreenMode(VertScreen::VertScreenLeft);
	else vertScreen.setScreenMode(VertScreen::HorzScreen);

	// update user translation
	user.setWorldTranslate(worldTranslate.get());
	user.setWorldScale(worldScale.get());
	user.setMirrorX(bFlipX);

	// update mirror camera placement
	mirrorCam.orbit(mirrorLong.get(), mirrorLat.get(), mirrorRad.get(), camTarget);
	mirrorCam.lookAt(camTarget);
	mirrorCam.setLensOffset(mirrorLensOffset.get());

	// update kinect
	kinect.update();
	if (!kinect.hasColorStream()) {
		ofLogVerbose("ofApp::update") << "kinect doesn't have color stream";
		return;
	}

	menus.update(); // update menus (for now, this just updates the carousel animations)

					// get body in bounds
	bool bNewUser = user.setBody(kinect.getCentralBodyPtr());
	// update user
	user.update();

	if (user.hasBody()) { // we have a user

		// move camera 
		if (bUseHeadCams) {
			moveHeadCam(user.getJoint3dPos(JointType_Head), headCamEasing);
		}

		// same user?
		if (!bNewUser) {

			// do hand positions
			ofVec2f hr = handPos[0];
			ofVec2f hl = handPos[1];
			handPos[0] = mirrorCam.worldToScreen(user.getJoint3dPos(JointType_HandRight));
			handPos[1] = mirrorCam.worldToScreen(user.getJoint3dPos(JointType_HandLeft));
			handPos[0] = hr.interpolate(handPos[0], uiEasing);
			handPos[1] = hl.interpolate(handPos[1], uiEasing);

			if (bMoveMenus) {
				moveMenus(menuOffset);
			}

			if (bDrawShirt) {
				buildShirtMesh(shirtExpand);
			}

			// do menu interaction
			vector<Button*> btns = menus.hover(handPos); // returns all hovered buttons
			string selectedBtn = "";
			float longestSelect = 0;
			for (auto btn : btns) {
				// get the button selected the longest
				if (btn->isSelected() && btn->getHoverTime() > longestSelect) {
					selectedBtn = btn->getName();
					longestSelect = btn->getHoverTime();
					ofLogVerbose("ofApp::update") << "button " << selectedBtn << " selected w/ hover of " << longestSelect << " secs" << endl;
				}
			}

			string menuName = menus.getMenuName();
			if (menuName == "style") {
				if (selectedBtn != "") { // any button pressed
					menus.goToMenu("rack");
				}
			}
			else if (menuName == "rack") {
				if (selectedBtn == "selfie") {
					menus.goToMenu("selfie");
				}
				else if (selectedBtn == "style") {
					menus.goToMenu("style");
				}
				else if (selectedBtn == "pageUp") {
					menus.getMenuPtr()->carouselUp(0);
					menus.getMenuPtr()->resetHover();
				}
				else if (selectedBtn == "pageDown") {
					menus.getMenuPtr()->carouselDown(0);
					menus.getMenuPtr()->resetHover();
				}
			}
			else if (menuName == "selfie") {
				if (selectedBtn == "back") { // back btn
					menus.goToMenu("rack");
				}
			}
		}

		// new user
		else {
			menus.goToMenu("style");
		}
	}
	else { // no user, restart
		menus.goToMenu("style");
	}
}

//--------------------------------------------------------------
void ofApp::draw() {

	vertScreen.begin();


	// -------------------------
	//           3D
	// -------------------------

	ofEnableDepthTest();
	ofCamera* camPtr;
	switch (camMode) {
	case VIEW_KINECT:
		camPtr = &kinectCam; break;
	case VIEW_MIRROR: default:
		camPtr = &mirrorCam; break;
	case VIEW_EASY:
		camPtr = &viewCam; break;
	}
	camPtr->begin();

	if (camMode == VIEW_EASY) {
		// draw cameras
		kinectCam.draw();
		mirrorCam.draw();
	}

	if (bDrawTarget) {
		ofPushStyle();
		ofPushMatrix();
		ofMesh target = ofMesh::axis(100);
		ofTranslate(camTarget);
		target.draw();
		ofSetColor(255, 200);
		ofDrawBox(20);
		ofPopMatrix();
		ofPopStyle();
	}

	if (bDrawSkeleton && user.hasBody()) {
		user.drawJoints3d();
	}
	if (bDrawMesh && user.hasBody()) {
		bool bMesh = user.buildMesh(&kinect, 1, maxMeshDepth);
		if (bMesh) {
			// translate/scale/flip 3d space (to match skeleton)
			ofPushMatrix();
			ofTranslate(worldTranslate.get());
			ofScale(worldScale, worldScale, worldScale);
			if (bFlipX) ofScale(-1, 1, 1);
			ofPushStyle();
			int a = ofMap(meshAlpha, 0, 1, 0, 255, true);
			ofSetColor(255, a);
			// draw mesh with color texture
			kinect.getColorTexture().bind();
			user.drawMeshFaces();
			kinect.getColorTexture().unbind();
			ofPopStyle();
			ofPopMatrix();
		}
	}

	if (bDrawHeadBox && user.hasBody()) {
		ofPushMatrix(); ofPushStyle();
		ofPoint headPos = user.getJoint3dPos(JointType_Head);
		ofQuaternion headOrient = user.getBodyPtr()->joints.at(JointType_Head).getOrientation();
		ofTranslate(headPos);
		ofVec3f qAxis; float qAngle;
		headOrient.getRotate(qAngle, qAxis);
		ofRotate(qAngle, qAxis.x, qAxis.y, qAxis.z);
		ofDrawAxis(150);
		int a = ofMap(meshAlpha, 0, 1, 0, 255, true);
		ofSetColor(255, 100);
		ofMesh box = ofMesh::box(150, 150, 150, 1, 1, 1);
		box.drawWireframe();
		ofPopStyle(); ofPopMatrix();
	}

	ofDisableDepthTest();

	// draw shirt
	if (bDrawShirt && user.hasBody()) {
		tshirtImg.bind();
		tshirtQuad.getMesh().draw();
		tshirtImg.unbind();
	}

	camPtr->end();

	// -------------------------
	//           2D
	// -------------------------

	// draw shirt 2d
	//if (bDrawShirt && user.hasBody()) {
	//	ofVec2f tl = mirrorCam.worldToScreen(user.getJoint3dPos(JointType_ShoulderLeft)) - ofVec2f(70, 50);
	//	ofVec2f br = mirrorCam.worldToScreen(user.getJoint3dPos(JointType_HipRight)) + ofVec2f(120, 50);
	//	tshirtImg.draw(tl, br.x - tl.x, br.y - tl.y);
	//}

	// menu
	menus.draw();

	// hand icons
	if (user.hasBody()) {

		string m = menus.getMenuName();

		int alphaR = 255;
		int alphaL = 255;

		if (m != "selfie" && m != "style") {
			// hand alpha based on dist from center
			float distR = abs(handPos[0].x - width*0.5);
			float distL = abs(handPos[1].x - width*0.5);
			float maxDist = abs(vCrop.getLeft() - width*0.5) - 100;
			float minDist = maxDist - 50;
			alphaR = ofMap(distR, minDist, maxDist, 0, 255, true);
			alphaL = ofMap(distL, minDist, maxDist, 0, 255, true);
		}
		else if (m == "selfie") {
			ofVec2f backButton = menus.getMenuPtr()->getButtons()[0].getPos() + ofVec2f(50,50);
			float distR = handPos[0].distance(backButton);
			float distL = handPos[1].distance(backButton);
			float maxDist = 75;
			float minDist = 25;
			alphaR = ofMap(distR, maxDist, minDist, 0, 255, true);
			alphaL = ofMap(distL, maxDist, minDist, 0, 255, true);
		}
		ofPushStyle();
		// right
		ofSetColor(255, alphaR);
		hover.draw(handPos[0] - ofVec2f(50, 50), 100, 100);
		// left
		ofSetColor(255, alphaL);
		hover.draw(handPos[1] - ofVec2f(-50, 50), -100, 100); // flip x
		ofPopStyle();
	}


	if (bShowGui) {
		gui.draw();
	}

	vertScreen.end();

}

void ofApp::exit() {
	saveButton.removeListener(this, &ofApp::saveSettings);
	loadButton.removeListener(this, &ofApp::loadSettings);
}

//--------------------------------------------------------------
void ofApp::setupMenus() {

	menus.clearMenus();

	// 0 style menu

	Menu styleMenu = Menu("style", HOVER_WAIT);
	styleMenu.addButton(ofImage("design_select.png"), "design", vCrop.getLeft() + 25, 300, 200, 200, ofImage("design_select_hover.png"));
	styleMenu.addButton(ofImage("rack_select.png"), "rack", vCrop.getRight() - 225, 300, 200, 200, ofImage("rack_select_hover.png"));
	menus.addMenu(styleMenu);

	// 1 rack menu

	Menu rackMenu = Menu("rack", HOVER_WAIT);
	// left
	rackMenu.addButton(ofImage("style_btn.png"), "style", vCrop.getLeft() + 10, 150, 125, 125);
	rackMenu.addButton(ofImage("shape_btn.png"), "shape", vCrop.getLeft() + 10, 300, 125, 125);
	rackMenu.addButton(ofImage("selfie_btn.png"), "selfie", vCrop.getLeft() + 10, 500, 125, 125);
	// right
	rackMenu.addButton(ofImage("up_btn.png"), "pageUp", vCrop.getRight() - 100, 150, 100, 100);
	vector<ofImage> items;
	vector<string> names;
	for (int i = 0; i < 6; i++) {
		string imgPath = "cat_" + ofToString(i) + ".png";
		items.push_back(ofImage(imgPath));
		names.push_back(imgPath);
	}
	rackMenu.addCarousel("styleSelect", items, names, vCrop.getRight() - 90, 260, 80, 80, 3, 7);
	rackMenu.addButton(ofImage("down_btn.png"), "pageDown", vCrop.getRight() - 100, 525, 100, 100);
	menus.addMenu(rackMenu);

	// 2 - selfie menu

	Menu selfieMenu = Menu("selfie", HOVER_WAIT);
	selfieMenu.addButton(ofImage("back_btn.png"), "back", vCrop.getRight() - 100, 525, 100, 100);
	menus.addMenu(selfieMenu);

}

void ofApp::moveMenus(float offset) {
	// get avg shoulder height on mirror
	float sL = mirrorCam.worldToScreen(user.getJoint3dPos(JointType_ShoulderLeft)).y;
	float sR = mirrorCam.worldToScreen(user.getJoint3dPos(JointType_ShoulderRight)).y;
	float sY = (sL + sR) * 0.5;

	float y = sY + offset;
	if (y > menuMinY && y <= menuMaxY) {

		auto& ms = menus.getMenus();
		for (int i = 0; i < ms.size(); i++) {
			ofVec2f prev = ms[i].getPos();
			ofVec2f next(prev.x, sY + offset);
			ms[i].setPos(prev.interpolate(next, uiEasing));
		}
	}
}

void ofApp::buildShirtMesh(float expandPct) {
	if (user.hasBody()) {

		// get torso
		ofPoint tl = user.getJoint3dPos(JointType_ShoulderLeft);
		ofPoint tr = user.getJoint3dPos(JointType_ShoulderRight);
		ofPoint bl = user.getJoint3dPos(JointType_HipLeft);
		ofPoint br = user.getJoint3dPos(JointType_HipRight);
		// expand
		if (expandPct != 1) {
			// top
			ofPoint eTl = tr.getInterpolated(tl, expandPct).getInterpolated(bl.getInterpolated(tl, expandPct*0.92), 0.5);
			ofPoint eTr = tl.getInterpolated(tr, expandPct).getInterpolated(br.getInterpolated(tr, expandPct*0.92), 0.5);
			// bottom
			ofPoint eBl = br.getInterpolated(bl, expandPct*1.5).getInterpolated(tl.getInterpolated(bl, expandPct), 0.1);
			ofPoint eBr = bl.getInterpolated(br, expandPct*1.5).getInterpolated(tr.getInterpolated(br, expandPct), 0.1);
			tshirtQuad.set(eTl, eTr, eBl, eBr);
		}
		else {
			tshirtQuad.set(tl, tr, bl, br);
		}
		tshirtQuad.interpolateMesh(10, 10);
	}
}

//--------------------------------------------------------------
void ofApp::saveSettings() {
	gui.saveToFile("settings.xml");
}

//--------------------------------------------------------------
void ofApp::loadSettings() {
	gui.loadFromFile("settings.xml");
}

//--------------------------------------------------------------
void ofApp::saveHeadCam() {
	ofxXmlSettings sets;

	// 3d - world space
	ofVec3f hp = user.getJoint3dPos(JointType_Head);
	sets.setValue("pos:world:head:x", hp.x);
	sets.setValue("pos:world:head:y", hp.y);
	sets.setValue("pos:world:head:z", hp.z);
	ofVec3f ls = user.getJoint3dPos(JointType_ShoulderLeft);
	sets.setValue("pos:world:lShoulder:x", ls.x);
	sets.setValue("pos:world:lShoulder:y", ls.y);
	sets.setValue("pos:world:lShoulder:z", ls.z);
	ofVec3f rs = user.getJoint3dPos(JointType_ShoulderRight);
	sets.setValue("pos:world:rShoulder:x", rs.x);
	sets.setValue("pos:world:rShoulder:y", rs.y);
	sets.setValue("pos:world:rShoulder:z", rs.z);
	// 2d - mirror space
	ofVec2f hpCam = mirrorCam.worldToScreen(hp);
	sets.setValue("pos:mirror:head:x", hpCam.x);
	sets.setValue("pos:mirror:head:y", hpCam.y);
	ofVec2f lsCam = mirrorCam.worldToScreen(ls);
	sets.setValue("pos:mirror:lShoulder:x", lsCam.x);
	sets.setValue("pos:mirror:lShoulder:y", lsCam.y);
	ofVec2f rsCam = mirrorCam.worldToScreen(rs);
	sets.setValue("pos:mirror:rShoulder:x", rsCam.x);
	sets.setValue("pos:mirror:rShoulder:y", rsCam.y);


	ofVec3f cp = mirrorCam.getPosition();
	sets.setValue("cam:pos:x", cp.x);
	sets.setValue("cam:pos:y", cp.y);
	sets.setValue("cam:pos:z", cp.z);
	sets.setValue("cam:orbit:long", mirrorLong);
	sets.setValue("cam:orbit:lat", mirrorLat);
	sets.setValue("cam:orbit:rad", mirrorRad);
	sets.setValue("cam:offset:x", mirrorLensOffset.get().x);
	sets.setValue("cam:offset:y", mirrorLensOffset.get().y);
	sets.setValue("world:translate:x", worldTranslate.get().x);
	sets.setValue("world:translate:y", worldTranslate.get().y);
	sets.setValue("world:translate:z", worldTranslate.get().z);
	sets.setValue("world:scale", worldScale);

	string fn = "headCamSettings_" + ofGetTimestampString() + ".xml";
	sets.saveFile(fn);

}

int ofApp::loadHeadCamFolder(string path) {
	ofDirectory dir(path);
	if (!dir.exists()) return 0;
	dir.allowExt("xml");
	dir.listDir();
	if (dir.size() < 1) return 0;
	int nLoaded = 0;
	for (auto& file : dir.getFiles()) {
		stringstream ss; ss << "loading " << file.getAbsolutePath() << "... ";
		if (loadHeadCamFile(file.getAbsolutePath())) {
			ss << "success";
			nLoaded++;
		}
		else ss << "failure";
		ofLogVerbose("ofApp::loadHeadCamFolder") << ss.str();
	}
	return nLoaded;
}

bool ofApp::loadHeadCamFile(string path) {
	ofxXmlSettings hc;
	if (!hc.loadFile(path)) {
		ofLogError("ofApp::loadHeadCamFile") << "couldn't load file " << path;
		return false;
	}
	HeadCam headCam;
	headCam.headPos.x = hc.getValue("pos:world:head:x", 1.337);
	headCam.headPos.y = hc.getValue("pos:world:head:y", 1.337);
	headCam.headPos.z = hc.getValue("pos:world:head:z", 1.337);
	headCam.camPos.x = hc.getValue("cam:pos:x", 1.337);
	headCam.camPos.y = hc.getValue("cam:pos:y", 1.337);
	headCam.camPos.z = hc.getValue("cam:pos:z", 1.337);
	headCam.orbitLong = hc.getValue("cam:orbit:long", 1.337);
	headCam.orbitLat = hc.getValue("cam:orbit:lat", 1.337);
	headCam.orbitRad = hc.getValue("cam:orbit:rad", 1.337);
	headCam.camOffset.x = hc.getValue("cam:offset:x", 1.337);
	headCam.camOffset.y = hc.getValue("cam:offset:y", 1.337);
	if (headCam.headPos == ofVec3f(1.337) || headCam.camPos == ofVec3f(1.337) ||
		headCam.orbitLong == 1.337 || headCam.orbitLat == 1.337 || headCam.orbitRad == 1.337 ||
		headCam.camOffset == ofVec2f(1.337)) {
		ofLogError("ofApp::loadHeadCamFile") << "couldn't load file " << path << " - invalid data";
		return false;
	}
	headCams.push_back(headCam);
	return true;
}

void ofApp::moveHeadCam(ofVec3f headPos, float pct) {

	// find closest 2 head positions in headCams
	HeadCam c1;
	HeadCam c2;
	float minD1 = 10000;
	float minD2 = 10000;
	for (auto& headCam : headCams) {
		float d = headCam.headPos.distance(headPos);
		if (d < minD1) {
			minD1 = d;
			c1 = headCam;
		}
		else if (d < minD2) {
			minD2 = d;
			c2 = headCam;
		}
	}
	if (minD1 == 10000 || minD2 == 10000) {
		ofLogError("ofApp::moveHeadCam") << "couldn't move cam, no close sample data!";
		return;
	}
	// interpolate between the 2 head/cam positions/values based on distance
	float p = minD1 / (minD1 + minD2);
	// move the camera according to interpolation
	HeadCam c;
	c.headPos = headPos;
	c.camPos = c1.camPos.getInterpolated(c2.camPos, p);
	c.orbitLong = c1.orbitLong * (1.0 - p) + c2.orbitLong * p;
	c.orbitLat = c1.orbitLong * (1.0 - p) + c2.orbitLat * p;
	c.orbitRad = c1.orbitRad * (1.0 - p) + c2.orbitRad * p;
	c.camOffset = c1.camOffset.getInterpolated(c2.camOffset, p);

	mirrorLong = ofMap(pct, 0, 1, mirrorLong, c.orbitLong, true);
	mirrorLat = ofMap(pct, 0, 1, mirrorLat, c.orbitLat, true);
	mirrorRad = ofMap(pct, 0, 1, mirrorRad, c.orbitRad, true);
	mirrorLensOffset.set(mirrorLensOffset.get().getInterpolated(c.camOffset, pct));

	mirrorCam.orbit(mirrorLong.get(), mirrorLat.get(), mirrorRad.get(), camTarget);
	mirrorCam.lookAt(camTarget);
	mirrorCam.setLensOffset(mirrorLensOffset.get());
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

	// v - change vertical/horizontal screen
	if (key == 'v' || key == 'V') {
		bVertScreen = !bVertScreen;
	}

	// c - change camera
	else if (key == 'c' || key == 'C') {
		switch (camMode) {
		case VIEW_MIRROR: camMode = VIEW_KINECT; break;
		case VIEW_KINECT: camMode = VIEW_EASY; break;
		case VIEW_EASY: camMode = VIEW_MIRROR; break;
		}
	}

	// t - draw camera target
	else if (key == 't' || key == 'T') bDrawTarget = !bDrawTarget;
	// u - draw user skel
	else if (key == 'u' || key == 'U') bDrawSkeleton = !bDrawSkeleton;
	// m - draw user mesh
	else if (key == 'm' || key == 'M') bDrawMesh = !bDrawMesh;
	// i - draw color img
	else if (key == 'i' || key == 'I') bDrawColor = !bDrawColor;

	// f - flip x
	else if (key == 'f' || key == 'F') bFlipX = !bFlipX;

	// LEFT - back a menu
	else if (key == OF_KEY_LEFT) menus.prev();
	// RIGHT - fwd a menu
	else if (key == OF_KEY_RIGHT) menus.next();

	// g - show/hide gui
	else if (key == 'g' || 'G') bShowGui = !bShowGui;

	// s - save settings
	else if (key == 's' || 'S') gui.saveToFile("settings.xml");
	// l - load settings
	else if (key == 'l' || 'L') gui.loadFromFile("settings.xml");

	// h - save head/cam placement
	else if (key == 'h' || 'H') saveHeadCam();
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
