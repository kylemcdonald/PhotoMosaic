#include "ofMain.h"
#include "ofxTiming.h"
#include "Highpass.h"

using namespace glm;

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
    ofClear(255, 255, 255, 255);
    auto filesitr = files.begin();
    for(auto& position : getGrid(width, height, side)) {
        int x = position.first;
        int y = position.second;
        if(img.load(filesitr->path())) {
            drawCenterSquare(img, x, y, side, side);
        }
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
    ofClear(255, 255, 255, 255);
    src.draw(0, 0);
    auto filesitr = files.begin();
    auto positions = getGrid(width, height, side);
    ofRandomize(positions);
    for(auto& position : positions) {
        int x = position.first;
        int y = position.second;
        if(img.load(filesitr->path())) {
            drawCenterSquare(img, x, y, side, side);
        }
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
class Tile : public vec2 {
public:
    int side;
    vector<ofColor> grid;
    float weight;
    Tile(int x, int y, int side, const vector<ofColor>& grid, float weight)
    :vec2(x, y)
    ,side(side)
    ,grid(grid)
    ,weight(weight) {
    }
    static vector<Tile> buildTiles(const ofPixels& pix, int side) {
        ofLog() << "Building tiles for " << pix.getWidth() << "x" << pix.getHeight() << " at side " << side;
        
        // we could do this with resizing but OF doesn't have a good downsampling method
        float subsample = (float) side / subsampling;
        int w = pix.getWidth(), h = pix.getHeight();
        int nx = w / side, ny = h / side;
        vector<Tile> tiles;
        vec2 center = vec2(w, h) / 2;
        for(int y = 0; y < h; y+=side) {
            for(int x = 0; x < w; x+=side) {
                vector<ofColor> grid;
                for(int ky = 0; ky < subsampling; ky++) {
                    for(int kx = 0; kx < subsampling; kx++) {
                        grid.push_back(getAverage(pix, x+kx*subsample, y+ky*subsample, subsample, subsample));
                    }
                }
                float weight = ofMap(distance(vec2(x, y), center), 0, w / 2, 1, 0, true);
                tiles.emplace_back(x, y, side, grid, weight);
            }
        }
        return tiles;
    }
};

// lerps along x then along y, linearly
// a nicer but more complicated implementation would
// lerp the bigger direction first
vec2 manhattanLerp(vec2 begin, vec2 end, float t) {
    float dx = fabs(begin.x - end.x);
    float dy = fabs(begin.y - end.y);
    float dd = dx + dy;
    float dc = dd * t;
    if(dc < dx) { // lerp dx
        float dt = dc / dx;
        return vec2(ofLerp(begin.x, end.x, dt), begin.y);
    } else if(dc < dd) { // lerp dy
        float dt = (dc - dx) / dy;
        return vec2(end.x, ofLerp(begin.y, end.y, dt));
    } else { // when dy or dx+dy is zero
        return vec2(end.x, end.y);
    }
}

vec2 euclideanLerp(vec2 begin, vec2 end, float t) {
    return mix(begin, end, t);
}

void addSubsection(ofMesh& mesh, ofTexture& tex, float x, float y, float w, float h, float sx, float sy) {
    vec2 nwc = tex.getCoordFromPoint(sx, sy);
    vec2 nec = tex.getCoordFromPoint(sx + w, sy);
    vec2 sec = tex.getCoordFromPoint(sx + w, sy + h);
    vec2 swc = tex.getCoordFromPoint(sx, sy + h);
    
    mesh.addTexCoord(nwc);
    mesh.addTexCoord(nec);
    mesh.addTexCoord(swc);
    mesh.addTexCoord(nec);
    mesh.addTexCoord(sec);
    mesh.addTexCoord(swc);
    
    vec3 nwp(x, y, 0);
    vec3 nep(x + w, y, 0);
    vec3 sep(x + w, y + h, 0);
    vec3 swp(x, y + h, 0);
    
    mesh.addVertex(nwp);
    mesh.addVertex(nep);
    mesh.addVertex(swp);
    mesh.addVertex(nep);
    mesh.addVertex(sep);
    mesh.addVertex(swp);
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
    ,processing(false) {
        startThread();
    }
    void close() {
        inputChannel.close();
        outputChannel.close();
        waitForThread(true);
    }
    void match(const vector<Tile>& inputSource, string target) {
        this->inputSource = &inputSource;
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
            vector<Tile> destTiles = buildTiles(filename);
            if(destTiles.size() == 0) {
                ofLog() << "No tiles, skipping matching process.";
            } else {
                const vector<Tile>& sourceTiles = *inputSource;
                if(destTiles.size() != sourceTiles.size()) {
                    ofLogError() << "The number of source tiles and destination tiles do not match.";
                }
                for(int i = 0; i < iterations; i++) {
                    int a = ofRandom(sourceTiles.size()), b = ofRandom(sourceTiles.size());
                    const Tile& lefta = sourceTiles[a];
                    const Tile& leftb = sourceTiles[b];
                    const Tile& righta = destTiles[a];
                    const Tile& rightb = destTiles[b];
                    long sumab = getCost(lefta, righta) + getCost(leftb, rightb);
                    long sumba = getCost(lefta, rightb) + getCost(leftb, righta);
                    if(sumba < sumab) {
                        swap(destTiles[a], destTiles[b]);
                    }
                }
            }
            outputChannel.send(destTiles);
            processing = false;
        }
    }
    vector<Tile> buildTiles(string filename) {
        ofPixels image;
        ofLog() << "Loading image " << filename;
        if(!ofLoadImage(image, filename)) {
            ofLogError() << "Error loading image " << filename;
            return {};
        } else {
            ofLog() << "Image loaded";
        }
        image.setImageType(OF_IMAGE_COLOR);
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
    const vector<Tile>* inputSource;
    ofThreadChannel<string> inputChannel;
    ofThreadChannel<vector<Tile>> outputChannel;
    vector<Tile> outputData;
};

class ofApp : public ofBaseApp {
public:
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
        
        iterations = 100000;
        highpassSize = 250;
        highpassContrast = 20;
        transitionSeconds = 1;
        onscreenSeconds = 10;
        
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
        
        // source = ofImage(addToGrid(source, "upcoming", width, height, side));
        source.save(sourceFile);
        ofPixels& pix = source.getPixels();
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
