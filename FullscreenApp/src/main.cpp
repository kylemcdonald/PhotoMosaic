#include "ofMain.h"

class ofApp : public ofBaseApp {
public:
    void setup() {
    }
    void update() {
    }
    void draw() {
        int mode = ofGetFrameNum() % 5;
        switch(mode) {
            case 0: ofBackground(255, 0, 0); break;
            case 1: ofBackground(0, 255, 0); break;
            case 2: ofBackground(0, 0, 255); break;
            case 3: ofBackground(255); break;
            case 4: ofBackground(0); break;
        }
    }
    void keyPressed(int key) {
        if(key == 'f') {
            ofToggleFullscreen();
        }
    }
};

int main() {
    ofGLFWWindowSettings settings;
    settings.width = 1024;
    settings.height = 768;
    settings.multiMonitorFullScreen = true;
    ofCreateWindow(settings)->setFullscreen(true);
    ofRunApp(new ofApp());
}
