#include "Tile.h"

Tile::Tile(const cv::Mat& mat, float weight) : weight(weight) {
    grid = mat.reshape(0, 1);
    cv::Scalar sumScalar = cv::sum(grid);
    colorSum = sumScalar[0] + sumScalar[1] + sumScalar[2];
}

cv::Mat_<cv::Vec3i> diff;
cv::Scalar total;
float Tile::distance(const Tile& a, const Tile& b) {
    cv::subtract(a.grid, b.grid, diff);
    cv::multiply(diff, diff, diff);
    total = cv::sum(diff);
    return (total[0] + total[1] + total[2]) * (a.weight + b.weight);
}

unsigned int Tile::getColorSum() const {
    return colorSum;
}

std::vector<Tile> Tile::buildTiles(const cv::Mat& mat, int subsampling) {
    std::vector<Tile> tiles;
    int w = mat.cols, h = mat.rows;
    cv::Vec2f center = cv::Vec2f(w-subsampling, h-subsampling) / 2;
    float maxDistance = norm(center);
    cv::Mat roi;
    for(int y = 0; y < h; y+=subsampling) {
        for(int x = 0; x < w; x+=subsampling) {
            // need to copy the current roi to a new cv::Mat to make it continuous
            mat(cv::Rect(x, y, subsampling, subsampling)).copyTo(roi);
            float distanceFromCenter = cv::norm(center - cv::Vec2f(x, y)) / maxDistance;
            tiles.emplace_back(roi, 1 - distanceFromCenter);
        }
    }
    return tiles;
}
