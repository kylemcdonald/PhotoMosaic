#pragma once

#include <vector>
#include "opencv2/opencv.hpp"

/// A Tile is the moveable piece of a PhotoMosaic.
class Tile {
private:
    cv::Mat_<cv::Vec3i> grid;
    unsigned int colorSum;
    float weight;
    
public:
    
    Tile(const cv::Mat& mat, float weight=0);
    unsigned int getColorSum() const;
    static std::vector<Tile> buildTiles(const cv::Mat& mat);
    static float getCost(const Tile& a, const Tile& b);
};
