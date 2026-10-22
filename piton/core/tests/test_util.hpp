#pragma once
#include <piton/level.hpp>

#include <doctest/doctest.h>

#include <string>

inline piton::Level loadTestLevel(int number) {
    piton::Level lv;
    std::string err;
    const bool ok = piton::Level::load(std::string(PITON_LEVELS_DIR) + "/" + piton::levelFileName(number), lv, &err);
    INFO(err);
    REQUIRE(ok);
    return lv;
}
