// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive tests for atlas::to_underlying()
//
// Tests all value categories and edge cases:
// - Lvalue const reference - returns const reference
// - Lvalue non-const reference - returns non-const reference
// - Rvalue (moveable type) - returns by value
// - Rvalue (non-moveable type) - SFINAE'd out
// - Nested atlas types - drills down to innermost value
// - Non-atlas types - returns value unchanged
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include "to_underlying_test_types.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <type_traits>

// ======================================================================
// TEST SUITE: BASIC VALUE CATEGORIES
// ======================================================================

TEST_SUITE("atlas::to_underlying - Value Categories")
{
    TEST_CASE("Lvalue const reference returns const reference")
    {
        test::SimpleInt const x{42};
        auto const & result = atlas::to_underlying(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int const &>::value);
    }

    TEST_CASE("Lvalue non-const reference returns non-const reference")
    {
        test::SimpleInt x{42};
        auto & result = atlas::to_underlying(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::to_underlying(x) == 100);
    }

    TEST_CASE("Rvalue returns by value for moveable types")
    {
        auto result = atlas::to_underlying(test::SimpleInt{42});

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Rvalue with moveable underlying type moves correctly")
    {
        // Create a type with a moveable underlying value (string)
        auto result = atlas::to_underlying(test::MovableString{"hello"});

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string>::value);
    }
}

// ======================================================================
// TEST SUITE: NESTED TYPES
// ======================================================================

TEST_SUITE("atlas::to_underlying - Nested Types")
{
    TEST_CASE("Single level nesting extracts underlying value")
    {
        test::SimpleInt x{42};
        auto & result = atlas::to_underlying(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::to_underlying(x) == 100);
    }

    TEST_CASE("Double level nesting drills down to innermost value")
    {
        test::NestedInt x{test::SimpleInt{42}};

        auto & result = atlas::to_underlying(x);

        // Should drill down to the int, not stop at SimpleInt
        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::to_underlying(x) == 100);
    }

    TEST_CASE("Triple level nesting drills down completely")
    {
        test::TripleNestedInt x{
            test::NestedInt{test::SimpleInt{42}}};

        auto & result = atlas::to_underlying(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::to_underlying(x) == 100);
    }
}

// ======================================================================
// TEST SUITE: NON-ATLAS TYPES
// ======================================================================

TEST_SUITE("atlas::to_underlying - Non-Atlas Types")
{
    TEST_CASE("Primitive types return unchanged")
    {
        int x = 42;
        auto & result = atlas::to_underlying(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        result = 100;
        CHECK(x == 100);
    }

    TEST_CASE("Const primitive types return const reference")
    {
        int const x = 42;
        auto const & result = atlas::to_underlying(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int const &>::value);
    }

    TEST_CASE("std::string returns unchanged")
    {
        std::string x = "hello";
        auto & result = atlas::to_underlying(x);

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string &>::value);
    }

    TEST_CASE("Rvalue non-atlas type returns by value")
    {
        auto result = atlas::to_underlying(std::string{"hello"});

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string>::value);
    }
}

// ======================================================================
// TEST SUITE: CONSTEXPR
// ======================================================================

TEST_SUITE("atlas::to_underlying - Constexpr")
{
    TEST_CASE("Works in constexpr context with lvalue")
    {
        constexpr test::SimpleInt x{42};
        constexpr int result = atlas::to_underlying(x);

        CHECK(result == 42);
    }

    TEST_CASE("Works in constexpr context with rvalue")
    {
        constexpr int result = atlas::to_underlying(test::SimpleInt{42});

        CHECK(result == 42);
    }
}

// ======================================================================
// TEST SUITE: MOVE-ONLY TYPES
// ======================================================================

TEST_SUITE("atlas::to_underlying - Move-Only Types")
{
    TEST_CASE("Lvalue returns reference (no move needed)")
    {
        test::MoveOnlyWrapper x{std::make_unique<int>(42)};

        auto & result = atlas::to_underlying(x);

        CHECK(*result == 42);
        CHECK(std::is_same<decltype(result), std::unique_ptr<int> &>::value);
    }

    TEST_CASE("Const lvalue returns const reference")
    {
        test::MoveOnlyWrapper const x{std::make_unique<int>(42)};

        auto const & result = atlas::to_underlying(x);

        CHECK(*result == 42);
        CHECK(std::is_same<decltype(result),
                           std::unique_ptr<int> const &>::value);
    }

}

// ======================================================================
// TEST SUITE: EDGE CASES
// ======================================================================

TEST_SUITE("atlas::to_underlying - Edge Cases")
{
    TEST_CASE("Default constructed value")
    {
        test::SimpleInt x{};
        CHECK(atlas::to_underlying(x) == 0);
    }

    TEST_CASE("Negative values")
    {
        test::SimpleInt x{-42};
        CHECK(atlas::to_underlying(x) == -42);
    }

    TEST_CASE("Large values")
    {
        test::SimpleInt x{std::numeric_limits<int>::max()};
        CHECK(atlas::to_underlying(x) == std::numeric_limits<int>::max());
    }
}
