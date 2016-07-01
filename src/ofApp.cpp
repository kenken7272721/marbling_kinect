#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    openNIDevice.setup();
    //    openNIDevice.addImageGenerator();
    openNIDevice.addDepthGenerator();
    openNIDevice.setRegister(true);
    openNIDevice.setMirror(true);
    
    openNIDevice.addHandsGenerator();
    
    
    openNIDevice.addAllHandFocusGestures();
    
    openNIDevice.setMaxNumHands(2);
    
    
    
    openNIDevice.start();
    
    
    ofSetVerticalSync(false);
    ofSetLogLevel(OF_LOG_NOTICE);
    
    drawWidth = 1280;
    drawHeight = 720;
    // process all but the density on 16th resolution
    flowWidth = drawWidth/8;
    flowHeight = drawHeight/8;
    
    // FLOW & MASK
    opticalFlow.setup(flowWidth, flowHeight);
    velocityMask.setup(drawWidth, drawHeight);
    
    // FLUID & PARTICLES
#ifdef USE_FASTER_INTERNAL_FORMATS
    fluidSimulation.setup(flowWidth, flowHeight, drawWidth, drawHeight, true);
    particleFlow.setup(flowWidth, flowHeight, drawWidth, drawHeight, true);
#else
    fluidSimulation.setup(flowWidth, flowHeight, drawWidth, drawHeight, false);
    particleFlow.setup(flowWidth, flowHeight, drawWidth, drawHeight, false);
#endif
    
    //    flowToolsLogoImage.loadImage("flowtools.png");
    //    fluidSimulation.addObstacle(flowToolsLogoImage.getTextureReference());
    //    showLogo = true;
    
    // VISUALIZATION
    displayScalar.setup(flowWidth, flowHeight);
    velocityField.setup(flowWidth / 4, flowHeight / 4);
    temperatureField.setup(flowWidth / 4, flowHeight / 4);
    pressureField.setup(flowWidth / 4, flowHeight / 4);
    velocityTemperatureField.setup(flowWidth / 4, flowHeight / 4);
    
    // MOUSE DRAW
    mouseForces.setup(flowWidth, flowHeight, drawWidth, drawHeight);
    
    // CAMERA
    simpleCam.initGrabber(640, 480, true);
    didCamUpdate = false;
    cameraFbo.allocate(640, 480);
    cameraFbo.clear();
    
     lastTime = ofGetElapsedTimef();
    
    ftFluidSimulation();
    ftParticleFlow();
    drawComposite();
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    openNIDevice.update();
    
    deltaTime = ofGetElapsedTimef() - lastTime;
    lastTime = ofGetElapsedTimef();
    
    simpleCam.update();
    
    if (simpleCam.isFrameNew()) {
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        cameraFbo.begin();
        
        if (doFlipCamera)
            simpleCam.draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());  // Flip Horizontal
        else
            simpleCam.draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
        cameraFbo.end();
        ofPopStyle();
        
        opticalFlow.setSource(cameraFbo.getTextureReference());
        opticalFlow.update(deltaTime);
        
        velocityMask.setDensity(cameraFbo.getTextureReference());
        velocityMask.setVelocity(opticalFlow.getOpticalFlow());
        velocityMask.update();
    }
    
    fluidSimulation.addVelocity(opticalFlow.getOpticalFlowDecay());
    fluidSimulation.addDensity(velocityMask.getColorMask());
    fluidSimulation.addTemperature(velocityMask.getLuminanceMask());
    
    mouseForces.update(deltaTime);
    
    for (int i=0; i<mouseForces.getNumForces(); i++) {
        if (mouseForces.didChange(i)) {
            switch (mouseForces.getType(i)) {
                case FT_DENSITY:
                    fluidSimulation.addDensity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_VELOCITY:
                    fluidSimulation.addVelocity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    particleFlow.addFlowVelocity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_TEMPERATURE:
                    fluidSimulation.addTemperature(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_PRESSURE:
                    fluidSimulation.addPressure(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_OBSTACLE:
                    fluidSimulation.addTempObstacle(mouseForces.getTextureReference(i));
                default:
                    break;
            }
        }
    }
    
    fluidSimulation.update();
    
    if (particleFlow.isActive()) {
        particleFlow.setSpeed(fluidSimulation.getSpeed());
        particleFlow.setCellSize(fluidSimulation.getCellSize());
        particleFlow.addFlowVelocity(opticalFlow.getOpticalFlow());
        particleFlow.addFluidVelocity(fluidSimulation.getVelocity());
        //particleFlow.addDensity(fluidSimulation.getDensity());
        particleFlow.setObstacle(fluidSimulation.getObstacle());
    }
    particleFlow.update();

}

//--------------------------------------------------------------
void ofApp::draw(){
    drawSource(0, 0, ofGetWidth(), ofGetHeight());
    drawComposite(0, 0, ofGetWidth(), ofGetHeight());
    
    ofPushMatrix();
   
    
    // draw debug (ie., image, depth, skeleton)
    openNIDevice.drawDebug(0, 0, ofGetWidth(), ofGetHeight());
    ofPopMatrix();
    

    // get number of current hands
    int numHands = openNIDevice.getNumTrackedHands();
    
    // iterate through users
    for (int i = 0; i < numHands; i++){
        
        // get a reference to this user
        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
        
        // get hand position
        ofPoint & handPosition = hand.getPosition();
        // do something with the positions like:
        
        
        
        // draw a rect at the position (don't confuse this with the debug draw which shows circles!!)
        ofSetColor(255);
        ofCircle(handPosition.x, handPosition.y, 20);
        
       
        
        }
    
    
    fluidSimulation.draw(0, 0, ofGetWidth(), ofGetHeight());
    particleFlow.draw(0, 0, ofGetWidth(), ofGetHeight());
    
    
//    ofClear(0,0);
//    if (doDrawCamBackground.get())
//        drawSource();
    
    
//    if (!toggleGuiDraw) {
//        ofHideCursor();
//        drawComposite();
//    }
//    else {
//        ofShowCursor();
//        switch(drawMode.get()) {
//            case DRAW_COMPOSITE: drawComposite(); break;
//            case DRAW_PARTICLES: drawParticles(); break;
//            case DRAW_FLUID_FIELDS: drawFluidFields(); break;
//            case DRAW_FLUID_DENSITY: drawFluidDensity(); break;
//            case DRAW_FLUID_VELOCITY: drawFluidVelocity(); break;
//            case DRAW_FLUID_PRESSURE: drawFluidPressure(); break;
//            case DRAW_FLUID_TEMPERATURE: drawFluidTemperature(); break;
//            case DRAW_FLUID_DIVERGENCE: drawFluidDivergence(); break;
//            case DRAW_FLUID_VORTICITY: drawFluidVorticity(); break;
//            case DRAW_FLUID_BUOYANCY: drawFluidBuoyance(); break;
//            case DRAW_FLUID_OBSTACLE: drawFluidObstacle(); break;
//            case DRAW_FLOW_MASK: drawMask(); break;
//            case DRAW_OPTICAL_FLOW: drawOpticalFlow(); break;
//            case DRAW_SOURCE: drawSource(); break;
//            case DRAW_MOUSE: drawMouseForces(); break;
//        }
//    }
    
}




//--------------------------------------------------------------
//void ofApp::drawModeSetName(int &_value) {
//    switch(_value) {
//        case DRAW_COMPOSITE:		drawName.set("Composite      (1)"); break;
//        case DRAW_PARTICLES:		drawName.set("Particles      "); break;
//        case DRAW_FLUID_FIELDS:		drawName.set("Fluid Fields   (2)"); break;
//        case DRAW_FLUID_DENSITY:	drawName.set("Fluid Density  "); break;
//        case DRAW_FLUID_VELOCITY:	drawName.set("Fluid Velocity (3)"); break;
//        case DRAW_FLUID_PRESSURE:	drawName.set("Fluid Pressure (4)"); break;
//        case DRAW_FLUID_TEMPERATURE:drawName.set("Fld Temperature(5)"); break;
//        case DRAW_FLUID_DIVERGENCE: drawName.set("Fld Divergence "); break;
//        case DRAW_FLUID_VORTICITY:	drawName.set("Fluid Vorticity"); break;
//        case DRAW_FLUID_BUOYANCY:	drawName.set("Fluid Buoyancy "); break;
//        case DRAW_FLUID_OBSTACLE:	drawName.set("Fluid Obstacle "); break;
//        case DRAW_OPTICAL_FLOW:		drawName.set("Optical Flow   (6)"); break;
//        case DRAW_FLOW_MASK:		drawName.set("Flow Mask      (7)"); break;
//        case DRAW_SOURCE:			drawName.set("Source         "); break;
//        case DRAW_MOUSE:			drawName.set("Left Mouse     (8)"); break;
//    }
//}
//
////--------------------------------------------------------------
void ofApp::drawComposite(int _x, int _y, int _width, int _height) {
    
    ofPushStyle();
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    fluidSimulation.draw(_x, _y, _width, _height);
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    if (particleFlow.isActive())
        particleFlow.draw(_x, _y, _width, _height);
    
    //    if (showLogo) {
    //        flowToolsLogoImage.draw(_x, _y, _width, _height);
    //    }
    
    ofPopStyle();
    
}

////--------------------------------------------------------------
//void ofApp::drawParticles(int _x, int _y, int _width, int _height) {
//    //    ofPushStyle();
//    //    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//    //    if (particleFlow.isActive())
//    //        particleFlow.draw(_x, _y, _width, _height);
//    //    ofPopStyle();
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidFields(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    
//    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//    pressureField.setPressure(fluidSimulation.getPressure());
//    pressureField.draw(handPosition.x, handPosition.y, _width, _height);
//    velocityTemperatureField.setVelocity(fluidSimulation.getVelocity());
//    velocityTemperatureField.setTemperature(fluidSimulation.getTemperature());
//    velocityTemperatureField.draw(handPosition.x, handPosition.y, _width, _height);
//    temperatureField.setTemperature(fluidSimulation.getTemperature());
//    
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidDensity(int _x, int _y, int _width, int _height) {
//    
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    
//    fluidSimulation.draw(handPosition.x, handPosition.y, _width, _height);
//    
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidVelocity(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        //		ofEnableBlendMode(OF_BLENDMODE_ALPHA); // altenate mode
//        displayScalar.setSource(fluidSimulation.getVelocity());
//        displayScalar.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ADD);
//        velocityField.setVelocity(fluidSimulation.getVelocity());
//        velocityField.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidPressure(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    ofClear(128);
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        displayScalar.setSource(fluidSimulation.getPressure());
//        displayScalar.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//        pressureField.setPressure(fluidSimulation.getPressure());
//        pressureField.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidTemperature(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        displayScalar.setSource(fluidSimulation.getTemperature());
//        displayScalar.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ADD);
//        temperatureField.setTemperature(fluidSimulation.getTemperature());
//        temperatureField.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidDivergence(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        displayScalar.setSource(fluidSimulation.getDivergence());
//        displayScalar.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ADD);
//        temperatureField.setTemperature(fluidSimulation.getDivergence());
//        temperatureField.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidVorticity(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        displayScalar.setSource(fluidSimulation.getConfinement());
//        displayScalar.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ADD);
//        ofSetColor(255, 255, 255, 255);
//        velocityField.setVelocity(fluidSimulation.getConfinement());
//        velocityField.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidBuoyance(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        displayScalar.setSource(fluidSimulation.getSmokeBuoyancy());
//        displayScalar.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ADD);
//        velocityField.setVelocity(fluidSimulation.getSmokeBuoyancy());
//        velocityField.draw(handPosition.x, handPosition.y, _width, _height);
//    }
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawFluidObstacle(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//    fluidSimulation.getObstacle().draw(handPosition.x, handPosition.y, _width, _height);
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawMask(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//    velocityMask.draw(handPosition.x, handPosition.y, _width, _height);
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
//void ofApp::drawOpticalFlow(int _x, int _y, int _width, int _height) {
//    
//    ofPushStyle();
//    //    	opticalFlow.getOpticalFlow().draw(_x, _y, _width, _height);
//    
//    if (showScalar.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//        displayScalar.setSource(opticalFlow.getOpticalFlowDecay());
//        displayScalar.draw(0, 0, _width, _height);
//    }
//    if (showField.get()) {
//        ofEnableBlendMode(OF_BLENDMODE_ADD);
//        velocityField.setVelocity(opticalFlow.getOpticalFlowDecay());
//        velocityField.draw(0, 0, _width, _height);
//    }
//    ofPopStyle();
//}
//
////--------------------------------------------------------------
//void ofApp::drawSource(int _x, int _y, int _width, int _height) {
//    for (int i = 0; i < numHands; i++){
//        
//        // get a reference to this user
//        ofxOpenNIHand & hand = openNIDevice.getTrackedHand(i);
//        
//        // get hand position
//        ofPoint & handPosition = hand.getPosition();
//    ofPushStyle();
//    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
//    cameraFbo.draw(handPosition.x, handPosition.y, _width, _height);
//    ofPopStyle();
//    }
//}
//
////--------------------------------------------------------------
void ofApp::drawMouseForces(int _x, int _y, int _width, int _height) {
    
    ofPushStyle();
    ofClear(0,0);
    
    for(int i=0; i<mouseForces.getNumForces(); i++) {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        if (mouseForces.didChange(i)) {
            ftDrawForceType dfType = mouseForces.getType(i);
            if (dfType == FT_DENSITY)
                mouseForces.getTextureReference(i).draw(_x, _y, _width, _height);
        }
    }
    
    for(int i=0; i<mouseForces.getNumForces(); i++) {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        if (mouseForces.didChange(i)) {
            switch (mouseForces.getType(i)) {
                case FT_VELOCITY:
                    velocityField.setVelocity(mouseForces.getTextureReference(i));
                    velocityField.draw(_x, _y, _width, _height);
                    break;
                case FT_TEMPERATURE:
                    temperatureField.setTemperature(mouseForces.getTextureReference(i));
                    temperatureField.draw(_x, _y, _width, _height);
                    break;
                case FT_PRESSURE:
                    pressureField.setPressure(mouseForces.getTextureReference(i));
                    pressureField.draw(_x, _y, _width, _height);
                    break;
                default:
                    break;
            }
        }
    }
    
    ofPopStyle();
    
}

//-------------------------------------------------------------
void ofApp::handEvent(ofxOpenNIHandEvent & event){
    // show hand event messages in the console
    ofLogNotice() << getHandStatusAsString(event.handStatus) << "for hand" << event.id << "from device" << event.deviceID;
}

//--------------------------------------------------------------
void ofApp::exit(){
    openNIDevice.stop();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
//    switch (key) {
//        case 'G':
//        case 'g': toggleGuiDraw = !toggleGuiDraw; break;
//        case 'f':
//        case 'F': doFullScreen.set(!doFullScreen.get()); break;
//        case 'c':
//        case 'C': doDrawCamBackground.set(!doDrawCamBackground.get()); break;
//            
//        case '1': drawMode.set(DRAW_COMPOSITE); break;
//        case '2': drawMode.set(DRAW_FLUID_FIELDS); break;
//        case '3': drawMode.set(DRAW_FLUID_VELOCITY); break;
//        case '4': drawMode.set(DRAW_FLUID_PRESSURE); break;
//        case '5': drawMode.set(DRAW_FLUID_TEMPERATURE); break;
//        case '6': drawMode.set(DRAW_OPTICAL_FLOW); break;
//        case '7': drawMode.set(DRAW_FLOW_MASK); break;
//        case '8': drawMode.set(DRAW_MOUSE); break;
//            
//        case 'r':
//        case 'R':
//            fluidSimulation.reset();
//            //            fluidSimulation.addObstacle(flowToolsLogoImage.getTextureReference());
//            mouseForces.reset();
//            break;
//            
//        default: break;
//    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
