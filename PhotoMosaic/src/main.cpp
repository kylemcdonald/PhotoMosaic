#include "Matcher.h"
#include "PhotoMosaicUtils.h"
#include "ofMain.h"

using namespace glm;

class ofApp : public ofBaseApp {
public:
    int side, width, height;
    
    ofImage source;
    vector<Tile> sourceTiles, beginTiles, endTiles;
    vector<float> transitionBegin, transitionEnd;
    uint64_t lastReceived = 0;
    uint64_t lastTransitionStart = 0;
    uint64_t lastTransitionStop = 0;
    float transition = 1;
    Matcher matcher;
    bool transitionInProcess = false;
    
    bool allowTransitionCircle = true;
    bool allowTransitionManhattan = true;
    bool transitionCircle = false;
    bool transitionManhattan = false;
    
    float transitionSeconds;
    float onscreenSeconds;
    
    string screenshotFormat;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        ofHideCursor();
        
        side = 30;
        width = ofGetWidth();
        height = ofGetHeight();
        
        // guarantee that width and height are evenly divisible by tile size
        width = side * (width / side);
        height = side * (height / side);
        
        transitionSeconds = 1;
        onscreenSeconds = 10;
        
        matcher.side = side;
        matcher.width = width;
        matcher.height = height;
        matcher.iterations = 100000;
        matcher.highpassSize = 250;
        matcher.highpassContrast = 20;
        
        setupSource();
        beginTiles = sourceTiles;
        endTiles = sourceTiles;
        
        transitionBegin = vector<float>(sourceTiles.size(), 0);
        transitionEnd = vector<float>(sourceTiles.size(), 1);
    }
    void exit() {
        ofLog() << "Shutting down.";
        matcher.close();
    }
    void keyPressed(int key) {
        switch (key) {
            case 'f': ofToggleFullscreen(); break;
            case '0': randomPortrait(); break;
        }
    }
    void randomPortrait() {
        if(!transitionInProcess) {
            loadPortrait(randomChoice(listImages("portraits")).path());
        }
    }
    void loadPortrait(string filename) {
        transitionInProcess = true;
        lastReceived = ofGetElapsedTimeMillis();
        matcher.match(sourceTiles, filename);
    }
    void updateTransition() {
        if(matcher.update()) {
            beginTiles = endTiles;
            endTiles = matcher.getOutput();
            if(endTiles.size() == 0) {
                ofLog() << "No tiles, skipping transition.";
                transitionInProcess = false;
            } else {
                lastTransitionStart = ofGetElapsedTimeMillis();
                
                vec2 center = vec2(width, height) / 2;
                float diagonal = sqrt(width*width + height*height) / 2;
                bool topDown = ofRandomuf() < .5;
                transitionCircle = allowTransitionCircle && ofRandomuf() < .5;
                transitionManhattan = allowTransitionManhattan && ofRandomuf() < .5;
                for(int i = 0; i < sourceTiles.size(); i++) {
                    Tile& cur = endTiles[i];
                    float begin;
                    if(transitionCircle) {
                        begin = ofMap(distance(cur, center), 0, diagonal, 0, .75);
                    } else {
                        if(topDown) {
                            begin = ofMap(cur.y, 0, height, 0, .75);
                        } else {
                            begin = ofMap(cur.y, height, 0, 0, .75);
                        }
                    }
                    float end = begin + .25;
                    transitionBegin[i] = begin;
                    transitionEnd[i] = MIN(end, 1);
                }
            }
        }
        
        float transitionPrev = transition;
        uint64_t transitionState = ofGetElapsedTimeMillis() - lastTransitionStart;
        transition = transitionState / (1000 * transitionSeconds);
        transition = MIN(transition, 1);
        transition = MAX(transition, 0);
        if(transition == 1 && transitionPrev < 1) {
            lastTransitionStop = ofGetElapsedTimeMillis();
            transitionFinished();
            transitionInProcess = false;
        }
    }
    void transitionFinished() {
        
    }
    void setupSource() {
        bool rebuild = false;
        
        ofFile sourceFile("source.tiff");
        if(sourceFile.exists()) {
            ofLog() << "Loading source.";
            source.load(sourceFile.path());
            ofLog() << "Loaded source.";
            if(source.getWidth() != width || source.getHeight() != height) {
                ofLog() << "Source does not match current width and height.";
                rebuild = true;
            }
        } else {
            rebuild = true;
        }
        
        if(rebuild) {
            ofLog() << "Rebuilding source.";
            source = ofImage(buildGrid("db", width, height, side));
            ofLog() << "Rebuilt source.";
        }
        
        source.save(sourceFile);
        sourceTiles = Tile::buildTiles(source, side);
    }
    void update() {
        if(!transitionInProcess) {
            if(ofGetElapsedTimeMillis() - lastTransitionStop > onscreenSeconds * 1000) {
                randomPortrait();
            }
        }
        updateTransition();
    }
    void draw() {
        int n = sourceTiles.size();
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        for(int i = 0; i < n; i++) {
            Tile& begin = beginTiles[i];
            Tile& end = endTiles[i];
            float t = ofMap(transition, transitionBegin[i], transitionEnd[i], 0, 1, true);
            vec2 lerp = transitionManhattan ?
            manhattanLerp(begin, end, smoothstep(t)) :
            euclideanLerp(begin, end, smoothstep(t));
            Tile& s = sourceTiles[i];
            addSubsection(mesh, source.getTexture(), lerp.x, lerp.y, side, side, s.x, s.y);
        }
        source.bind();
        mesh.drawFaces();
        source.unbind();
    }
};

int main() {
    ofGLFWWindowSettings settings;
    settings.multiMonitorFullScreen = true;
    ofCreateWindow(settings)->setFullscreen(true);
    ofRunApp(new ofApp());
}
