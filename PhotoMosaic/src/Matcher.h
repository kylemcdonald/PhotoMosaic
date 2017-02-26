#pragma once

#include "Highpass.h"
#include "Tile.h"

/// A Matcher finds a good solution for reorganizing Tiles.
/// Access `getProgress()` from another thread to monitor progress.
class Matcher {
private:
public:
    unsigned int stepCurrent = 0;
    unsigned int refinementSteps = 1000000;
    void setRefinementSteps(unsigned int refinementSteps);
    std::vector<unsigned int> match(const std::vector<Tile>& src,
                                    const std::vector<Tile>& dst);
    float getProgress() const;
};
