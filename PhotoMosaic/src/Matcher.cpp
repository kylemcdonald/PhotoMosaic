#include "Matcher.h"

#include "ofImage.h"

void Matcher::close() {
    inputChannel.close();
    outputChannel.close();
    waitForThread(true);
}

void Matcher::match(const vector<Tile>& inputSource, string target) {
    this->inputSource = &inputSource;
    inputChannel.send(target);
}

bool Matcher::update() {
    ready = false;
    while(outputChannel.tryReceive(outputData)){
        ready = true;
    }
    return ready;
}

bool Matcher::getProcessing() const {
    return processing;
}
const vector<Tile>& Matcher::getOutput() {
    return outputData;
}

void Matcher::threadedFunction() {
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
                long sumab = Tile::getCost(lefta, righta) + Tile::getCost(leftb, rightb);
                long sumba = Tile::getCost(lefta, rightb) + Tile::getCost(leftb, righta);
                if(sumba < sumab) {
                    swap(destTiles[a], destTiles[b]);
                }
            }
        }
        outputChannel.send(destTiles);
        processing = false;
    }
}

// what should really happen: Matcher should be the only class calling Tile::buildTiles
// Matcher should handle any necessary cropping and resizing of the image
// then Tile::buildTiles should only take a subsampling parameter
vector<Tile> Matcher::buildTiles(string filename) {
    ofPixels image;
    ofLog() << "Loading image " << filename;
    if(!ofLoadImage(image, filename)) {
        ofLogError() << "Error loading image " << filename;
        return {};
    } else {
        ofLog() << "Image loaded";
    }
    image.setImageType(OF_IMAGE_COLOR);
    
    // get cv::Mat from src and dst
    cv::Mat mat(image.getHeight(), image.getWidth(), CV_8UC3, image.getData(), 0);
    highpass.filter(mat, highpassSize, highpassContrast);
    
    ofRectangle originalRect(0, 0, image.getWidth(), image.getHeight());
    ofRectangle targetRect(0, 0, width, height);
    targetRect.scaleTo(originalRect, OF_SCALEMODE_FIT);
    ofPixels cropped;
    image.cropTo(cropped, targetRect.x, targetRect.y, targetRect.width, targetRect.height);
    cropped.resize(width, height, OF_INTERPOLATE_BICUBIC);
    
    cv::Mat croppedMat(cropped.getHeight(), cropped.getWidth(), CV_8UC3, cropped.getData(), 0);
    vector<Tile> tiles = Tile::buildTiles(croppedMat, side);
    return tiles;
}
