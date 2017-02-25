#pragma once

#include <vector>
#include "opencv2/opencv.hpp"

/// A Tile is the smallest piece of a PhotoMosaic.
class Tile {
public:
    std::vector<cv::Vec3b> grid;
    float weight;
    
    Tile(const std::vector<cv::Vec3b>& grid, float weight)
    :grid(grid)
    ,weight(weight) {
    }
    static std::vector<Tile> buildTiles(const cv::Mat& mat);
    static float getCost(const Tile& a, const Tile& b);
};
