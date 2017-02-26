#pragma once

#include "Matcher.h"
#include "Highpass.h"

/// PhotoMosaic is composed of the Matcher and Highpass classes
/// and handles interaction between these classes.
class PhotoMosaic {
private:
    int side = 0, subsampling = 0;
    int width = 0, height = 0;
    int nx = 0, ny = 0, n = 0;
    
    Matcher matcher;
    Highpass highpass;
    cv::Mat dst;
    
    cv::Mat atlas;
    std::vector<cv::Point2i> atlasPositions;
    std::vector<Tile> srcTiles;
    std::vector<cv::Point2i> screenPositions;
    std::vector<unsigned int> matchedIndices;
    
    std::vector<cv::Point2i> beginPositions, endPositions;
    std::vector<float> transitionBegin, transitionEnd;
    bool transitionTopDown = false;
    bool transitionCircle = false;
    bool transitionManhattan = false;
    
public:
    
    /// Before using PhotoMosaic, you must call setup()
    /// Optionally, you may also set options with setRefinementSteps(),
    /// setFilterScale(), setFilterContrast(), or setTransitionStyle().
    /// And then load the icons with setIcons().
    void setup(int width, int height, int side=32, int subsampling=3);
    void setRefinementSteps(int refinementSteps);
    void setFilterScale(float filterScale);
    void setFilterContrast(float filterContrast);
    
    /// This can also be called before every match() to change the style.
    void setTransitionStyle(bool topDown, bool circle, bool manhattan);
    
    /// setIcons() will automatically resize images as necessary,
    /// but it assumes that the images are square. If they are not
    /// square, they will be stretched/squashed to fit.
    void setIcons(const std::vector<cv::Mat>& icons);
    
    /// After setting up the PhotoMosaic, call match() on an image.
    /// If the image does not match the size or aspect ratio, then
    /// match() will automatically crop into the image.
    void match(const cv::Mat& mat);
    
    
    /// Build an image of the finished Photomosaic.
    cv::Mat buildResult() const;
    
    int getWidth() const;
    int getHeight() const;
    int getSide() const;
    int getSubsampling() const;
    
    /// The atlas contains a subsection for each icon. This should be
    /// loaded into a texture for rendering a lot of images without
    /// doing a context switch on the GPU.
    const cv::Mat& getAtlas() const;
    const std::vector<cv::Point2i>& getAtlasPositions() const;
    
    /// Each of the tiles has a screen-based position.
    /// getScreenPositions() returns the original positions on screen.
    const std::vector<cv::Point2i>& getScreenPositions() const;
    
    /// getCurrentPositions(float t) returns the positions on screen
    /// after match(). t should stay in the range [0, 1]. Note that
    /// this version versions floating point positions, and the other
    /// returns integer positions.
    std::vector<cv::Point2f> getCurrentPositions(float t) const;
};
