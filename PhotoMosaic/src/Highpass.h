#pragma once
#include "opencv2/opencv.hpp"

class Highpass {
private:
    float filterScale = 0.10;
    float filterContrast = 1;
    cv::Mat lab, lowpass, highpass;
    std::vector<cv::Mat> labChannels;
    
public:
    /// filterScale should be between 0 to 1
    void setFilterScale(float filterScale);
    
    /// filterContrast should be between 0.1 to 10
    void setFilterContrast(float filterContrast);
    
    /// Filters an image in-place.
    void filter(cv::Mat& mat);
};
