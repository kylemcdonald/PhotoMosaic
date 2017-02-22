#include "Tile.h"

const int subsampling = 3;

/// Get the perceptual difference between two colors.
long getCost(const cv::Vec3b& c1, const cv::Vec3b& c2) {
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

/// Get the average color from a region of a matrix.
cv::Vec3b getAverage(const cv::Mat& mat, int x, int y, int w, int h) {
    cv::Scalar avg = cv::mean(mat(cv::Rect(x, y, w, h)));
    return cv::Vec3b(avg[0], avg[1], avg[2]);
}

std::vector<Tile> Tile::buildTiles(const cv::Mat& mat, int side) {
    int w = mat.cols, h = mat.rows;
    float subsample = (float) side / subsampling;
    std::cout << "Building tiles for " << w << "x" << h << " at side " << side << " subsample " << subsample << std::endl;
    
    std::vector<Tile> tiles;
    cv::Vec2f center = cv::Vec2f(w, h) / 2;
    float maxDistance = norm(center);
    for(int y = 0; y < h; y+=side) {
        for(int x = 0; x < w; x+=side) {
            std::vector<cv::Vec3b> grid;
            for(int ky = 0; ky < subsampling; ky++) {
                for(int kx = 0; kx < subsampling; kx++) {
                    grid.push_back(getAverage(mat, x+kx*subsample, y+ky*subsample, subsample, subsample));
                }
            }
            float distanceFromCenter = cv::norm(center - cv::Vec2f(x, y)) / maxDistance;
            tiles.emplace_back(x, y, side, grid, 1 - distanceFromCenter);
        }
    }
    return tiles;
}
