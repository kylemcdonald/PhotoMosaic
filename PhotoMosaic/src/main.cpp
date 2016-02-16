/*
 todo:
 - update images while running
 - save screenshots
 */

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxTiming.h"
#include "Highpass.h"

int side, width, height;
float highpassSize, highpassContrast;
int iterations;

template <class T>
const T& randomChoice(const vector<T>& x) {
    return x[ofRandom(x.size())];
}

float smoothstep(float x) {
    return x*x*(3 - 2*x);
}

int getMaxTextureSize() {
    GLint max[2];
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, max);
    return max[0];
}

vector<ofFile> listImages(string directory) {
    ofDirectory dir(directory);
    dir.allowExt("jpg");
    dir.allowExt("jpeg");
    dir.allowExt("png");
    vector<ofFile> files = dir.getFiles();
    ofLog() << "Listed " << files.size() << " files in " << directory << ".";
    return files;
}

ofRectangle getCenterRectangle(const ofImage& img) {
    int width = img.getWidth(), height = img.getHeight();
    int side = MIN(width, height);
    ofRectangle crop;
    crop.setFromCenter(width / 2, height / 2, side, side);
    return crop;
}

void drawCenterSquare(const ofImage& img, float x, float y, float w, float h) {
    ofRectangle crop = getCenterRectangle(img);
    img.drawSubsection(x, y, w, h, crop.x, crop.y, crop.width, crop.height);
}

void getCenterSquare(const ofPixels& img, int w, int h, ofImage& out) {
    ofRectangle crop = getCenterRectangle(img);
    ofPixels cropped;
    img.cropTo(cropped, crop.x, crop.y, crop.width, crop.height);
    out.allocate(w, h, OF_IMAGE_COLOR);
    cropped.resizeTo(out.getPixels(), OF_INTERPOLATE_BICUBIC);
}

vector<pair<int, int>> getGrid(int width, int height, int side) {
    vector<pair<int, int>> grid;
    int m = width / side;
    int n = height / side;
    for(int y = 0; y < n; y++) {
        for(int x = 0; x < m; x++) {
            grid.emplace_back(x * side, y * side);
        }
    }
    return grid;
}

vector<bool> arbTex;
void pushArbTex() {
    arbTex.push_back(ofGetUsingArbTex());
}
void popArbTex() {
    if(arbTex.back()) {
        ofEnableArbTex();
    } else {
        ofDisableArbTex();
    }
    arbTex.pop_back();
}

ofPixels buildGrid(string dir, int width, int height, int side) {
    auto files = listImages(dir);
    
    ofFbo buffer;
    
    ofFbo::Settings settings;
    settings.width = width;
    settings.height = height;
    settings.useDepth = false;
    buffer.allocate(settings);
    
    pushArbTex();
    ofDisableArbTex();
    ofImage img;
    img.getTexture().enableMipmap();
    buffer.begin();
    auto filesitr = files.begin();
    for(auto& position : getGrid(width, height, side)) {
        int x = position.first;
        int y = position.second;
        img.load(*filesitr);
        drawCenterSquare(img, x, y, side, side);
        filesitr++;
        if(filesitr == files.end()) {
            filesitr = files.begin();
        }
    }
    buffer.end();
    popArbTex();
    
    ofPixels out;
    buffer.readToPixels(out);
    return out;
}

ofPixels addToGrid(const ofImage& src, string dir, int width, int height, int side) {
    auto files = listImages(dir);
    if(files.empty()) {
        return src.getPixels();
    }
    
    ofFbo buffer;
    
    ofFbo::Settings settings;
    settings.width = width;
    settings.height = height;
    settings.useDepth = false;
    buffer.allocate(settings);
    
    pushArbTex();
    ofDisableArbTex();
    ofImage img;
    img.getTexture().enableMipmap();
    buffer.begin();
    src.draw(0, 0);
    auto filesitr = files.begin();
    auto positions = getGrid(width, height, side);
    ofRandomize(positions);
    for(auto& position : positions) {
        int x = position.first;
        int y = position.second;
        img.load(*filesitr);
        drawCenterSquare(img, x, y, side, side);
        filesitr++;
        if(filesitr == files.end()) {
            break;
        }
    }
    buffer.end();
    popArbTex();
    
    ofPixels out;
    buffer.readToPixels(out);
    return out;
}

ofColor getAverage(const ofPixels& pix, int x, int y, int w, int h) {
    float r = 0, g = 0, b = 0;
    for(int j = y; j < y + h; j++) {
        for(int i = x; i < x + w; i++) {
            const ofColor& cur = pix.getColor(i, j);
            r += cur.r;
            g += cur.g;
            b += cur.b;
        }
    }
    float n = w * h;
    return ofColor(r / n, g / n, b / n);
}

const int subsampling = 3;
class Tile : public ofVec2f {
public:
    int side;
    vector<ofColor> grid;
    float weight;
    Tile(int x, int y, int side, const vector<ofColor>& grid, float weight)
    :ofVec2f(x, y)
    ,side(side)
    ,grid(grid)
    ,weight(weight) {
    }
    static vector<Tile> buildTiles(const ofPixels& pix, int side) {
        // we could do this with resizing but OF doesn't have a good downsampling method
        float subsample = (float) side / subsampling;
        int w = pix.getWidth(), h = pix.getHeight();
        int nx = w / side, ny = h / side;
        vector<Tile> tiles;
        for(int y = 0; y < h; y+=side) {
            for(int x = 0; x < w; x+=side) {
                vector<ofColor> grid;
                for(int ky = 0; ky < subsampling; ky++) {
                    for(int kx = 0; kx < subsampling; kx++) {
                        grid.push_back(getAverage(pix, x+kx*subsample, y+ky*subsample, subsample, subsample));
                    }
                }
                float distance = ofVec2f(x, y).distance(ofVec2f(w, h) / 2);
                float weight = ofMap(distance, 0, w / 2, 1, 0, true);
                tiles.emplace_back(x, y, side, grid, weight);
            }
        }
        return tiles;
    }
};

// lerps along x then along y, linearly
// a nicer but more complicated implementation would
// lerp the bigger direction first
ofVec2f manhattanLerp(ofVec2f begin, ofVec2f end, float t) {
    float dx = fabs(begin.x - end.x);
    float dy = fabs(begin.y - end.y);
    float dd = dx + dy;
    float dc = dd * t;
    if(dc < dx) { // lerp dx
        float dt = dc / dx;
        return ofVec2f(ofLerp(begin.x, end.x, dt), begin.y);
    } else if(dc < dd) { // lerp dy
        float dt = (dc - dx) / dy;
        return ofVec2f(end.x, ofLerp(begin.y, end.y, dt));
    } else { // when dy or dx+dy is zero
        return ofVec2f(end.x, end.y);
    }
}

ofVec2f euclideanLerp(ofVec2f begin, ofVec2f end, float t) {
    return begin.interpolate(end, t);
}

void addSubsection(ofMesh& mesh, ofTexture& tex, float x, float y, float w, float h, float sx, float sy) {
    ofVec2f nwc = tex.getCoordFromPoint(sx, sy), nwp(x,y);
    ofVec2f nec = tex.getCoordFromPoint(sx + w, sy), nep(x + w, y);
    ofVec2f sec = tex.getCoordFromPoint(sx + w, sy + h), sep(x + w, y + h);
    ofVec2f swc = tex.getCoordFromPoint(sx, sy + h), swp(x, y + h);
    mesh.addTexCoord(nwc); mesh.addVertex(nwp);
    mesh.addTexCoord(nec); mesh.addVertex(nep);
    mesh.addTexCoord(swc); mesh.addVertex(swp);
    mesh.addTexCoord(nec); mesh.addVertex(nep);
    mesh.addTexCoord(sec); mesh.addVertex(sep);
    mesh.addTexCoord(swc); mesh.addVertex(swp);
}

long getCost(const ofColor& c1, const ofColor& c2) {
    long rmean = ((long) c1.r + (long) c2.r) / 2;
    long r = (long) c1.r - (long) c2.r;
    long g = (long) c1.g - (long) c2.g;
    long b = (long) c1.b - (long) c2.b;
    return ((((512 + rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
}

float getCost(const Tile& a, const Tile& b) {
    const int n = subsampling * subsampling;
    float total = 0;
    for(int i = 0; i < n; i++) {
        total += getCost(a.grid[i], b.grid[i]);
    }
    return total * (a.weight + b.weight);
}

class Matcher : public ofThread {
public:
    Matcher()
    :ready(false)
    ,saveImage(false)
    ,processing(false) {
        startThread();
    }
    void close() {
        inputChannel.close();
        outputChannel.close();
        waitForThread(true);
    }
    void match(const vector<Tile>& inputSource, string target, bool saveImage=false) {
        this->inputSource = &inputSource;
        this->saveImage = saveImage;
        inputChannel.send(target);
    }
    bool update() {
        ready = false;
        while(outputChannel.tryReceive(outputData)){
            ready = true;
        }
        return ready;
    }
    bool getProcessing() const {
        return processing;
    }
    const vector<Tile>& getOutput() {
        return outputData;
    }
private:
    bool processing;
    bool ready;
    void threadedFunction() {
        string filename;
        while(inputChannel.receive(filename)) {
            processing = true;
            vector<Tile> data = buildTiles(filename);
            if(data.size() == 0) {
                ofLog() << "No tiles, skipping matching process.";
                continue;
            }
            const vector<Tile>& sourceTiles = *inputSource;
            for(int i = 0; i < iterations; i++) {
                int a = ofRandom(sourceTiles.size()), b = ofRandom(sourceTiles.size());
                const Tile& lefta = sourceTiles[a], leftb = sourceTiles[b];
                const Tile& righta = data[a], rightb = data[b];
                long sumab = getCost(lefta, righta) + getCost(leftb, rightb);
                long sumba = getCost(lefta, rightb) + getCost(leftb, righta);
                if(sumba < sumab) {
                    swap(data[a], data[b]);
                }
            }
            outputChannel.send(data);
            processing = false;
        }
    }
    vector<Tile> buildTiles(string filename) {
        ofPixels image;
        ofLog() << "Loading image " << filename;
        if(!ofLoadImage(image, filename)) {
            ofLogError() << "Error loading image " << filename;
            return {};
        }
        image.setImageType(OF_IMAGE_COLOR);
        if(saveImage) {
            ofSaveImage(image, "portraits/" + ofGetTimestampString() + ".jpg");
        }
        highpass.filter(image, image, highpassSize, highpassContrast);
        ofRectangle originalRect(0, 0, image.getWidth(), image.getHeight());
        ofRectangle targetRect(0, 0, width, height);
        targetRect.scaleTo(originalRect, OF_SCALEMODE_FIT);
        ofPixels cropped;
        image.cropTo(cropped, targetRect.x, targetRect.y, targetRect.width, targetRect.height);
        cropped.resize(width, height, OF_INTERPOLATE_BICUBIC);
        vector<Tile> tiles = Tile::buildTiles(cropped, side);
        return tiles;
    }
    
    Highpass highpass;
    bool saveImage;
    const vector<Tile>* inputSource;
    ofThreadChannel<string> inputChannel;
    ofThreadChannel<vector<Tile>> outputChannel;
    vector<Tile> outputData;
};

void deleteOld(string path, int minutes=1440) {
    path = ofToDataPath(path);
    string cmd = "find " + path + " -name '*.jpg' -mmin +" + ofToString(minutes) + " -delete";
    ofLog() << cmd;
    ofLog() << "> " << ofSystem(cmd);
}

void copyFiles(string from, string to) {
    from = ofToDataPath(from);
    to = ofToDataPath(to);
    string cmd = "rsync " + from + "/* " + to + "/";
    ofLog() << cmd;
    ofLog() << "> " << ofSystem(cmd);
}

class ofApp : public ofBaseApp {
public:
    ofImage source;
    vector<Tile> sourceTiles, beginTiles, endTiles;
    vector<float> transitionBegin, transitionEnd;
    ofxOscReceiver osc;
    uint64_t lastReceived = 0;
    uint64_t lastTransitionStart = 0;
    uint64_t lastTransitionStop = 0;
    float transition = 1;
    Matcher matcher;
    bool debugMode = true;
    bool manualTransition = false;
    bool transitionInProcess = false;
    
    bool transitionCircle = false;
    bool transitionManhattan = false;
    
    int oscPort;
    string ipInterface, ipAddress;
    
    float transitionSeconds;
    float onscreenSeconds;
    
    string screenshotFormat;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        ofHideCursor();
        
        deleteOld("portraits");
        copyFiles("persistent", "portraits");
        
        ofXml xml;
        xml.load("settings.xml");
        side = xml.getIntValue("side");
        width = xml.getIntValue("size/width");
        height = xml.getIntValue("size/height");
        iterations = xml.getIntValue("iterations");
        highpassSize = xml.getIntValue("highpass/size");
        highpassContrast = xml.getIntValue("highpass/contrast");
        transitionSeconds = xml.getFloatValue("transitionSeconds");
        onscreenSeconds = xml.getFloatValue("onscreenSeconds");
        screenshotFormat = xml.getValue("screenshotFormat");
        debugMode = xml.getBoolValue("debugMode");
        oscPort = xml.getIntValue("oscPort");
        osc.setup(oscPort);
        
        updateDebugInfo();
        
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
//            case 'a': loadPortrait("a.jpg"); break;
//            case 'b': loadPortrait("b.jpg"); break;
//            case 'c': loadPortrait("c.jpg"); break;
            case 'd': debugMode = !debugMode; break;
            case 'f': ofToggleFullscreen(); break;
            case '0': randomPortrait(); break;
//            case '2': transitionCircle = !transitionCircle; break;
//            case '3': transitionManhattan = !transitionManhattan; break;
        }
    }
    void randomPortrait() {
        if(!transitionInProcess) {
            loadPortrait(randomChoice(listImages("portraits")).path());
        }
    }
    void loadPortrait(string filename, bool manual = false) {
        transitionInProcess = true;
        manualTransition = manual;
        lastReceived = ofGetElapsedTimeMillis();
        matcher.match(sourceTiles, filename, manualTransition);
    }
    void updateDebugInfo() {
        ipInterface = ofTrim(ofSystem("route -n get 0.0.0.0 2>/dev/null | awk '/interface: / {print $2}'"));
        if(ipInterface == "") {
            ipAddress = "[no connection]";
            ipInterface = "[no interface]";
        } else {
            ipAddress = ofTrim(ofSystem("ipconfig getifaddr " + ipInterface));
        }
    }
    void updateTransition() {
        if(matcher.update()) {
            beginTiles = endTiles;
            endTiles = matcher.getOutput();
            lastTransitionStart = ofGetElapsedTimeMillis();
            
            ofVec2f center = ofVec2f(width, height) / 2;
            float diagonal = sqrt(width*width + height*height) / 2;
//            transitionCircle = ofRandomuf() < .5;
//            transitionManhattan = ofRandomuf() < .5;
            for(int i = 0; i < sourceTiles.size(); i++) {
                Tile& cur = endTiles[i];
                float begin;
                if(transitionCircle) {
                    begin = ofMap(cur.y, 0, height, 0, .75);
                } else {
                    begin = ofMap(cur.y, height, 0, 0, .75);
//                    begin = ofMap(cur.distance(center), 0, diagonal, 0, .75);
                }
                float end = begin + .25;
                transitionBegin[i] = begin;
                transitionEnd[i] = MIN(end, 1);
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
        if(screenshotFormat != "" && manualTransition) {
            ofSaveScreen("screenshots/" + ofGetTimestampString() + "." + screenshotFormat);
        }
    }
    void setupSource(bool rebuild = false) {
        ofFile sourceFile("source.tiff");
        if(sourceFile.exists() && !rebuild) {
            ofLog() << "Loading source.";
            source.load(sourceFile);
            ofLog() << "Loaded source.";
        } else {
            ofLog() << "Rebuilding source.";
            source = ofImage(buildGrid("db", width, height, side));
            ofLog() << "Rebuilt source.";
        }
        source = ofImage(addToGrid(source, "upcoming", width, height, side));
        deleteOld("upcoming", 0);
        source.save(sourceFile);
        ofPixels& pix = source.getPixels();
        sourceTiles = Tile::buildTiles(source, side);
    }
    void update() {
        if(!transitionInProcess) {
            while(osc.hasWaitingMessages()) {
                ofxOscMessage msg;
                osc.getNextMessage(msg);
                string filename = msg.getArgAsString(0);
                loadPortrait(filename, true);
            }
            if(ofGetElapsedTimeMillis() - lastTransitionStop > onscreenSeconds * 1000) {
                randomPortrait();
            }
        }
        updateTransition();
    }
    void drawDebug() {
        ofPushMatrix();
        ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
        ofDrawBitmapStringHighlight(ipAddress + ":" + ofToString(oscPort) + " on " + ipInterface, 0, 0);
        ofDrawBitmapStringHighlight("Screen size: " + ofToString(ofGetScreenWidth()) + "x" + ofToString(ofGetScreenHeight()), 0, 20);
        ofDrawBitmapStringHighlight("Display size: " + ofToString(ofGetWidth()) + "x" + ofToString(ofGetHeight()), 0, 40);
        ofDrawBitmapStringHighlight("Source size: " + ofToString(width) + "x" + ofToString(height), 0, 60);
        ofDrawBitmapStringHighlight("Max texture size: " + ofToString(getMaxTextureSize()), 0, 80);
        ofDrawBitmapStringHighlight("Side: " + ofToString(side), 0, 100);
        ofDrawBitmapStringHighlight(ofToString((int) (ofGetFrameRate() + .5)) + "fps", 0, 120);
        string timeSinceReceived = ofToString((ofGetElapsedTimeMillis() - lastReceived) / 1000., 2);
        string timeSinceTransitionStart = ofToString((ofGetElapsedTimeMillis() - lastTransitionStart) / 1000., 2);
        string timeSinceTransitionStop = ofToString((ofGetElapsedTimeMillis() - lastTransitionStop) / 1000., 2);
        ofDrawBitmapStringHighlight("Last received: " + timeSinceReceived + "s", 0, 140);
        ofDrawBitmapStringHighlight("Last transition start: " + timeSinceTransitionStart + "s", 0, 160);
        ofDrawBitmapStringHighlight("Last transition stop: " + timeSinceTransitionStop + "s", 0, 180);
        ofDrawBitmapStringHighlight("Transition: " + ofToString(int(transition * 100)) + "%", 0, 200);
        string ready = "Transition in process: ";
        ready += transitionInProcess ? "yes" : "no";
        ofDrawBitmapStringHighlight(ready, 0, 220);
        ofPopMatrix();
    }
    void draw() {
        int n = sourceTiles.size();
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        for(int i = 0; i < n; i++) {
            Tile& begin = beginTiles[i];
            Tile& end = endTiles[i];
            float t = ofMap(transition, transitionBegin[i], transitionEnd[i], 0, 1, true);
            ofVec2f lerp = transitionManhattan ?
                manhattanLerp(begin, end, smoothstep(t)) :
                euclideanLerp(begin, end, smoothstep(t));
            Tile& s = sourceTiles[i];
            addSubsection(mesh, source.getTexture(), lerp.x, lerp.y, side, side, s.x, s.y);
        }
        source.bind();
        mesh.drawFaces();
        source.unbind();
        
        if(debugMode) {
            drawDebug();
        }
    }
};

int main() {
    ofGLFWWindowSettings settings;
    settings.multiMonitorFullScreen = true;
    ofCreateWindow(settings)->setFullscreen(true);
    ofRunApp(new ofApp());
}
