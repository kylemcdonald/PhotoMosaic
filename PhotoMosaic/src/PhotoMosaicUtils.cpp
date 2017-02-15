#include "PhotoMosaicUtils.h"

using namespace std;
using namespace glm;

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

// Get a square centered on the middle of the img.
ofRectangle getCenterSquare(const ofImage& img) {
    int width = img.getWidth(), height = img.getHeight();
    int side = MIN(width, height);
    ofRectangle crop;
    crop.setFromCenter(width / 2, height / 2, side, side);
    return crop;
}

void drawCenterSquare(const ofImage& img, float x, float y, float side) {
    ofRectangle crop = getCenterSquare(img);
    img.drawSubsection(x, y, side, side, crop.x, crop.y, crop.width, crop.height);
}

vector<pair<int, int>> buildGrid(int width, int height, int side) {
    int m = width / side;
    int n = height / side;
    vector<pair<int, int>> grid(m*n);
    auto itr = grid.begin();
    for(int y = 0; y < n; y++) {
        for(int x = 0; x < m; x++) {
            itr->first = x * side;
            itr->second = y * side;
            itr++;
        }
    }
    return grid;
}

#include "ofFbo.h"
#include "ofGraphics.h"
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
    return out;
}

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
