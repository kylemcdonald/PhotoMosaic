#pragma once

#include <vector>
#include "opencv2/opencv.hpp"

/// A Tile is the smallest piece of a PhotoMosaic.
class Tile {
public:
    int side;
    cv::Vec2f position;
    std::vector<cv::Vec3b> grid;
    float weight;
    
    Tile(int x, int y, int side, const std::vector<cv::Vec3b>& grid, float weight)
    :position(x, y)
    ,side(side)
    ,grid(grid)
    ,weight(weight) {
    }
    static std::vector<Tile> buildTiles(const cv::Mat& pix, int side);
    static float getCost(const Tile& a, const Tile& b);
};
