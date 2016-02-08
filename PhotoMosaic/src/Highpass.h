#pragma once
#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

class Highpass {
public:
    cv::Mat lab, lowpass, highpass;
    vector<cv::Mat> labChannels;
    
    template <class S, class D>
    void filter(S& src, D& dst, int size, float contrast = 1) {
        if(size == 0) {
            ofxCv::copy(src, dst);
            return;
        }
        ofxCv::convertColor(src, lab, CV_RGB2Lab);
        cv::split(lab, labChannels);
        cv::Mat& lightness = labChannels[0];
        ofxCv::blur(lightness, lowpass, size);
        // could convert to 16s instead of 32f for extra speed
        cv::subtract(lightness, lowpass, highpass, cv::noArray(), CV_32F);
        if (contrast != 1) {
            highpass *= contrast;
        }
        highpass += 128; // should be diff for other datatypes
        highpass.convertTo(lightness, lightness.type());
        ofxCv::imitate(dst, src);
        cv::merge(labChannels, ofxCv::toCv(dst));
        ofxCv::convertColor(dst, dst, CV_Lab2RGB);
    }
};