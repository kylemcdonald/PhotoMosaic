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

    /// Get the euclidean distance between two Tiles.
    float operator-(const Tile& other) const;
    
    /// Default sorting for Tiles is by brightness.
    bool operator<(const Tile& other) const;
    
    /// Build a vector of Tiles from a perfectly-sized image.
    static std::vector<Tile> buildTiles(const cv::Mat& mat, int subsampling);
};
