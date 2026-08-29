#pragma once

#include <vector>

#include "Tetromino.h"

class TetrominoBag
{
private:
    std::vector<Tetromino::Type> bag;

    void Refill();

public:
    [[nodiscard]] Tetromino::Type Next();
};