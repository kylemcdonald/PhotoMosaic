#include "Tile.h"

using namespace glm;

const int subsampling = 3;

/// Get the perceptual difference between two colors.
long getCost(const ofColor& c1, const ofColor& c2) {
    long rmean = ((long) c1.r + (long) c2.r) / 2;
    long r = (long) c1.r - (long) c2.r;
    long g = (long) c1.g - (long) c2.g;
    long b = (long) c1.b - (long) c2.b;
    return ((((512 + rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
}

/// Get the perceptual difference between two tiles.
float Tile::getCost(const Tile& a, const Tile& b) {
    const int n = subsampling * subsampling;
    unsigned long total = 0;
    for(int i = 0; i < n; i++) {
        total += ::getCost(a.grid[i], b.grid[i]);
    }
    return total * (a.weight + b.weight);
}

/// Get the average color from a region in pix.
ofColor getAverage(const ofPixels& pix, int x, int y, int w, int h) {
    unsigned long r = 0, g = 0, b = 0;
    for(int j = y; j < y + h; j++) {
        for(int i = x; i < x + w; i++) {
            const ofColor& cur = pix.getColor(i, j);
            r += cur.r;
            g += cur.g;
            b += cur.b;
        }
    }
    unsigned long n = w * h;
    return ofColor(r / n, g / n, b / n);
}

vector<Tile> Tile::buildTiles(const ofPixels& pix, int side) {
    ofLog() << "Building tiles for " << pix.getWidth() << "x" << pix.getHeight() << " at side " << side;
    
    // we could do this with resizing but OF doesn't have a good downsampling method
    float subsample = (float) side / subsampling;
    int w = pix.getWidth(), h = pix.getHeight();
    vector<Tile> tiles;
    vec2 center = vec2(w, h) / 2;
    for(int y = 0; y < h; y+=side) {
        for(int x = 0; x < w; x+=side) {
            vector<ofColor> grid;
            for(int ky = 0; ky < subsampling; ky++) {
                for(int kx = 0; kx < subsampling; kx++) {
                    grid.push_back(getAverage(pix, x+kx*subsample, y+ky*subsample, subsample, subsample));
                }
            }
            float weight = ofMap(distance(vec2(x, y), center), 0, w / 2, 1, 0, true);
            tiles.emplace_back(x, y, side, grid, weight);
        }
    }
    return tiles;
}
