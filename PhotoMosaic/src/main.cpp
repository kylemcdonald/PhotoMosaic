#include "Matcher.h"
#include "PhotoMosaicUtils.h"
#include "ofMain.h"

using namespace glm;

cv::Vec2f toCv(vec2 v) {
    return cv::Vec2f(v.x, v.y);
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

void drawCenterSquare(const ofImage& img, float x, float y, float side) {
    cv::Rect crop = getCenterSquare(img.getWidth(), img.getHeight());
    img.drawSubsection(x, y, side, side, crop.x, crop.y, crop.width, crop.height);
}

ofPixels buildGrid(string dir, int width, int height, int side) {
    auto files = listImages(dir);
    
    ofFbo buffer;
    
    ofFbo::Settings settings;
    settings.width = width;
    settings.height = height;
    settings.useDepth = false;
    buffer.allocate(settings);
    
    ofImage img;
    buffer.begin();
    ofClear(255, 255, 255, 255);
    auto filesitr = files.begin();
    for(auto& position : buildGrid(width, height, side)) {
        int x = position.first;
        int y = position.second;
        if(img.load(filesitr->path())) {
            drawCenterSquare(img, x, y, side);
        }
        filesitr++;
        if(filesitr == files.end()) {
            filesitr = files.begin();
        }
    }
    buffer.end();
    
    ofPixels out;
    buffer.readToPixels(out);
    out.setImageType(OF_IMAGE_COLOR);
    return out;
}

/// Add a subsection of tex to a mesh as two triangles.
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

class ofApp : public ofBaseApp {
public:
    int side, width, height;
    
    ofImage source;
    vector<Tile> sourceTiles;
    vector<unsigned int> matchedIndices;
    vector<vec2> beginPositions, endPositions;
    
    vector<float> transitionBegin, transitionEnd;
    uint64_t lastReceived = 0;
    uint64_t lastTransitionStart = 0;
    float transitionStatus = 1;
    bool transitionInProcess = false;
    float transitionSeconds;
    bool transitionCircle = false;
    bool transitionManhattan = false;
    
    Matcher matcher;
    Highpass highpass;
    
    unsigned int subsampling;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        
        side = 32;
        subsampling = 3;
        width = ofGetWidth();
        height = ofGetHeight();
        
        // guarantee that width and height are evenly divisible by tile size
        width = side * (width / side);
        height = side * (height / side);
        
        transitionSeconds = 5;
        
        matcher.setStepTotal(1000000);
        highpass.setFilterScale(0.1);
        highpass.setFilterContrast(1.0);
        
        setupSource();
        
        for(const Tile& tile : sourceTiles) {
            beginPositions.emplace_back(tile.position[0], tile.position[1]);
        }
        endPositions = beginPositions;
        
        transitionBegin = vector<float>(sourceTiles.size(), 0);
        transitionEnd = vector<float>(sourceTiles.size(), 1);
    }
    void keyPressed(int key) {
        switch (key) {
            case 'f': ofToggleFullscreen(); break;
            case ' ': loadPortrait("portraits/img.jpg"); break;
        }
    }
    void loadPortrait(string filename) {
        if(transitionInProcess) return;
        
        transitionInProcess = true;
        lastReceived = ofGetElapsedTimeMillis();
        
        ofPixels image;
        ofLog() << "Loading image " << filename;
        if(!ofLoadImage(image, filename)) {
            ofLogError() << "Error loading image " << filename;
            return;
        } else {
            ofLog() << "Image loaded";
        }
        image.setImageType(OF_IMAGE_COLOR);
        
        // get cv::Mat from src and dst
        cv::Mat mat(image.getHeight(), image.getWidth(), CV_8UC3, image.getData(), 0);
        
        ofRectangle originalRect(0, 0, image.getWidth(), image.getHeight());
        ofRectangle targetRect(0, 0, width, height);
        targetRect.scaleTo(originalRect, OF_SCALEMODE_FIT);
        
        cv::Mat resized;
        cv::Mat cropped(mat(cv::Rect(targetRect.x, targetRect.y, targetRect.width, targetRect.height)));
        int w = subsampling * (width / side);
        int h = subsampling * (height / side);
        cv::resize(cropped, resized, cv::Size(w, h), 0, 0, cv::INTER_AREA);
        
        highpass.filter(resized);
        ofLog() << "resized to " << w << "x" << h;
        std::vector<Tile> destTiles = Tile::buildTiles(resized, side);
        
        ofPixels pix;
        pix.setFromExternalPixels(resized.data, resized.cols, resized.rows, OF_PIXELS_RGB);
        ofSaveImage(pix, "debug-filtered.tiff");
        
        matchedIndices = matcher.match(sourceTiles, destTiles);
        
        vec2 center = vec2(width, height) / 2;
        float diagonal = sqrt(width*width + height*height) / 2;
        bool topDown = ofRandomuf() < .5;
        transitionCircle = ofRandomuf() < .5;
        transitionManhattan = ofRandomuf() < .5;
        for(int i = 0; i < sourceTiles.size(); i++) {
            vec2 cur = endPositions[i];
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
            beginPositions[i] = endPositions[i];
            unsigned int index = matchedIndices[i];
            endPositions[i] = vec2(sourceTiles[index].position[0], sourceTiles[index].position[1]);
        }
        
        lastTransitionStart = ofGetElapsedTimeMillis();
    }
    void updateTransition() {
        float transitionPrev = transitionStatus;
        transitionStatus = (ofGetElapsedTimeMillis() - lastTransitionStart) / (1000 * transitionSeconds);
        transitionStatus = ofClamp(transitionStatus, 0, 1);
        if(transitionStatus == 1 && transitionPrev < 1) {
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
            source = ofImage(buildGrid("db-trimmed", width, height, side));
            ofLog() << "Rebuilt source.";
        }
        
        source.save(sourceFile);
        cv::Mat input(source.getHeight(), source.getWidth(), CV_8UC3, source.getPixels().getData(), 0);
        int sw = input.cols, sh = input.rows;
        float subsample = (float) side / subsampling;
        int w = (sw * subsampling) / side;
        int h = (sh * subsampling) / side;
        
        cv::Mat mat;
        std::cout << "converting to " << w << " x " << h << std::endl;
        cv::resize(input, mat, cv::Size(w, h), 0, 0, cv::INTER_AREA);
        
        ofPixels pix;
        pix.setFromExternalPixels(mat.data, mat.cols, mat.rows, OF_PIXELS_RGB);
        ofSaveImage(pix, "debug-source.tiff");
        
        sourceTiles = Tile::buildTiles(mat, side);
    }
    void update() {
        updateTransition();
    }
    void draw() {
        int n = sourceTiles.size();
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        for(int i = 0; i < n; i++) {
            vec2& begin = beginPositions[i], end = endPositions[i];
            float t = ofMap(transitionStatus, transitionBegin[i], transitionEnd[i], 0, 1, true);
            cv::Vec2f lerp = transitionManhattan ?
                manhattanLerp(toCv(begin), toCv(end), smoothstep(t)) :
                euclideanLerp(toCv(begin), toCv(end), smoothstep(t));
            vec2 s(sourceTiles[i].position[0], sourceTiles[i].position[1]);
            addSubsection(mesh, source.getTexture(), lerp[0], lerp[1], side, side, s.x, s.y);
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
