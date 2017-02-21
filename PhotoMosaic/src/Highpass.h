#pragma once

#include "ofBaseTypes.h"
#include "opencv2/opencv.hpp"

class Highpass {
public:
    cv::Mat lab, lowpass, highpass;
    vector<cv::Mat> labChannels;
    
    void filter(ofPixels& pix, float size, float contrast=1) {
        if(size == 0) {
            return;
        }
        
        unsigned int w = pix.getWidth(), h = pix.getHeight();
        size *= MAX(w, h);
        
        // get cv::Mat from src and dst
        cv::Mat mat = cv::Mat(h, w, CV_8UC3, pix.getData(), 0);
        
        // allocate the first time
        lab.create(mat.rows, mat.cols, CV_8UC3);
        lowpass.create(mat.rows, mat.cols, CV_8UC1);
        highpass.create(mat.rows, mat.cols, CV_8UC1);
        
        // convert to Lab color and extract lightness
        cv::cvtColor(mat, lab, CV_RGB2Lab);
        cv::split(lab, labChannels);
        const cv::Mat& lightness = labChannels[0];
        
        // do lowpass filter
        size = ((size / 2) * 2) + 1; // foce size to be odd
        cv::blur(lightness, lowpass, cv::Size(size, size));
        
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
