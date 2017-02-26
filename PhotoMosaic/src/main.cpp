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

/// PhotoMosaic is composed of the Matcher and Highpass classes
/// and handles interaction between these classes.
class PhotoMosaic {
private:
    int side = 0, subsampling = 0;
    int width = 0, height = 0;
    int nx = 0, ny = 0;
    
    Matcher matcher;
    Highpass highpass;
    cv::Mat dst;
    
    cv::Mat atlas;
    std::vector<cv::Point2i> atlasPositions;
    vector<Tile> srcTiles;
    vector<cv::Point2i> screenPositions;
    vector<unsigned int> matchedIndices;
    
public:
    void setup(int width, int height, int side=32, int subsampling=3) {
        this->subsampling = subsampling;
        this->side = side;
        nx = width / side;
        ny = height / side;
        this->width = side * nx;
        this->height = side * ny;
        if (width != this->width || height != this->height) {
            std::cout << width << "x" << height << " is not evenly divisible by " << side <<
                ", using " << this->width << "x" << this->height << " instead" << std::endl;
        }
    }
    void setRefinementSteps(int refinementSteps) { matcher.setRefinementSteps(refinementSteps); }
    void setFilterScale(float filterScale) { highpass.setFilterScale(filterScale); }
    void setFilterContrast(float filterContrast) { highpass.setFilterContrast(filterContrast); }
    
    void setIcons(const std::vector<cv::Mat>& icons) {
        atlas = buildAtlas(icons, side, atlasPositions);
        vector<cv::Mat> smaller = batchResize(icons, subsampling);
        subtractMean(smaller);
        unsigned int i = 0;
        for(int y = 0; y < ny; y++) {
            for(int x = 0; x < nx; x++) {
                screenPositions.emplace_back(x*side, y*side);
                unsigned int index = i % smaller.size();
                srcTiles.emplace_back(smaller[index]);
                i++;
            }
        }
    }
    
    const std::vector<unsigned int>& match(const cv::Mat& mat) {
        int w = subsampling * nx;
        int h = subsampling * ny;
        
        cv::Mat crop(getRegionWithRatio(mat, float(width) / height));
        cv::resize(crop, dst, cv::Size(w, h), 0, 0, cv::INTER_AREA);
        
        highpass.filter(dst);
        std::vector<Tile> dstTiles = Tile::buildTiles(dst, subsampling);
        
        matchedIndices = matcher.match(srcTiles, dstTiles);
        
        return matchedIndices;
    }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getSide() const { return side; }
    int getSubsampling() const { return subsampling; }
    
    const cv::Mat& getAtlas() const { return atlas; }
    const std::vector<cv::Point2i>& getAtlasPositions() const { return atlasPositions; }
    const std::vector<cv::Point2i>& getScreenPositions() const { return screenPositions; }
};

class ofApp : public ofBaseApp {
public:
    
    ofTexture atlasTexture; // used for rendering
    
    float transitionSeconds = 5; // this is variable
    
    vector<cv::Point2i> beginPositions, endPositions;
    vector<float> transitionBegin, transitionEnd;
    uint64_t lastTransitionStart = 0;
    float transitionStatus = 1;
    bool transitionInProcess = false;
    bool transitionCircle = false;
    bool transitionManhattan = false;
    
    PhotoMosaic photomosaic;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        
        photomosaic.setup(ofGetWidth(), ofGetHeight());
        photomosaic.setRefinementSteps(1000000);
        photomosaic.setFilterScale(0.1);
        photomosaic.setFilterContrast(1.0);
        photomosaic.setIcons(loadImages("db-trimmed"));
        
        // copy the atlas to a texture for rendering later
        ofPixels atlasPix;
        const cv::Mat& atlasMat = photomosaic.getAtlas();
        atlasPix.setFromExternalPixels(atlasMat.data, atlasMat.cols, atlasMat.rows, OF_PIXELS_RGB);
        atlasTexture.allocate(atlasPix);
        
        beginPositions = photomosaic.getScreenPositions();
        endPositions = photomosaic.getScreenPositions();
        
        int n = beginPositions.size();
        transitionBegin = vector<float>(n, 0);
        transitionEnd = vector<float>(n, 1);
    }
    void keyPressed(int key) {
        if(key == ' ') {
            loadPortrait("portraits/img.jpg");
        }
    }
    void loadPortrait(string filename) {
        if(transitionInProcess) return;
        
        transitionInProcess = true;
        
        vector<unsigned int> matchedIndices = photomosaic.match(loadMat(filename));
        
        int width = photomosaic.getWidth(), height = photomosaic.getHeight();
        const std::vector<cv::Point2i>& screenPositions = photomosaic.getScreenPositions();
        int n = matchedIndices.size();
        
        cv::Point2i center(width / 2, height / 2);
        float diagonal = sqrt(width*width + height*height) / 2;
        bool topDown = ofRandomuf() < .5;
        transitionCircle = ofRandomuf() < .5;
        transitionManhattan = ofRandomuf() < .5;
        for(int i = 0; i < n; i++) {
            cv::Point2i cur = endPositions[i];
            float begin;
            if(transitionCircle) {
                begin = ofMap(cv::norm(cur-center), 0, diagonal, 0, .75);
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
            endPositions[i] = screenPositions[index];
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
    void update() {
        updateTransition();
    }
    void draw() {
        int n = beginPositions.size();
        const std::vector<cv::Point2i>& atlasPositions = photomosaic.getAtlasPositions();
        int side = photomosaic.getSide();
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        for(int i = 0; i < n; i++) {
            cv::Point2i& begin = beginPositions[i], end = endPositions[i];
            float t = ofMap(transitionStatus, transitionBegin[i], transitionEnd[i], 0, 1, true);
            cv::Point2f lerp = transitionManhattan ?
                manhattanLerp(begin, end, smoothstep(t)) :
                euclideanLerp(begin, end, smoothstep(t));
            cv::Point2i s = atlasPositions[i % atlasPositions.size()];
            addSubsection(mesh, atlasTexture, lerp.x, lerp.y, side, side, s.x, s.y);
        }
        atlasTexture.bind();
        mesh.drawFaces();
        atlasTexture.unbind();
    }
};

int main() {
    ofGLFWWindowSettings settings;
    settings.multiMonitorFullScreen = true;
    ofCreateWindow(settings)->setFullscreen(true);
    ofRunApp(new ofApp());
}
