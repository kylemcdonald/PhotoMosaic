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
