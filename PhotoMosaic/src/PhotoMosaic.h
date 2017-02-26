#pragma once

#include "Matcher.h"
#include "Highpass.h"
#include "Utils.h"

/// PhotoMosaic is composed of the Matcher and Highpass classes
/// and handles interaction between these classes.
class PhotoMosaic {
private:
    int side = 0, subsampling = 0;
    int width = 0, height = 0;
    int nx = 0, ny = 0;
    
    Matcher matcher;
    Highpass highpass;
    cv::Mat dst;
    
    cv::Mat atlas;
    std::vector<cv::Point2i> atlasPositions;
    std::vector<Tile> srcTiles;
    std::vector<cv::Point2i> screenPositions;
    std::vector<unsigned int> matchedIndices;
    
public:
    void setup(int width, int height, int side=32, int subsampling=3);
    void setRefinementSteps(int refinementSteps);
    void setFilterScale(float filterScale);
    void setFilterContrast(float filterContrast);
    void setIcons(const std::vector<cv::Mat>& icons);
    
    const std::vector<unsigned int>& match(const cv::Mat& mat);
    
    int getWidth() const;
    int getHeight() const;
    int getSide() const;
    int getSubsampling() const;
    
    const cv::Mat& getAtlas() const;
    const std::vector<cv::Point2i>& getAtlasPositions() const;
    const std::vector<cv::Point2i>& getScreenPositions() const;
};
