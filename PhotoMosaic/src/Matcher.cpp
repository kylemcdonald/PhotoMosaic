#include "Matcher.h"

#include <random>
std::random_device rd;
std::default_random_engine gen(rd());

void Matcher::setStepTotal(unsigned int stepTotal) {
    this->stepTotal = stepTotal;
}

std::vector<unsigned int> Matcher::match(const std::vector<Tile>& src, const std::vector<Tile>& dst) {
    unsigned int n = dst.size();
    std::uniform_int_distribution<> dis(0, n-1);
    std::vector<unsigned int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    for(stepCurrent = 0; stepCurrent < stepTotal; stepCurrent++) {
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
        }
    }
    return indices;
}

float Matcher::getProgress() const {
    return float(stepCurrent) / float(stepTotal);
}
