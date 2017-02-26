#pragma once
#include <vector>
#include <utility>
#include "opencv2/opencv.hpp"

/// Classic smoothstep function x^2 * (3 - 2*x)
float smoothstep(float x);

/// Get the center square from a given width and height.
cv::Rect getCenterSquare(int width, int height);

/// Build a grid with spacing side covering a space width x height.
std::vector<std::pair<int, int>> buildGrid(int width, int height, int side);

/// Interpolates along x then along y, linearly.
cv::Point2f manhattanLerp(cv::Point2f begin, cv::Point2f end, float t);

/// Interpolates along a line from begin to end.
cv::Point2f euclideanLerp(cv::Point2f begin, cv::Point2f end, float t);

/// Subtract the mean of a set of cv::Mats from each of those cv::Mats.
/// "0" is treated as "127", acting similarly to a highpass filter.
void subtractMean(std::vector<cv::Mat>& mats);

/// Pack a set of square cv::Mat images into a texture atlas.
/// Each image is resized to be size (side x side).
/// The positions of the tiles are returned by the positions argument.
cv::Mat buildAtlas(const std::vector<cv::Mat>& images, unsigned int side, std::vector<cv::Point2i>& positions);

/// Batch resize a collection of cv::Mat images to be size (side x side).
std::vector<cv::Mat> batchResize(const std::vector<cv::Mat>& src, unsigned int side);

/// Return a full-image region of interest from the mat with a given aspect ratio.
cv::Mat getRegionWithRatio(const cv::Mat& mat, float aspectRatio);
