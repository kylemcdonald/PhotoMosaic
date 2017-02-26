#include "Utils.h"

float smoothstep(float x) {
    return x*x*(3 - 2*x);
}

cv::Rect getCenterSquare(int width, int height) {
    int side = std::min(width, height);
    return cv::Rect((width - side) / 2, (height - side) / 2,
                     side, side);
}

std::vector<std::pair<int, int>> buildGrid(int width, int height, int side) {
    int m = width / side;
    int n = height / side;
    std::vector<std::pair<int, int>> grid(m*n);
    auto itr = grid.begin();
    for(int y = 0; y < n; y++) {
        for(int x = 0; x < m; x++) {
            itr->first = x * side;
            itr->second = y * side;
            itr++;
        }
    }
    return grid;
}

float lerp(float start, float stop, float amt) {
    return start + (stop-start) * amt;
}

cv::Point2f manhattanLerp(cv::Point2f begin, cv::Point2f end, float t) {
    float dx = fabs(begin.x - end.x);
    float dy = fabs(begin.y - end.y);
    float dd = dx + dy;
    float dc = dd * t;
    if(dc < dx) { // lerp dx
        float dt = dc / dx;
        return cv::Point2f(lerp(begin.x, end.x, dt), begin.y);
    } else if(dc < dd) { // lerp dy
        float dt = (dc - dx) / dy;
        return cv::Point2f(end.x, lerp(begin.y, end.y, dt));
    } else { // when dy or dx+dy is zero
        return cv::Point2f(end.x, end.y);
    }
}

cv::Point2f euclideanLerp(cv::Point2f begin, cv::Point2f end, float t) {
    return cv::Point2f(lerp(begin.x,end.x,t), lerp(begin.y,end.y,t));
}
