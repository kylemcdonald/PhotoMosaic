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

cv::Mat getMean(const std::vector<cv::Mat>& mats) {
    cv::Mat sum = cv::Mat::zeros(mats[0].rows, mats[0].cols, CV_32FC3);
    cv::Mat matf;
    for(auto& mat : mats) {
        mat.convertTo(matf, CV_32FC3);
        cv::add(matf, sum, sum);
    }
    sum /= mats.size();
    return sum;
}

void subtractMean(std::vector<cv::Mat>& mats) {
    cv::Mat mean = getMean(mats);
    cv::Mat matf;
    for(auto& mat : mats) {
        mat.convertTo(matf, CV_32FC3);
        cv::subtract(matf, mean, matf);
        matf += cv::Scalar(127, 127, 127);
        matf.convertTo(mat, CV_8UC3);
    }
}

cv::Mat buildAtlas(const std::vector<cv::Mat>& images, unsigned int side, std::vector<cv::Point2i>& positions) {
    cv::Mat atlas;
    unsigned int n = images.size();
    int nx = ceilf(sqrtf(n));
    int ny = ceilf(float(n) / nx);
    positions.resize(n);
    atlas.create(ny * side, nx * side, CV_8UC3);
    atlas = cv::Scalar(255, 255, 255);
    cv::Size wh(side, side);
    unsigned int x = 0, y = 0;
    for(unsigned int i = 0; i < n; i++) {
        float xs = x * side, ys = y * side;
        cv::Mat roi(atlas, cv::Rect(xs, ys, side, side));
        cv::resize(images[i], roi, wh, cv::INTER_AREA);
        positions[i].x = xs;
        positions[i].y = ys;
        x++;
        if(x == nx) {
            x = 0;
            y++;
        }
    }
    return atlas;
}

std::vector<cv::Mat> batchResize(const std::vector<cv::Mat>& src, unsigned int side) {
    unsigned int n = src.size();
    cv::Size wh(side, side);
    std::vector<cv::Mat> dst(n);
    for(unsigned int i = 0; i < n; i++) {
        dst[i].create(wh, CV_8UC3);
        cv::resize(src[i], dst[i], wh, 0, 0, cv::INTER_AREA);
    }
    return dst;
}

cv::Mat getRegionWithRatio(const cv::Mat& mat, float aspectRatio) {
    int w = mat.cols, h = mat.rows;
    float currentAspectRatio = float(w) / h;
    if(aspectRatio > currentAspectRatio) { // if aspect ratio is wider than mat
        h = w / aspectRatio; // make mat height smaller
    } else if(aspectRatio < currentAspectRatio) { // if aspect ratio is narrower than mat
        w = h * aspectRatio; // make mat width smaller
    }
    // center the crop
    int x = (mat.cols - w) / 2;
    int y = (mat.rows - h) / 2;
    return mat(cv::Rect(x, y, w, h));
}
