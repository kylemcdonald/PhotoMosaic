#include "Matcher.h"

/// Sorts a set of objects and returns the indices of those objects.
template <class T>
std::vector<unsigned int> sortIndices(const std::vector<T>& v) {
    unsigned int n = v.size();
    std::vector<std::pair<const T*, unsigned int>> pairs(n);
    for(unsigned int i = 0; i < n; i++) {
        pairs[i].first = &v[i];
        pairs[i].second = i;
    }
    std::sort(pairs.begin(), pairs.end());
    std::vector<unsigned int> indices(n);
    for(unsigned int i = 0; i < n; i++) {
        indices[i] = pairs[i].second;
    }
    return indices;
}

void Matcher::setRefinementSteps(unsigned int refinementSteps) {
    this->refinementSteps = refinementSteps;
}

/// Set the maximum duration in seconds for match()
void Matcher::setMaximumDuration(float maximumDurationSeconds) {
    this->maximumDurationSeconds = maximumDurationSeconds;
}

/// Match two equal-length vectors of objects that have an operator-() and operator<()
/// Starts by sorting both sets and matching them up, then helpful random swaps.
std::vector<unsigned int> Matcher::match(const std::vector<Tile>& src, const std::vector<Tile>& dst) {
    using namespace std::chrono;
    auto start = steady_clock::now();
    unsigned int n = dst.size();
    
    // sort all the incoming tiles by their overall brightness
    std::vector<unsigned int> srcIndices = sortIndices(src);
    std::vector<unsigned int> dstIndices = sortIndices(dst);
    
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
        long cursum = (srca - dsta) + (srcb - dstb);
        long swpsum = (srca - dstb) + (srcb - dsta);
        if(swpsum < cursum) {
            std::swap(ia, ib);
        }
        
        // check the duration and break if matching has taken too long
        if(stepCurrent % checkDurationInterval == 0) {
            auto stop = std::chrono::steady_clock::now();
            durationCurrentSeconds = duration_cast<duration<float>>(stop - start).count();
            if(durationCurrentSeconds > maximumDurationSeconds) {
                break;
            }
        }
    }
    return indices;
}

float Matcher::getProgress() const {
    float stepProgress = float(stepCurrent) / float(refinementSteps);
    float durationProgress = durationCurrentSeconds / maximumDurationSeconds;
    return std::max(stepProgress, durationProgress);
}
