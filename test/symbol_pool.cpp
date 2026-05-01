#include "ast/symbol_pool.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <string_view>

using namespace stc;

TEST_CASE("SymbolPool basics", "[SymbolPool]") {
    SymbolPool pool{1};

    SECTION("inserting symbols") {
        SymbolId id1 = pool.get_id("test");
        SymbolId id2 = pool.get_id("case");

        REQUIRE_FALSE(id1.is_null());
        REQUIRE_FALSE(id2.is_null());
        REQUIRE(id1 != id2);
    }

    SECTION("interning") {
        SymbolId id1 = pool.get_id("test");
        SymbolId id2 = pool.get_id("test");
        SymbolId id3 = pool.get_id("something else");

        REQUIRE(id1 == id2);
        REQUIRE(id2 != id3);
    }

    SECTION("retrieval") {
        SymbolId id = pool.get_id("string data");

        auto opt_str = pool.get_symbol_maybe(id);
        REQUIRE(opt_str.has_value());
        REQUIRE(opt_str.value() == "string data");

        REQUIRE(pool.has_id(id));
        REQUIRE(pool.get_symbol(id) == "string data");

        // view to same string
        REQUIRE(pool.get_symbol(id).data() == pool.get_symbol(id).data());
    }

    SECTION("invalid ids") {
        SymbolId fake_id{9999};

        REQUIRE_FALSE(pool.has_id(fake_id));
        REQUIRE_FALSE(pool.get_symbol_maybe(fake_id).has_value());
    }
}
