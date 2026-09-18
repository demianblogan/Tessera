#pragma once

#include <vector>

#include "Tetromino.h"

// Deals out piece types one at a time. In 7-bag mode (the default and the
// guideline standard), every type appears exactly once per shuffled batch of
// seven, so there's never a long drought of one piece or three of the same
// one in a row. `isSevenBagEnabled = false` deals pure uniform-random types
// instead -- a player-facing option, since some prefer the classic randomiser
// even though it can be streaky.
class TetrominoBag
{
public:
    explicit TetrominoBag(bool isSevenBagEnabled = true);

    [[nodiscard]] Tetromino::Type Next();

private:
    void Refill();

    bool isSevenBagEnabled;
    std::vector<Tetromino::Type> bag;
};
