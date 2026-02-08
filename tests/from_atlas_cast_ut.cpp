// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Runtime tests for the from_atlas_cast constructor feature.
//
// Tests:
// - Construct one atlas type from another with same underlying type
// - Multi-level drilling (nested atlas types)
// - lvalue semantics (copy)
// - rvalue semantics (move)
// - constexpr construction
// - SFINAE rejection: types without from_atlas_cast
// - SFINAE rejection: same type (copy/move ctors take priority)
// - SFINAE rejection: non-atlas types (variadic ctor takes priority)
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "from_atlas_cast_test_types.hpp"

#include <string>
#include <type_traits>

#include "doctest.hpp"

// ======================================================================
// TEST SUITE: BASIC CROSS-TYPE CONSTRUCTION
// ======================================================================

TEST_SUITE("from_atlas_cast - Basic")
{
    TEST_CASE("Construct IntTypeA from IntTypeB")
    {
        test::IntTypeB b{42};
        test::IntTypeA a{b};

        CHECK(atlas::undress(a) == 42);
    }

    TEST_CASE("Construct IntTypeB from IntTypeA")
    {
        test::IntTypeA a{42};
        test::IntTypeB b{a};

        CHECK(atlas::undress(b) == 42);
    }

    TEST_CASE("Value is preserved through cross-type construction")
    {
        test::IntTypeA a{-1};
        test::IntTypeB b{a};

        CHECK(atlas::undress(b) == -1);
    }
}

// ======================================================================
// TEST SUITE: MULTI-LEVEL DRILLING
// ======================================================================

TEST_SUITE("from_atlas_cast - Multi-level drilling")
{
    TEST_CASE("Construct IntTypeA from NestedType (two levels)")
    {
        // NestedType wraps IntTypeA wraps int
        // Constructing IntTypeB from NestedType should drill through
        test::NestedType nested{test::IntTypeA{99}};
        test::IntTypeB b{nested};

        CHECK(atlas::undress(b) == 99);
    }

    TEST_CASE("Construct IntTypeA from NestedType rvalue")
    {
        test::IntTypeB b{test::NestedType{test::IntTypeA{77}}};

        CHECK(atlas::undress(b) == 77);
    }
}

// ======================================================================
// TEST SUITE: VALUE CATEGORY SEMANTICS
// ======================================================================

TEST_SUITE("from_atlas_cast - Value category")
{
    TEST_CASE("lvalue source copies the string")
    {
        test::StringTypeB b{std::string("hello")};
        test::StringTypeA a{b};

        CHECK(atlas::undress(a) == "hello");
        // Original should be unchanged (copied, not moved)
        CHECK(atlas::undress(b) == "hello");
    }

    TEST_CASE("rvalue source moves the string")
    {
        test::StringTypeB b{std::string("hello")};
        test::StringTypeA a{std::move(b)};

        CHECK(atlas::undress(a) == "hello");
        // After move, source should be empty (moved-from state)
        CHECK(atlas::undress(b).empty());
    }
}

// ======================================================================
// TEST SUITE: CONSTEXPR
// ======================================================================

TEST_SUITE("from_atlas_cast - constexpr")
{
    TEST_CASE("constexpr cross-type construction")
    {
        constexpr test::IntTypeB b{42};
        constexpr test::IntTypeA a{b};

        static_assert(atlas::undress(a) == 42, "constexpr from_atlas_cast");
        CHECK(atlas::undress(a) == 42);
    }
}

// ======================================================================
// TEST SUITE: SFINAE REJECTION
// ======================================================================

TEST_SUITE("from_atlas_cast - SFINAE")
{
    TEST_CASE("Type without from_atlas_cast is not constructible from atlas types")
    {
        // NoFromCastType does NOT have from_atlas_cast
        CHECK_FALSE(std::is_constructible<test::NoFromCastType, test::IntTypeA>::value);
        CHECK_FALSE(std::is_constructible<test::NoFromCastType, test::IntTypeB>::value);
    }

    TEST_CASE("Non-atlas types use variadic ctor, not from_atlas_cast")
    {
        // int is not an atlas type, so it goes through the variadic ctor
        // This should compile and work normally
        test::IntTypeA a{42};
        CHECK(atlas::undress(a) == 42);
    }

    TEST_CASE("Same-type construction uses copy/move, not from_atlas_cast")
    {
        // Copy construction
        test::IntTypeA a1{42};
        test::IntTypeA a2{a1};
        CHECK(atlas::undress(a2) == 42);

        // Move construction
        test::IntTypeA a3{std::move(a1)};
        CHECK(atlas::undress(a3) == 42);
    }
}
