#include "PhotoMosaic.h"
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

class ofApp : public ofBaseApp {
public:
    PhotoMosaic photomosaic;
    ofTexture atlasTexture;
    
    float transitionDurationSeconds = 5;
    uint64_t lastTransitionStart = 0;
    float transitionStatus = 1;
    bool transitionInProcess = false;
    
    void setup() {
        ofSetBackgroundAuto(false);
        ofSetVerticalSync(true);
        
        photomosaic.setup(ofGetWidth(), ofGetHeight());
        photomosaic.setRefinementSteps(1000000);
        photomosaic.setFilterScale(0.1);
        photomosaic.setFilterContrast(1.0);
        photomosaic.setIcons(loadImages("db"));
        
        // copy the atlas to a texture for rendering later
        ofPixels atlasPix;
        const cv::Mat& atlasMat = photomosaic.getAtlas();
        atlasPix.setFromExternalPixels(atlasMat.data, atlasMat.cols, atlasMat.rows, OF_PIXELS_RGB);
        atlasTexture.allocate(atlasPix);
    }
    void keyPressed(int key) {
        if(key == ' ') {
            loadPortrait("portraits/img.jpg");
        }
    }
    void loadPortrait(string filename) {
        if(transitionInProcess) return;
        transitionInProcess = true; // transition starts
        photomosaic.setTransitionStyle(ofRandomuf() < 0.5,
                                       ofRandomuf() < 0.5,
                                       ofRandomuf() < 0.5);
        photomosaic.match(loadMat(filename));
        // this is how you build the result without drawing it:
        // saveMat(photomosaic.buildResult(), "output.tiff");
        lastTransitionStart = ofGetElapsedTimeMillis();
    }
    void update() {
        float transitionPrev = transitionStatus;
        transitionStatus = (ofGetElapsedTimeMillis() - lastTransitionStart) / (1000 * transitionDurationSeconds);
        transitionStatus = ofClamp(transitionStatus, 0, 1);
        if(transitionStatus == 1 && transitionPrev < 1) {
            transitionInProcess = false; // transition finishes
        }
    }
    void draw() {
        ofMesh mesh;
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        int side = photomosaic.getSide();
        const std::vector<cv::Point2i>& atlasPositions = photomosaic.getAtlasPositions();
        std::vector<cv::Point2f> screenPositions = photomosaic.getCurrentPositions(transitionStatus);
        int n = screenPositions.size();
        for(int i = 0; i < n; i++) {
            cv::Point2f screen = screenPositions[i];
            cv::Point2i atlas = atlasPositions[i % atlasPositions.size()];
            addSubsection(mesh, atlasTexture, screen.x, screen.y, side, side, atlas.x, atlas.y);
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
