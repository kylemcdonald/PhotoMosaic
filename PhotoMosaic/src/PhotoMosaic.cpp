#include "PhotoMosaic.h"

/// Classic smoothstep function x^2 * (3 - 2*x)
float smoothstep(float x) {
    return x*x*(3 - 2*x);
}

/// Return a full-image region of interest from the mat with a given aspect ratio.
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

/// Pack a set of square cv::Mat images into a texture atlas.
/// Each image is resized to be size (side x side).
/// The positions of the tiles are returned by the positions argument.
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
        const cv::Mat& cur = images[i];
        if(cur.rows != cur.cols) {
            std::cerr << "image " << i << " is not square, stretching to fit" << std::endl;
        }
        if(cur.channels() != 3) {
            throw std::invalid_argument("image is not 3 channels");
        }
        float xs = x * side, ys = y * side;
        cv::Mat roi(atlas, cv::Rect(xs, ys, side, side));
        cv::resize(cur, roi, wh, cv::INTER_AREA);
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

/// Subtract the mean of a set of cv::Mats from each of those cv::Mats.
/// "0" is treated as "127", acting similarly to a highpass filter.
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

/// Batch resize a collection of cv::Mat images to be size (side x side).
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

float lerp(float start, float stop, float amt) {
    return start + (stop-start) * amt;
}

/// Interpolates along x then along y, linearly.
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

/// Interpolates along a line from begin to end.
cv::Point2f euclideanLerp(cv::Point2f begin, cv::Point2f end, float t) {
    return cv::Point2f(lerp(begin.x,end.x,t), lerp(begin.y,end.y,t));
}

/// Clip a float to a range.
float clip(float t, float lower, float upper) {
    return std::max(lower, std::min(t, upper));
}

void PhotoMosaic::setup(int width, int height, int side, int subsampling) {
    if(subsampling < 1 || subsampling > 5) {
        throw std::out_of_range("subsampling is out of range");
    }
    if(width == 0 || height == 0) {
        throw std::out_of_range("width or height are zero");
    }
    this->subsampling = subsampling;
    this->side = side;
    nx = width / side;
    ny = height / side;
    n = nx * ny;
    this->width = side * nx;
    this->height = side * ny;
    if (width != this->width || height != this->height) {
        std::cout << width << "x" << height << " is not evenly divisible by " << side <<
        ", using " << this->width << "x" << this->height << " instead" << std::endl;
    }
}

void PhotoMosaic::setRefinementSteps(int refinementSteps) {
    if(refinementSteps < 0 || refinementSteps > 10000000) {
        throw std::out_of_range("refinementSteps is out of range");
    }
    matcher.setRefinementSteps(refinementSteps);
}

void PhotoMosaic::setMaximumDuration(float maximumDurationSeconds) {
    if(maximumDurationSeconds < 0 || maximumDurationSeconds > 10) {
        throw std::out_of_range("maximumDurationSeconds is out of range");
    }
    matcher.setMaximumDuration(maximumDurationSeconds);
}

void PhotoMosaic::setFilterScale(float filterScale) {
    if(filterScale < 0 || filterScale > 1) {
        throw std::out_of_range("filterScale is out of range");
    }
    highpass.setFilterScale(filterScale);
}

void PhotoMosaic::setFilterContrast(float filterContrast) {
    if(filterContrast < 0.01 || filterContrast > 100) {
        throw std::out_of_range("filterContrast is out of range");
    }
    highpass.setFilterContrast(filterContrast);
}

void PhotoMosaic::setTransitionStyle(bool topDown, bool circle, bool manhattan) {
    transitionTopDown = topDown;
    transitionCircle = circle;
    transitionManhattan = manhattan;
}

void PhotoMosaic::setIcons(const std::vector<cv::Mat>& icons) {
    if(icons.empty()) {
        throw std::invalid_argument("no icons");
    }
    atlas = buildAtlas(icons, side, atlasPositions);
    std::vector<cv::Mat> smaller = batchResize(icons, subsampling);
    subtractMean(smaller);
    unsigned int i = 0;
    for(int y = 0; y < ny; y++) {
        for(int x = 0; x < nx; x++) {
            screenPositions.emplace_back(x*side, y*side);
            unsigned int index = i % smaller.size();
            srcTiles.emplace_back(smaller[index]);
            i++;
        }
    }
    
    beginPositions = screenPositions;
    endPositions = screenPositions;
    transitionBegin.assign(n, 0);
    transitionEnd.assign(n, 1);
}

void PhotoMosaic::match(const cv::Mat& mat) {
    if(mat.empty()) {
        throw std::invalid_argument("mat is empty");
    }
    if(mat.channels() != 3) {
        throw std::invalid_argument("mat is not 3 channels");
    }
    
    int w = subsampling * nx;
    int h = subsampling * ny;
    
    cv::Mat crop(getRegionWithRatio(mat, float(width) / height));
    cv::resize(crop, dst, cv::Size(w, h), 0, 0, cv::INTER_AREA);
    
    highpass.filter(dst);
    std::vector<Tile> dstTiles = Tile::buildTiles(dst, subsampling);
    
    matchedIndices = matcher.match(srcTiles, dstTiles);
    
    // setup the transition timings
    cv::Point2i center(width / 2, height / 2);
    float diagonal = sqrt(width*width + height*height) / 2;
    // this variable could be made accessible, it should be a number betwee 0-0.5
    float transitionPaddingRatio = 0.25;
    for(int i = 0; i < n; i++) {
        cv::Point2i cur = endPositions[i];
        float begin;
        if(transitionCircle) {
            begin = cv::norm(cur - center) / diagonal;
        } else {
            begin = float(cur.y) / height;
        }
        if(transitionTopDown) {
            begin = 1 - begin;
        }
        begin *= 1 - transitionPaddingRatio;
        float end = begin + transitionPaddingRatio;
        transitionBegin[i] = clip(begin, 0, 1);
        transitionEnd[i] = clip(end, 0, 1);
        beginPositions[i] = endPositions[i];
        unsigned int index = matchedIndices[i];
        endPositions[i] = screenPositions[index];
    }
}

cv::Mat PhotoMosaic::buildResult() const {
    cv::Mat screen(height, width, CV_8UC3);
    for(int i = 0; i < n; i++) {
        const cv::Point2i& screenPosition = endPositions[i];
        const cv::Point2i& atlasPosition = atlasPositions[i % atlasPositions.size()];
        cv::Mat atlasRoi(atlas(cv::Rect(atlasPosition.x, atlasPosition.y, side, side)));
        cv::Mat screenRoi(screen(cv::Rect(screenPosition.x, screenPosition.y, side, side)));
        atlasRoi.copyTo(screenRoi);
    }
    return screen;
}

int PhotoMosaic::getWidth() const { return width; }
int PhotoMosaic::getHeight() const { return height; }
int PhotoMosaic::getSide() const { return side; }
int PhotoMosaic::getSubsampling() const { return subsampling; }

const cv::Mat& PhotoMosaic::getAtlas() const { return atlas; }
const std::vector<cv::Point2i>& PhotoMosaic::getAtlasPositions() const { return atlasPositions; }
const std::vector<cv::Point2i>& PhotoMosaic::getScreenPositions() const { return screenPositions; }

std::vector<cv::Point2f> PhotoMosaic::getCurrentPositions(float t) const {
    t = clip(t, 0, 1);
    std::vector<cv::Point2f> currentPositions(n);
    for(int i = 0; i < n; i++) {
        const cv::Point2i& begin = beginPositions[i], end = endPositions[i];
        float tb = transitionBegin[i], te = transitionEnd[i];
        float curt = (t - tb) / (te - tb);
        curt = clip(curt, 0, 1);
        currentPositions[i] = transitionManhattan ?
        manhattanLerp(begin, end, smoothstep(curt)) :
        euclideanLerp(begin, end, smoothstep(curt));
    }
    return currentPositions;
}
