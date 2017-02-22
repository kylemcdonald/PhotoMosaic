#pragma once

#include <algorithm>
#include "opencv2/opencv.hpp"

class Highpass {
public:
    cv::Mat lab, lowpass, highpass;
    vector<cv::Mat> labChannels;
    
    void filter(cv::Mat& mat, float filterSize, float contrast=1) {
        if(filterSize == 0) {
            return;
        }
        
        // size is a ratio of the largest side
        filterSize *= std::max(mat.rows, mat.cols);
        
        // allocate the first time
        lab.create(mat.size(), CV_8UC3);
        lowpass.create(mat.size(), CV_8UC1);
        highpass.create(mat.size(), CV_8UC1);
        
        // convert to Lab color and extract lightness
        cv::cvtColor(mat, lab, CV_RGB2Lab);
        cv::split(lab, labChannels);
        const cv::Mat& lightness = labChannels[0];
        
        // do lowpass filter
        filterSize = ((filterSize / 2) * 2) + 1; // foce size to be odd
        cv::blur(lightness, lowpass, cv::Size(filterSize, filterSize));
        
        // use lowpass to produce highpass
        // could convert to 16s instead of 32f for extra speed
        cv::subtract(lightness, lowpass, highpass, cv::noArray(), CV_32F);
        if (contrast != 1) {
            highpass *= contrast;
        }
        highpass += 128; // would be diff for other datatypes
        highpass.convertTo(lightness, lightness.type());

        cv::merge(labChannels, mat);
        cv::cvtColor(mat, mat, CV_Lab2RGB);
    }
};
