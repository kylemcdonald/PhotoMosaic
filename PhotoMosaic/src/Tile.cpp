#include "Tile.h"

const int subsampling = 3;

/// Get the perceptual difference between two colors.
inline long getCost(const cv::Vec3b& c1, const cv::Vec3b& c2) {
    long rmean = ((long) c1[0] + (long) c2[0]) / 2;
    long r = (long) c1[0] - (long) c2[0];
    long g = (long) c1[1] - (long) c2[1];
    long b = (long) c1[2] - (long) c2[2];
    return ((((512 + rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
}

/// Get the perceptual difference between two tiles.
float Tile::getCost(const Tile& a, const Tile& b) {
    const int n = subsampling * subsampling;
    unsigned long total = 0;
    for(int i = 0; i < n; i++) {
        total += ::getCost(a.grid[i], b.grid[i]);
    }
    return total * (a.weight + b.weight);
}

std::vector<Tile> Tile::buildTiles(const cv::Mat& mat, int side) {
    int w = mat.cols, h = mat.rows;
    std::cout << "Building tiles for " << w << "x" << h << " at side " << side << " subsampling " << subsampling << std::endl;

    std::vector<Tile> tiles;
    // subtract the subsampling amount so we compare distances relative to tile center
    cv::Vec2f center = cv::Vec2f(w-subsampling, h-subsampling) / 2;
    float maxDistance = norm(center); // distance from corner to center
    cv::Mat roi;
    for(int y = 0; y < h; y+=subsampling) {
        for(int x = 0; x < w; x+=subsampling) {
            // copy the current region of interest to a new cv::Mat to make it continuous
            mat(cv::Rect(x, y, subsampling, subsampling)).copyTo(roi);
            float distanceFromCenter = cv::norm(center - cv::Vec2f(x, y)) / maxDistance;
            int sx = (x*side)/subsampling, sy = (y*side)/subsampling;
            tiles.emplace_back(sx, sy, side, roi.reshape(0, 1), 1 - distanceFromCenter);
        }
    }
    return tiles;
}
