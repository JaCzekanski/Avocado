#include "utils/logic.h"
#include <catch2/catch.hpp>

namespace utils {

TEST_CASE("Extend 10bit integer - positive", "[extend_sign]") { REQUIRE(extend_sign<10>(0x1FF) == (int16_t)0x1FF); }

TEST_CASE("Extend 10bit integer - positive with garbage", "[extend_sign]") { REQUIRE(extend_sign<10>(0xF9FF) == (int16_t)0x1FF); }

TEST_CASE("Extend 10bit integer - negative", "[extend_sign]") { REQUIRE(extend_sign<10>(0x7FF) == (int16_t)0xFFFF); }

TEST_CASE("Extend 10bit integer - negative with garbage", "[extend_sign]") { REQUIRE(extend_sign<10>(0x77FF) == (int16_t)0xFFFF); }

}  // namespace utils