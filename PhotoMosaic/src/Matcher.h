#pragma once

#include "ofThread.h"
#include "ofThreadChannel.h"

#include "Highpass.h"
#include "Tile.h"

/// A Matcher finds a good solution for reorganizing Tiles.
class Matcher : public ofThread {
public:
    Matcher()
    :ready(false)
    ,processing(false) {
        startThread();
    }
    
    int iterations, side, width, height;
    float highpassSize, highpassContrast;
    
    void close();
    void match(const vector<Tile>& inputSource, string target);
    bool update();
    bool getProcessing() const;
    const vector<Tile>& getOutput();
    
private:
    bool processing;
    bool ready;
    
    void threadedFunction();
    vector<Tile> buildTiles(string filename);
    
    Highpass highpass;
    const vector<Tile>* inputSource;
    ofThreadChannel<string> inputChannel;
    ofThreadChannel<vector<Tile>> outputChannel;
    vector<Tile> outputData;
};
