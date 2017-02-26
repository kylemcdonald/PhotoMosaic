#include "Matcher.h"
#include "Utils.h"
#include "ofMain.h"

/// Load an RGB image from disk.
cv::Mat loadMat(std::string filename) {
    ofPixels pix;
    ofLoadImage(pix, filename);
    pix.setImageType(OF_IMAGE_COLOR); // guarantee it's RGB
    cv::Mat mat(pix.getHeight(), pix.getWidth(), CV_8UC3, pix.getData(), 0);
    return mat.clone();
}

/// Save an RGB image to disk.
void saveMat(const cv::Mat& mat, std::string filename) {
    ofPixels pix;
    pix.setFromExternalPixels(mat.data, mat.cols, mat.rows, OF_PIXELS_RGB);
    ofSaveImage(pix, filename);
}

/// Convert from glm::vec2 to cv::Point2f
cv::Point2f toCv(glm::vec2 v) {
    return cv::Point2f(v.x, v.y);
}

/// Load all the .png images in a directory and return them as std::vector<cv::Mat>
std::vector<cv::Mat> loadImages(std::string directory) {
    std::vector<cv::Mat> mats;
    ofDirectory dir(directory);
    dir.allowExt("png");
    for(auto file : dir.getFiles()) {
        mats.push_back(loadMat(file.path()));
    }
    return mats;
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
    int side = 32;
    int subsampling = 3;
    
    int width, height;
    
    ofTexture atlas; // used for rendering
    vector<Tile> sourceTiles; // length is number of total tiles
    vector<glm::vec2> initialPositions; // length is number of total tiles
    vector<cv::Point2i> atlasPositions; // length is number of unique icons
    
    vector<unsigned int> matchedIndices;
    vector<glm::vec2> beginPositions, endPositions;
    vector<float> transitionBegin, transitionEnd;
    uint64_t lastTransitionStart = 0;
    float transitionStatus = 1;
    bool transitionInProcess = false;
    float transitionSeconds;
    bool transitionCircle = false;
    bool transitionManhattan = false;
    
    Matcher matcher;
    Highpass highpass;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        
        width = ofGetWidth();
        height = ofGetHeight();
        
        // guarantee that width and height are evenly divisible by tile size
        width = side * (width / side);
        height = side * (height / side);
        
        transitionSeconds = 5;
        
        matcher.setRefinementSteps(1000000);
        highpass.setFilterScale(0.1);
        highpass.setFilterContrast(1.0);
        
        setupAtlas();
    }
    void keyPressed(int key) {
        if(key == ' ') {
            loadPortrait("portraits/img.jpg");
        }
    }
    void loadPortrait(string filename) {
        if(transitionInProcess) return;
        
        transitionInProcess = true;
        
        ofPixels image;
        std::cout << "Loading image " << filename << std::endl;
        if(!ofLoadImage(image, filename)) {
            std::cerr << "Error loading image " << filename << std::endl;
            return;
        } else {
            std::cout << "Image loaded" << std::endl;
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
        std::cout << "Resized to " << w << "x" << h << std::endl;
        std::vector<Tile> destTiles = Tile::buildTiles(resized, subsampling);
        
        std::cout << "Running matcher." << std::endl;
        matchedIndices = matcher.match(sourceTiles, destTiles);
        
        std::cout << "Computing transitions." << std::endl;
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
        
        std::cout << "Starting transition." << std::endl;
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
        std::cout << "Loading images." << std::endl;
        std::vector<cv::Mat> images = loadImages("db-trimmed");
        
        std::cout << "Building atlas from images." << std::endl;
        cv::Mat atlasMat = buildAtlas(images, side, atlasPositions);
        
        ofPixels atlasPix;
        atlasPix.setFromExternalPixels(atlasMat.data, atlasMat.cols, atlasMat.rows, OF_PIXELS_RGB);
        atlas.allocate(atlasPix);
        
        std::cout << "Resizing images into subsampled tiles." << std::endl;
        vector<cv::Mat> smaller = batchResize(images, subsampling);
        subtractMean(smaller);
        
        unsigned int nx = width / side;
        unsigned int ny = height / side;
        unsigned int n = nx * ny;
        unsigned int i = 0;
        for(int y = 0; y < ny; y++) {
            for(int x = 0; x < nx; x++) {
                initialPositions.emplace_back(x*side, y*side);
                unsigned int index = i % smaller.size();
                sourceTiles.emplace_back(smaller[index]);
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
