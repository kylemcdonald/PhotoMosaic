#pragma once

#include "ofPixels.h"

/// A Tile is the smallest piece of a PhotoMosaic.
class Tile : public glm::vec2 {
public:
    int side;
    std::vector<ofColor> grid;
    float weight;
    Tile(int x, int y, int side, const vector<ofColor>& grid, float weight)
    :glm::vec2(x, y)
    ,side(side)
    ,grid(grid)
    ,weight(weight) {
    }
    static vector<Tile> buildTiles(const ofPixels& pix, int side);
    static float getCost(const Tile& a, const Tile& b);
};
