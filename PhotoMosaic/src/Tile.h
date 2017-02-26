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
    
    /// Get the sum of all the colors (effectively average brightness).
    unsigned int getColorSum() const;
    
    /// Build a vector of Tiles from a perfectly-sized image.
    static std::vector<Tile> buildTiles(const cv::Mat& mat, int subsampling);
    
    /// Get the euclidean distance between two Tiles.
    static float distance(const Tile& a, const Tile& b);
};
