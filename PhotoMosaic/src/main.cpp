#include "Matcher.h"
#include "Utils.h"
#include "ofMain.h"

void saveMat(const cv::Mat& mat, string filename) {
    ofPixels pix;
    pix.setFromExternalPixels(mat.data, mat.cols, mat.rows, OF_PIXELS_RGB);
    ofSaveImage(pix, filename);
}

cv::Point2f toCv(glm::vec2 v) {
    return cv::Point2f(v.x, v.y);
}

vector<cv::Mat> loadImages(string directory) {
    vector<cv::Mat> mats;
    ofDirectory dir(directory);
    dir.allowExt("png");
    for(auto file : dir.getFiles()) {
        ofPixels pix;
        ofLoadImage(pix, file.path());
        pix.setImageType(OF_IMAGE_COLOR);
        cv::Mat mat(pix.getHeight(), pix.getWidth(), CV_8UC3, pix.getData(), 0);
        mats.push_back(mat.clone());
    }
    return mats;
}

cv::Mat buildAtlas(const vector<cv::Mat>& images, unsigned int side, std::vector<cv::Point2i>& positions) {
    cv::Mat atlas;
    unsigned int n = images.size();
    int nx = ceilf(sqrtf(n));
    int ny = ceilf(float(n) / nx);
    positions.resize(n);
    atlas.create(ny * side, nx * side, CV_8UC3); // allocate atlas
    atlas = cv::Scalar(255, 255, 255); // set to white
    cv::Size wh(side, side);
    unsigned int x = 0, y = 0;
    for(unsigned int i = 0; i < n; i++) {
        float xs = x * side, ys = y * side;
        cv::Mat roi(atlas, cv::Rect(xs, ys, side, side));
        cv::resize(images[i], roi, wh, cv::INTER_AREA);
        positions[i].x = xs;
        positions[i].y = ys;
        x++;
        if(x == nx) {
            x = 0;
            y++;
        }
    }
    return atlas;
}

vector<cv::Mat> batchResize(const vector<cv::Mat>& src, unsigned int side) {
    unsigned int n = src.size();
    cv::Size wh(side, side);
    vector<cv::Mat> dst(n);
    for(unsigned int i = 0; i < n; i++) {
        dst[i].create(wh, CV_8UC3);
        cv::resize(src[i], dst[i], wh, 0, 0, cv::INTER_AREA);
    }
    return dst;
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
    glm::vec2 nwc = tex.getCoordFromPoint(sx, sy);
    glm::vec2 nec = tex.getCoordFromPoint(sx + w, sy);
    glm::vec2 sec = tex.getCoordFromPoint(sx + w, sy + h);
    glm::vec2 swc = tex.getCoordFromPoint(sx, sy + h);
    
    mesh.addTexCoord(nwc);
    mesh.addTexCoord(nec);
    mesh.addTexCoord(swc);
    mesh.addTexCoord(nec);
    mesh.addTexCoord(sec);
    mesh.addTexCoord(swc);
    
    glm::vec3 nwp(x, y, 0);
    glm::vec3 nep(x + w, y, 0);
    glm::vec3 sep(x + w, y + h, 0);
    glm::vec3 swp(x, y + h, 0);
    
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
    
    ofTexture atlas;
    vector<Tile> sourceTiles; // length is number of total tiles
    vector<glm::vec2> initialPositions; // length is number of total tiles
    vector<cv::Point2i> atlasPositions; // length is number of unique icons
    
    vector<unsigned int> matchedIndices;
    vector<glm::vec2> beginPositions, endPositions;
    
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
        
        matcher.setRefinementSteps(100000);
        highpass.setFilterScale(0.1);
        highpass.setFilterContrast(1.0);
        
        setupAtlas();
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
        ofLog() << "Resized to " << w << "x" << h;
        std::vector<Tile> destTiles = Tile::buildTiles(resized);
        
//        ofPixels pix;
//        pix.setFromExternalPixels(resized.data, resized.cols, resized.rows, OF_PIXELS_RGB);
//        ofSaveImage(pix, "debug-filtered.tiff");
        
        ofLog() << "Running matcher.";
        matchedIndices = matcher.match(sourceTiles, destTiles);
        
        ofLog() << "Computing transitions.";
        glm::vec2 center = glm::vec2(width, height) / 2;
        float diagonal = sqrt(width*width + height*height) / 2;
        bool topDown = ofRandomuf() < .5;
        transitionCircle = ofRandomuf() < .5;
        transitionManhattan = ofRandomuf() < .5;
        for(int i = 0; i < sourceTiles.size(); i++) {
            glm::vec2 cur = endPositions[i];
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
            endPositions[i] = initialPositions[index];
        }
        
        lastTransitionStart = ofGetElapsedTimeMillis();
        
        ofLog() << "Starting transition.";
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
    void setupAtlas() {
        ofLog() << "Loading images.";
        std::vector<cv::Mat> images = loadImages("db-trimmed");
        
        ofLog() << "Building atlas from images.";
        cv::Mat atlasMat = buildAtlas(images, side, atlasPositions);
        
        ofPixels atlasPix;
        atlasPix.setFromExternalPixels(atlasMat.data, atlasMat.cols, atlasMat.rows, OF_PIXELS_RGB);
        atlas.allocate(atlasPix);
        
        ofLog() << "Resizing images into subsampled tiles.";
        vector<cv::Mat> smaller = batchResize(images, subsampling);
        
        unsigned int nx = width / side;
        unsigned int ny = height / side;
        unsigned int n = nx * ny;
        unsigned int i = 0;
        for(int y = 0; y < ny; y++) {
            for(int x = 0; x < nx; x++) {
                initialPositions.emplace_back(x*side, y*side);
                unsigned int index = i % smaller.size();
                sourceTiles.emplace_back(smaller[index], 1);
                i++;
            }
        }
        beginPositions = initialPositions;
        endPositions = initialPositions;
        
        transitionBegin = vector<float>(sourceTiles.size(), 0);
        transitionEnd = vector<float>(sourceTiles.size(), 1);
    }
    void update() {
        updateTransition();
    }
    void draw() {
        int n = sourceTiles.size();
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        for(int i = 0; i < n; i++) {
            glm::vec2& begin = beginPositions[i], end = endPositions[i];
            float t = ofMap(transitionStatus, transitionBegin[i], transitionEnd[i], 0, 1, true);
            cv::Point2f lerp = transitionManhattan ?
                manhattanLerp(toCv(begin), toCv(end), smoothstep(t)) :
                euclideanLerp(toCv(begin), toCv(end), smoothstep(t));
            cv::Point2i s = atlasPositions[i % atlasPositions.size()];
            addSubsection(mesh, atlas, lerp.x, lerp.y, side, side, s.x, s.y);
        }
        atlas.bind();
        mesh.drawFaces();
        atlas.unbind();
    }
};

int main() {
    ofGLFWWindowSettings settings;
    settings.multiMonitorFullScreen = true;
    ofCreateWindow(settings)->setFullscreen(true);
    ofRunApp(new ofApp());
}
