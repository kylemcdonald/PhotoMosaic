#include "PhotoMosaicUtils.h"

using namespace std;

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
