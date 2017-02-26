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
    
    /// Before using PhotoMosaic, you must call setup()
    /// Optionally, you may also set options with setRefinementSteps(),
    /// setFilterScale(), setFilterContrast(). And then load the icons
    /// with setIcons().
    void setup(int width, int height, int side=32, int subsampling=3);
    void setRefinementSteps(int refinementSteps);
    void setFilterScale(float filterScale);
    void setFilterContrast(float filterContrast);
    
    /// setIcons() will automatically resize images as necessary,
    /// but it assumes that the images are square. If they are not
    /// square, they will be stretched/squashed to fit.
    void setIcons(const std::vector<cv::Mat>& icons);
    
    /// After setting up PhotoMosaic call match() on an image.
    /// If the image does not match the size or aspect ratio, then
    /// match() will automatically crop into the image.
    const std::vector<unsigned int>& match(const cv::Mat& mat);
    
    int getWidth() const;
    int getHeight() const;
    int getSide() const;
    int getSubsampling() const;
    
    const cv::Mat& getAtlas() const;
    const std::vector<cv::Point2i>& getAtlasPositions() const;
    const std::vector<cv::Point2i>& getScreenPositions() const;
};
