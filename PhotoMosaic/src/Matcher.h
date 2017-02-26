#pragma once

#include "Highpass.h"
#include "Tile.h"

/// A Matcher finds a good solution for reorganizing Tiles.
/// Access `getProgress()` from another thread to monitor progress.
/// `match()` breaks after refinementSteps or maximumDurationSeconds,
/// whichever happens first.
class Matcher {
private:
    unsigned int stepCurrent = 0;
    unsigned int refinementSteps = 1000000;
    float maximumDurationSeconds = 1;
    
public:
    void setRefinementSteps(unsigned int refinementSteps);
    void setMaximumDuration(float maximumDurationSeconds);
    std::vector<unsigned int> match(const std::vector<Tile>& src, const std::vector<Tile>& dst);
    float getProgress() const;
};
