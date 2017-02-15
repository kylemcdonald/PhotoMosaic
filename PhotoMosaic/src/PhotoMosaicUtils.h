#pragma once
#include "ofMath.h"

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
