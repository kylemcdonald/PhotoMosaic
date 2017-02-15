#pragma once
#include "ofxCv.h"

class Highpass {
public:
    cv::Mat lab, lowpass, highpass;
    vector<cv::Mat> labChannels;
    
    template <class S, class D>
    void filter(S& src, D& dst, int size, float contrast=1) {
        if(size == 0) {
            ofxCv::copy(src, dst);
            return;
        }
        
        // get cv::Mat from src and dst
        cv::Mat srcMat = ofxCv::toCv(src);
        cv::Mat dstMat = ofxCv::toCv(dst);
        
        // allocate the first time
        lab.create(srcMat.rows, srcMat.cols, CV_8UC3);
        lowpass.create(srcMat.rows, srcMat.cols, CV_8UC1);
        highpass.create(srcMat.rows, srcMat.cols, CV_8UC1);
        
        // convert to Lab color and extract lightness
        cv::cvtColor(srcMat, lab, CV_RGB2Lab);
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
        highpass += 128; // should be diff for other datatypes
        highpass.convertTo(lightness, lightness.type());
        
        ofxCv::imitate(dst, src);
        cv::merge(labChannels, dstMat);
        cv::cvtColor(dstMat, dstMat, CV_Lab2RGB);
    }
};
