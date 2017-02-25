#include "PhotoMosaicUtils.h"

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

cv::Vec2f manhattanLerp(cv::Vec2f begin, cv::Vec2f end, float t) {
    float& bx = begin[0], by = begin[1];
    float& ex = end[0], ey = end[1];
    float dx = fabs(bx - ex);
    float dy = fabs(by - ey);
    float dd = dx + dy;
    float dc = dd * t;
    if(dc < dx) { // lerp dx
        float dt = dc / dx;
        return cv::Vec2f(lerp(bx, ex, dt), by);
    } else if(dc < dd) { // lerp dy
        float dt = (dc - dx) / dy;
        return cv::Vec2f(ex, lerp(by, ey, dt));
    } else { // when dy or dx+dy is zero
        return cv::Vec2f(ex, ey);
    }
}

cv::Vec2f euclideanLerp(cv::Vec2f begin, cv::Vec2f end, float t) {
    float& bx = begin[0], by = begin[1];
    float& ex = end[0], ey = end[1];
    return cv::Vec2f(lerp(bx,ex,t), lerp(by,ey,t));
}
