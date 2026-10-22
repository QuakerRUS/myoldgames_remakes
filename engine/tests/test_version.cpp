#include <doctest/doctest.h>
#include <vs/engine/version.hpp>

TEST_CASE("engine reports its version") {
    CHECK(vs::engine_version() == "0.1.0");
}
