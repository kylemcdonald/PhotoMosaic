#pragma once

#include "Tile.h"
#include <random>

/// A Matcher finds a good solution for matching two set of objects.
/// Access `getProgress()` from another thread to monitor progress.
/// `match()` breaks after refinementSteps or maximumDurationSeconds,
/// whichever happens first.
class Matcher {
private:
    unsigned int stepCurrent = 0;
    unsigned int refinementSteps = 1000000;
    float durationCurrentSeconds = 0;
    float maximumDurationSeconds = 1;
    
    std::random_device rd;
    std::default_random_engine gen;
    
public:
    Matcher()
    :gen(rd()) {
    }
    
    /// Set the maximum number of refinement steps for match()
    /// On a laptop 10000 steps take ~2 milliseconds.
    void setRefinementSteps(unsigned int refinementSteps);
    
    /// Set the maximum duration in seconds for match()
    void setMaximumDuration(float maximumDurationSeconds);
    
    /// Match up two equal-length vectors of Tiles.
    /// Starts by sorting both sets and matching them up, then searches for good random swaps.
    std::vector<unsigned int> match(const std::vector<Tile>& src, const std::vector<Tile>& dst);
    
    /// When running match() in a thread, getProgress() is useful for reporting
    /// how much longer the match will take.
    float getProgress() const;
};
