// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive tests for atlas::undress()
//
// Tests all value categories and edge cases:
// - Lvalue const reference - returns const reference
// - Lvalue non-const reference - returns non-const reference
// - Rvalue (moveable type) - returns by value
// - Rvalue (non-moveable type) - SFINAE'd out
// - Nested atlas types - drills down to innermost value
// - Non-atlas types - returns value unchanged
// - Enums - converts to underlying type (same as unwrap)
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "undress_test_types.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <type_traits>

#include "doctest.hpp"

// ======================================================================
// TEST SUITE: BASIC VALUE CATEGORIES
// ======================================================================

TEST_SUITE("atlas::undress - Value Categories")
{
    TEST_CASE("Lvalue const reference returns const reference")
    {
        test::SimpleInt const x{42};
        auto const & result = atlas::undress(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int const &>::value);
    }

    TEST_CASE("Lvalue non-const reference returns non-const reference")
    {
        test::SimpleInt x{42};
        auto & result = atlas::undress(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::undress(x) == 100);
    }

    TEST_CASE("Rvalue returns by value for moveable types")
    {
        auto result = atlas::undress(test::SimpleInt{42});

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Rvalue with moveable underlying type moves correctly")
    {
        // Create a type with a moveable underlying value (string)
        auto result = atlas::undress(test::MovableString{"hello"});

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string>::value);
    }
}

// ======================================================================
// TEST SUITE: NESTED TYPES
// ======================================================================

TEST_SUITE("atlas::undress - Nested Types")
{
    TEST_CASE("Single level nesting extracts underlying value")
    {
        test::SimpleInt x{42};
        auto & result = atlas::undress(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::undress(x) == 100);
    }

    TEST_CASE("Double level nesting drills down to innermost value")
    {
        test::NestedInt x{test::SimpleInt{42}};

        auto & result = atlas::undress(x);

        // Should drill down to the int, not stop at SimpleInt
        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::undress(x) == 100);
    }

    TEST_CASE("Triple level nesting drills down completely")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};

        auto & result = atlas::undress(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::undress(x) == 100);
    }
}

// ======================================================================
// TEST SUITE: NON-ATLAS TYPES
// ======================================================================

TEST_SUITE("atlas::undress - Non-Atlas Types")
{
    TEST_CASE("Primitive types return unchanged")
    {
        int x = 42;
        auto & result = atlas::undress(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        result = 100;
        CHECK(x == 100);
    }

    TEST_CASE("Const primitive types return const reference")
    {
        int const x = 42;
        auto const & result = atlas::undress(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int const &>::value);
    }

    TEST_CASE("std::string returns unchanged")
    {
        std::string x = "hello";
        auto & result = atlas::undress(x);

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string &>::value);
    }

    TEST_CASE("Rvalue non-atlas type returns by value")
    {
        auto result = atlas::undress(std::string{"hello"});

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string>::value);
    }
}

// ======================================================================
// TEST SUITE: CONSTEXPR
// ======================================================================

TEST_SUITE("atlas::undress - Constexpr")
{
    TEST_CASE("Works in constexpr context with lvalue")
    {
        constexpr test::SimpleInt x{42};
        constexpr int result = atlas::undress(x);

        CHECK(result == 42);
    }

    TEST_CASE("Works in constexpr context with rvalue")
    {
        constexpr int result = atlas::undress(test::SimpleInt{42});

        CHECK(result == 42);
    }
}

// ======================================================================
// TEST SUITE: MOVE-ONLY TYPES
// ======================================================================

TEST_SUITE("atlas::undress - Move-Only Types")
{
    TEST_CASE("Lvalue returns reference (no move needed)")
    {
        test::MoveOnlyWrapper x{std::make_unique<int>(42)};

        auto & result = atlas::undress(x);

        CHECK(*result == 42);
        CHECK(std::is_same<decltype(result), std::unique_ptr<int> &>::value);
    }

    TEST_CASE("Const lvalue returns const reference")
    {
        test::MoveOnlyWrapper const x{std::make_unique<int>(42)};

        auto const & result = atlas::undress(x);

        CHECK(*result == 42);
        CHECK(std::is_same<decltype(result), std::unique_ptr<int> const &>::
                  value);
    }
}

// ======================================================================
// TEST SUITE: EDGE CASES
// ======================================================================

TEST_SUITE("atlas::undress - Edge Cases")
{
    TEST_CASE("Default constructed value")
    {
        test::SimpleInt x{};
        CHECK(atlas::undress(x) == 0);
    }

    TEST_CASE("Negative values")
    {
        test::SimpleInt x{-42};
        CHECK(atlas::undress(x) == -42);
    }

    TEST_CASE("Large values")
    {
        test::SimpleInt x{std::numeric_limits<int>::max()};
        CHECK(atlas::undress(x) == std::numeric_limits<int>::max());
    }
}

// ======================================================================
// TEST SUITE: ENUMS
// ======================================================================

// Test enums for undress
enum class ScopedColor : int { Red = 1, Green = 2, Blue = 3 };
enum UnscopedSize : short { Small = 10, Medium = 20, Large = 30 };

TEST_SUITE("atlas::undress - Enums")
{
    TEST_CASE("Scoped enum returns underlying type")
    {
        auto result = atlas::undress(ScopedColor::Red);

        CHECK(result == 1);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Scoped enum with different values")
    {
        CHECK(atlas::undress(ScopedColor::Green) == 2);
        CHECK(atlas::undress(ScopedColor::Blue) == 3);
    }

    TEST_CASE("Unscoped enum returns underlying type")
    {
        auto result = atlas::undress(Small);

        CHECK(result == 10);
        CHECK(std::is_same<decltype(result), short>::value);
    }

    TEST_CASE("Unscoped enum with different values")
    {
        CHECK(atlas::undress(Medium) == 20);
        CHECK(atlas::undress(Large) == 30);
    }

    TEST_CASE("Enum lvalue returns underlying type by value")
    {
        ScopedColor color = ScopedColor::Blue;
        auto result = atlas::undress(color);

        CHECK(result == 3);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Const enum lvalue returns underlying type by value")
    {
        ScopedColor const color = ScopedColor::Green;
        auto result = atlas::undress(color);

        CHECK(result == 2);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Enum in constexpr context")
    {
        constexpr int result = atlas::undress(ScopedColor::Blue);

        CHECK(result == 3);
    }

    TEST_CASE("undress and unwrap yield same result for enums")
    {
        CHECK(atlas::undress(ScopedColor::Red) == atlas::unwrap(ScopedColor::Red));
        CHECK(atlas::undress(ScopedColor::Green) == atlas::unwrap(ScopedColor::Green));
        CHECK(atlas::undress(Small) == atlas::unwrap(Small));
        CHECK(atlas::undress(Large) == atlas::unwrap(Large));
    }
}
