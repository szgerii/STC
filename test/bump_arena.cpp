#include "common/bump_arena.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

using namespace stc;

struct CtorTracker {
    bool ctor_invoked_with_args;

    CtorTracker()
        : ctor_invoked_with_args{false} {}
    CtorTracker(int)
        : ctor_invoked_with_args{true} {}
};

struct DtorTracker {
    bool* was_destroyed;
    DtorTracker(bool* flag)
        : was_destroyed(flag) {
        *was_destroyed = false;
    }
    ~DtorTracker() { *was_destroyed = true; }
};

TEST_CASE("init and basic alloc", "[BumpArena]") {
    BumpArena32 arena{128};

    SECTION("initial state") {
        REQUIRE(arena.get_current_offset() == 1U); // needed for null id-s to work
    }

    SECTION("allocate_for") {
        auto [id, ptr] = arena.allocate_for<int>();
        REQUIRE(ptr != nullptr);
        REQUIRE(id == 1U);
        REQUIRE(arena.get_current_offset() == 1U + sizeof(int));

        int* retrieved = arena.get_ptr<int>(id);
        REQUIRE(retrieved == static_cast<int*>(ptr));
    }

    SECTION("allocate_for alignedness") {
        auto [off1, p1] = arena.allocate_for<char>();
        auto [off2, p2] = arena.allocate_for<double>();

        REQUIRE((reinterpret_cast<uintptr_t>(p2) % alignof(double)) == 0);

        REQUIRE(off2 >= off1 + sizeof(char));
    }
}

TEST_CASE("capacity, slabs", "[BumpArena]") {
    BumpArena32 arena{32};

    SECTION("new slab") {
        auto [off1, p1] = arena.allocate_for<int>(4); // 16 bytes
        auto [off2, p2] = arena.allocate_for<int>(6); // 24 bytes

        REQUIRE(p1 != nullptr);
        REQUIRE(p2 != nullptr);

        // different slabs -> non-contigouity
        REQUIRE(p2 != static_cast<std::byte*>(p1) + 16);

        // lookup across slabs
        REQUIRE(arena.get_ptr<int>(off1) == p1);
        REQUIRE(arena.get_ptr<int>(off2) == p2);
    }
}

TEST_CASE("emplace and dtors", "[BumpArena]") {
    BumpArena32 arena{1024};

    SECTION("emplace and ctor calling") {
        auto [off, ptr] = arena.emplace<int>(42);
        REQUIRE(ptr != nullptr);
        REQUIRE(*ptr == 42);

        auto [off2, ptr2] = arena.emplace<CtorTracker>(42);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr2->ctor_invoked_with_args);

        auto [off3, ptr3] = arena.emplace<CtorTracker>();
        REQUIRE(ptr3 != nullptr);
        REQUIRE(ptr3->ctor_invoked_with_args);
    }

    SECTION("dtor calling") {
        bool flag1 = false;
        bool flag2 = false;

        {
            BumpArena32 scoped_arena{1024};
            std::ignore = scoped_arena.emplace<DtorTracker>(&flag1);
            std::ignore = scoped_arena.emplace<DtorTracker>(&flag2);

            REQUIRE_FALSE(flag1);
            REQUIRE_FALSE(flag2);
        }

        REQUIRE(flag1);
        REQUIRE(flag2);
    }

    SECTION("reset") {
        bool flag   = false;
        std::ignore = arena.emplace<DtorTracker>(&flag);

        arena.reset(); // reset should call dtors

        REQUIRE(flag);
        REQUIRE(arena.get_current_offset() == 1U);
    }
}

TEST_CASE("get_offset", "[BumpArena]") {
    BumpArena32 arena{1024};

    auto [off1, ptr1] = arena.allocate_for<int>();
    auto [off2, ptr2] = arena.allocate_for<double>();

    SECTION("ptr -> offset") {
        REQUIRE(arena.get_offset(ptr1) == off1);
        REQUIRE(arena.get_offset(ptr2) == off2);
    }

    SECTION("invalid ptr") {
        int not_in_arena = 0;
        REQUIRE(arena.get_offset(&not_in_arena) == 0U);
        REQUIRE(arena.get_offset(nullptr) == 0U);
    }
}
