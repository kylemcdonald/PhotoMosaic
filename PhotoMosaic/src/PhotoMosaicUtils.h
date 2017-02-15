#pragma once

#include "ofMath.h"
#include "ofFileUtils.h"
#include "ofImage.h"
#include "ofTypes.h"

/// Select and return a random item from `x`.
template <class T>
const T& randomChoice(const std::vector<T>& x) {
    return x[ofRandom(x.size())];
}

/// Classic smoothstep function x^2 * (3 - 2*x)
float smoothstep(float x);

/// Filter jpg, jpeg, png files in directory and return them as a list of ofFile.
std::vector<ofFile> listImages(string directory);

/// Draw a square subsection from the center of img at position x, y with width and height equal to side.
void drawCenterSquare(const ofImage& img, float x, float y, float side);

/// Build a grid with spacing side covering a space width x height.
std::vector<pair<int, int>> buildGrid(int width, int height, int side);

/// Build a grid from images in directory dir covering a space width x height with tiles of size side.
ofPixels buildGrid(string dir, int width, int height, int side);

/// Interpolates along x then along y, linearly.
glm::vec2 manhattanLerp(glm::vec2 begin, glm::vec2 end, float t);

/// Interpolates along a line from begin to end.
glm::vec2 euclideanLerp(glm::vec2 begin, glm::vec2 end, float t);

/// Add a subsection of tex to a mesh as two triangles.
void addSubsection(ofMesh& mesh, ofTexture& tex, float x, float y, float w, float h, float sx, float sy);
