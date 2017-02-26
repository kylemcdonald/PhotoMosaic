#include "Matcher.h"

#include <random>
std::random_device rd;
std::default_random_engine gen(rd());

void Matcher::setRefinementSteps(unsigned int refinementSteps) {
    this->refinementSteps = refinementSteps;
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

std::vector<unsigned int> Matcher::match(const std::vector<Tile>& src, const std::vector<Tile>& dst) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<unsigned int> srcIndices = getSortedTileIndices(src);
    std::vector<unsigned int> dstIndices = getSortedTileIndices(dst);
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << microseconds << "us for initial sorting" << std::endl;
    
    unsigned int n = dst.size();
    std::vector<unsigned int> indices(n);
    for(int i = 0; i < n; i++) {
        indices[srcIndices[i]] = dstIndices[i];
    }
    
    // about 2% of the swaps are worthwhile
    unsigned int totalSwaps = 0;
    std::uniform_int_distribution<> dis(0, n-1);
    for(stepCurrent = 0; stepCurrent < refinementSteps; stepCurrent++) {
        unsigned int a = dis(gen);
        unsigned int b = dis(gen);
        if (a == b) continue;
        const Tile& srca = src[a];
        const Tile& srcb = src[b];
        unsigned int& ia = indices[a];
        unsigned int& ib = indices[b];
        const Tile& dsta = dst[ia];
        const Tile& dstb = dst[ib];
        long cursum = Tile::getCost(srca, dsta) + Tile::getCost(srcb, dstb);
        long swpsum = Tile::getCost(srca, dstb) + Tile::getCost(srcb, dsta);
        if(swpsum < cursum) {
            std::swap(ia, ib);
            totalSwaps++;
        }
    }
    std::cout << float(100*totalSwaps)/refinementSteps << "% swaps " << std::endl;
    return indices;
}

float Matcher::getProgress() const {
    return float(stepCurrent) / float(refinementSteps);
}
