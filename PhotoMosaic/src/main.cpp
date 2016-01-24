/*
 overall resolution is 1920x1080x6
 1920x2=3840, 1080x3=3240
 
 going to guess the right resolution is around 60x60 pixels per tile
 or about 3200 images onscreen at any moment
 
 todo:
 - run transition with different timings in different areas
 - make sure grid fills non integer sizes
 - run highpass filter on the lightness channel of input image
 - add falling new images
 - update images while running
 - save screenshots
 
 - debug status info
 - available and actual resolution
 - how many images are in use
 - last time images were updated
 - network connectivity info
 - debug control panel
 - consider dominant rather than average color?
 - make sure cache is running regularly
 */

#include "ofMain.h"
#include "ofxOsc.h"
#include "Highpass.h"
#include "ofxAssignment.h"

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
    return dir.getFiles();
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
    ofFbo buffer;
    
    ofFbo::Settings settings;
    settings.width = width;
    settings.height = height;
    //        settings.numSamples = 1;
    settings.useDepth = false;
    buffer.allocate(settings);
    
    pushArbTex();
    ofDisableArbTex();
    ofImage img;
    img.getTexture().enableMipmap();
    buffer.begin();
    auto files = listImages(dir);
    auto filesitr = files.begin();
    ofImage small;
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

class Tile : public ofVec2f {
public:
    int side;
    float brightness, hue;
    ofColor average;
    vector<ofColor> grid;
    Tile(int x, int y, int side, const vector<ofColor>& grid)
    :ofVec2f(x, y)
    ,side(side)
    ,grid(grid) {
    }
    static vector<Tile> buildTiles(const ofPixels& pix, int side) {
        // we could do this with resizing but OF doesn't have a good downsampling method
        int subsampling = 3;
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
                tiles.emplace_back(x, y, side, grid);
            }
        }
        return tiles;
    }
};

float getCost(const ofColor& c1, const ofColor& c2) {
    long rmean = ((long) c1.r + (long) c2.r) / 2;
    long r = (long) c1.r - (long) c2.r;
    long g = (long) c1.g - (long) c2.g;
    long b = (long) c1.b - (long) c2.b;
    return (((512 + rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8);
}

vector<vector<double>> getCost(const vector<Tile>& a, const vector<Tile>& b) {
    int n = a.size();
    vector<vector<double>> cost(n, vector<double>(n, 0));
    for(int i = 0; i < n; i++) {
        const Tile& at = a[i];
        for(int j = 0; j < n; j++) {
            const Tile& bt = b[j];
            double& dist = cost[i][j];
            int m = at.grid.size();
            for(int k = 0; k < m; k++) {
                dist += getCost(at.grid[k], bt.grid[k]);
            }
        }
    }
    return cost;
}

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

class ofApp : public ofBaseApp {
public:
    int side, width, height;
    int hsbSortSteps;
    float transitionSeconds;
    
    ofxAssignment solver;
    Highpass highpass;
    float highpassSize, highpassContrast;
    
    ofImage source;
    vector<Tile> sourceTiles, beginTiles, endTiles;
    
    ofxOscReceiver osc;
    
    uint64_t transitionBegin;
    float transition = 0;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        
        ofXml xml;
        xml.load("settings.xml");
        side = xml.getIntValue("side");
        width = xml.getIntValue("size/width");
        height = xml.getIntValue("size/height");
        highpassSize = xml.getIntValue("highpass/size");
        highpassContrast = xml.getIntValue("highpass/contrast");
        osc.setup(xml.getIntValue("oscPort"));
        hsbSortSteps = xml.getFloatValue("hsbSortSteps");
        transitionSeconds = xml.getFloatValue("transitionSeconds");
        
        ofLog() << "Max texture size: " << getMaxTextureSize();
        ofLog() << "Side: " << side;
        ofLog() << "Source size: " << width << "x" << height;
        ofLog() << "Screen size: " << ofGetScreenWidth() << "x" << ofGetScreenHeight();
        
        setupSource();
        beginTiles = sourceTiles;
        endTiles = sourceTiles;
    }
    void keyPressed(int key) {
        switch (key) {
            case 'a': loadPortrait("a.jpg"); break;
            case 'b': loadPortrait("b.jpg"); break;
            case 'c': loadPortrait("c.jpg"); break;
            case 'f': ofToggleFullscreen(); break;
        }
    }
    void loadPortrait(string filename) {
        beginTiles = endTiles;
        endTiles = buildTiles(filename);
        
        // rearrange tiles to match
        ofLog() << "Computing match between " << beginTiles.size() << " tiles";
        vector<vector<double>> cost = getCost(beginTiles, endTiles);
        const vector<int>& assignment = solver.solve(cost);
        vector<Tile> after;
        for(int i : assignment) {
            after.push_back(endTiles[i]);
        }
        endTiles = after;
        
        transitionBegin = ofGetElapsedTimeMillis();
    }
    void updateTransition() {
        uint64_t transitionState = ofGetElapsedTimeMillis() - transitionBegin;
        transition = transitionState / (1000 * transitionSeconds);
        transition = MIN(transition, 1);
        transition = MAX(transition, 0);
    }
    vector<Tile> buildTiles(string filename) {
        ofPixels original;
        ofLoadImage(original, filename);
        highpass.filter(original, original, highpassSize, highpassContrast);
        return buildTiles(original);
    }
    vector<Tile> buildTiles(ofPixels& image) {
        ofRectangle originalRect(0, 0, image.getWidth(), image.getHeight());
        ofRectangle targetRect(0, 0, width, height);
        targetRect.scaleTo(originalRect, OF_SCALEMODE_FIT);
        ofPixels cropped;
        image.cropTo(cropped, targetRect.x, targetRect.y, targetRect.width, targetRect.height);
        cropped.resize(width, height);
        vector<Tile> tiles = Tile::buildTiles(cropped, side);
        return tiles;
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
            source.save(sourceFile);
            ofLog() << "Rebuilt source.";
        }
        ofPixels& pix = source.getPixels();
        sourceTiles = Tile::buildTiles(source, side);
    }
    void update() {
        while(osc.hasWaitingMessages()) {
            ofxOscMessage msg;
            osc.getNextMessage(msg);
            string filename = msg.getArgAsString(0);
            ofLog() << filename;
            loadPortrait(filename);
        }
        updateTransition();
    }
    void draw() {
        float t = smoothstep(transition);
        int n = sourceTiles.size();
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        for(int i = 0; i < n; i++) {
            Tile& begin = beginTiles[i];
            Tile& end = endTiles[i];
            ofVec2f lerp = manhattanLerp(begin, end, t);
            Tile& s = sourceTiles[i];
            addSubsection(mesh, source.getTexture(), lerp.x, lerp.y, side, side, s.x, s.y);
        }
        // to get this on multiple screens we can setup multiple viewports
        // or render to a big FBO then push parts to each screen
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
