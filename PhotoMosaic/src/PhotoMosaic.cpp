#include "PhotoMosaic.h"

void PhotoMosaic::setup(int width, int height, int side, int subsampling) {
    this->subsampling = subsampling;
    this->side = side;
    nx = width / side;
    ny = height / side;
    this->width = side * nx;
    this->height = side * ny;
    if (width != this->width || height != this->height) {
        std::cout << width << "x" << height << " is not evenly divisible by " << side <<
        ", using " << this->width << "x" << this->height << " instead" << std::endl;
    }
}
void PhotoMosaic::setRefinementSteps(int refinementSteps) { matcher.setRefinementSteps(refinementSteps); }
void PhotoMosaic::setFilterScale(float filterScale) { highpass.setFilterScale(filterScale); }
void PhotoMosaic::setFilterContrast(float filterContrast) { highpass.setFilterContrast(filterContrast); }

void PhotoMosaic::setIcons(const std::vector<cv::Mat>& icons) {
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
}

const std::vector<unsigned int>& PhotoMosaic::match(const cv::Mat& mat) {
    int w = subsampling * nx;
    int h = subsampling * ny;
    
    cv::Mat crop(getRegionWithRatio(mat, float(width) / height));
    cv::resize(crop, dst, cv::Size(w, h), 0, 0, cv::INTER_AREA);
    
    highpass.filter(dst);
    std::vector<Tile> dstTiles = Tile::buildTiles(dst, subsampling);
    
    matchedIndices = matcher.match(srcTiles, dstTiles);
    
    return matchedIndices;
}

int PhotoMosaic::getWidth() const { return width; }
int PhotoMosaic::getHeight() const { return height; }
int PhotoMosaic::getSide() const { return side; }
int PhotoMosaic::getSubsampling() const { return subsampling; }

const cv::Mat& PhotoMosaic::getAtlas() const { return atlas; }
const std::vector<cv::Point2i>& PhotoMosaic::getAtlasPositions() const { return atlasPositions; }
const std::vector<cv::Point2i>& PhotoMosaic::getScreenPositions() const { return screenPositions; }
