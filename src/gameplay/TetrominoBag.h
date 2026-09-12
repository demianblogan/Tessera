#pragma once

#include <vector>

#include "Tetromino.h"

// Deals out piece types one at a time. In 7-bag mode (the default and the
// guideline standard), every type appears exactly once per shuffled batch of
// seven, so there's never a long drought of one piece or three of the same
// one in a row. `sevenBagEnabled = false` deals pure uniform-random types
// instead -- a player-facing option, since some prefer the classic randomiser
// even though it can be streaky.
class TetrominoBag
{
public:
    explicit TetrominoBag(bool sevenBagEnabled = true);

    [[nodiscard]] Tetromino::Type Next();

private:
    void Refill();

    bool sevenBagEnabled;
    std::vector<Tetromino::Type> bag;
};
