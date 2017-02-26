#include "Matcher.h"

#include <random>
std::random_device rd;
std::default_random_engine gen(rd());

void Matcher::setRefinementSteps(unsigned int refinementSteps) {
    this->refinementSteps = refinementSteps;
}

void Matcher::setMaximumDuration(float maximumDurationSeconds) {
    this->maximumDurationSeconds = maximumDurationSeconds;
}

typedef std::pair<unsigned int, unsigned int> IndexedTile;
std::vector<unsigned int> getSortedTileIndices(const std::vector<Tile>& tiles) {
    unsigned int n = tiles.size();
    std::vector<IndexedTile> indexedTiles(n);
    std::vector<unsigned int> indices(n);
    for(unsigned int i = 0; i < n; i++) {
        indexedTiles[i].first = tiles[i].getColorSum();
        indexedTiles[i].second = i;
    }
    std::sort(indexedTiles.begin(), indexedTiles.end());
    for(unsigned int i = 0; i < n; i++) {
        indices[i] = indexedTiles[i].second;
    }
    return indices;
}

using namespace std::chrono;
std::vector<unsigned int> Matcher::match(const std::vector<Tile>& src, const std::vector<Tile>& dst) {
    auto start = steady_clock::now();
    unsigned int n = dst.size();
    
    // sort all the incoming tiles by their overall brightness
    std::vector<unsigned int> srcIndices = getSortedTileIndices(src);
    std::vector<unsigned int> dstIndices = getSortedTileIndices(dst);
    
    // use the sorted tiles to set the initial indices
    std::vector<unsigned int> indices(n);
    for(int i = 0; i < n; i++) {
        indices[srcIndices[i]] = dstIndices[i];
    }
    
    std::uniform_int_distribution<> dis(0, n-1);
    unsigned int checkDurationInterval = 1000;
    for(stepCurrent = 0; stepCurrent < refinementSteps; stepCurrent++) {
        // randomly select tile pairs and swap them when it works better
        unsigned int a = dis(gen);
        unsigned int b = dis(gen);
        if (a == b) continue;
        const Tile& srca = src[a];
        const Tile& srcb = src[b];
        unsigned int& ia = indices[a];
        unsigned int& ib = indices[b];
        const Tile& dsta = dst[ia];
        const Tile& dstb = dst[ib];
        long cursum = Tile::distance(srca, dsta) + Tile::distance(srcb, dstb);
        long swpsum = Tile::distance(srca, dstb) + Tile::distance(srcb, dsta);
        if(swpsum < cursum) {
            std::swap(ia, ib);
        }
        
        // check the duration and break if matching has taken too long
        if(stepCurrent % checkDurationInterval == 0) {
            auto stop = std::chrono::steady_clock::now();
            float elapsed = duration_cast<duration<float>>(stop - start).count();
            if(elapsed > maximumDurationSeconds) {
                break;
            }
        }
    }
    return indices;
}

float Matcher::getProgress() const {
    return float(stepCurrent) / float(refinementSteps);
}
