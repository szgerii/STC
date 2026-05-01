#include <catch2/catch_test_macros.hpp>
#include "api/transpiler.h"
#include "meta.h"

TEST_CASE("library version test", "[sanity]") {
    REQUIRE(stc::api::stc_abi_version() == stc::meta::version_major);
}
