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
cv::Vec2f manhattanLerp(cv::Vec2f begin, cv::Vec2f end, float t);

/// Interpolates along a line from begin to end.
cv::Vec2f euclideanLerp(cv::Vec2f begin, cv::Vec2f end, float t);
