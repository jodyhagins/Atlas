// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive tests for atlas::unwrap()
//
// Tests that unwrap removes exactly ONE layer:
// - Atlas types: returns atlas_value_type (not the innermost type)
// - Enums: returns underlying_type_t
// - Non-atlas/non-enum types: SFINAE fails (does not compile)
//
// Contrast with atlas::undress() which recursively drills to the raw type.
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

// Define test enum BEFORE including generated types (they reference it)
namespace test {
enum class Color : int { Red = 1, Green = 2, Blue = 3 };
}

#include "undress_test_types.hpp"

#include <memory>
#include <string>
#include <type_traits>

#include "doctest.hpp"

// Test enums for unwrap
enum class ScopedColor : int { Red = 1, Green = 2, Blue = 3 };
enum UnscopedSize : short { Small = 10, Medium = 20, Large = 30 };

// Detection idiom for SFINAE tests
template <typename T, typename = void>
struct is_unwrappable : std::false_type
{
};

template <typename T>
struct is_unwrappable<
    T,
    decltype(atlas::unwrap(std::declval<T &>()), void())> : std::true_type
{
};

// ======================================================================
// TEST SUITE: BASIC VALUE CATEGORIES
// ======================================================================

TEST_SUITE("atlas::unwrap - Value Categories")
{
    TEST_CASE("Lvalue const reference returns const reference")
    {
        test::SimpleInt const x{42};
        auto const & result = atlas::unwrap(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int const &>::value);
    }

    TEST_CASE("Lvalue non-const reference returns non-const reference")
    {
        test::SimpleInt x{42};
        auto & result = atlas::unwrap(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Can modify through the reference
        result = 100;
        CHECK(atlas::unwrap(x) == 100);
    }

    TEST_CASE("Rvalue returns by value for moveable types")
    {
        auto result = atlas::unwrap(test::SimpleInt{42});

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Rvalue with moveable underlying type moves correctly")
    {
        auto result = atlas::unwrap(test::MovableString{"hello"});

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string>::value);
    }
}

// ======================================================================
// TEST SUITE: SINGLE LAYER ONLY (key difference from undress)
// ======================================================================

TEST_SUITE("atlas::unwrap - Single Layer Only")
{
    TEST_CASE("Single level nesting extracts underlying value")
    {
        test::SimpleInt x{42};
        auto & result = atlas::unwrap(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);
    }

    TEST_CASE("Double level nesting stops at first layer")
    {
        test::NestedInt x{test::SimpleInt{42}};

        auto & result = atlas::unwrap(x);

        // Should return SimpleInt&, NOT int&
        CHECK(std::is_same<decltype(result), test::SimpleInt &>::value);

        // Can still access the inner value via another unwrap or undress
        CHECK(atlas::unwrap(result) == 42);
        CHECK(atlas::undress(x) == 42);
    }

    TEST_CASE("Triple level nesting stops at first layer")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};

        auto & result = atlas::unwrap(x);

        // Should return NestedInt&, NOT SimpleInt& or int&
        CHECK(std::is_same<decltype(result), test::NestedInt &>::value);

        // Chain unwrap to get to deeper layers
        auto & second = atlas::unwrap(result);
        CHECK(std::is_same<decltype(second), test::SimpleInt &>::value);

        auto & third = atlas::unwrap(second);
        CHECK(std::is_same<decltype(third), int &>::value);
        CHECK(third == 42);
    }

    TEST_CASE("Comparison with undress - shows the difference")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};

        // unwrap: one layer at a time
        auto & unwrapped = atlas::unwrap(x);
        CHECK(std::is_same<decltype(unwrapped), test::NestedInt &>::value);

        // undress: all the way down
        auto & undressed = atlas::undress(x);
        CHECK(std::is_same<decltype(undressed), int &>::value);
        CHECK(undressed == 42);
    }
}

// ======================================================================
// TEST SUITE: ENUM HANDLING
// ======================================================================

TEST_SUITE("atlas::unwrap - Enums")
{
    TEST_CASE("Scoped enum returns underlying type")
    {
        auto result = atlas::unwrap(ScopedColor::Red);

        CHECK(result == 1);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Scoped enum with different values")
    {
        CHECK(atlas::unwrap(ScopedColor::Green) == 2);
        CHECK(atlas::unwrap(ScopedColor::Blue) == 3);
    }

    TEST_CASE("Unscoped enum returns underlying type")
    {
        auto result = atlas::unwrap(Small);

        CHECK(result == 10);
        CHECK(std::is_same<decltype(result), short>::value);
    }

    TEST_CASE("Unscoped enum with different values")
    {
        CHECK(atlas::unwrap(Medium) == 20);
        CHECK(atlas::unwrap(Large) == 30);
    }
}

// ======================================================================
// TEST SUITE: SFINAE (non-atlas/non-enum types should fail)
// ======================================================================

TEST_SUITE("atlas::unwrap - SFINAE")
{
    TEST_CASE("Atlas types are unwrappable")
    {
        CHECK(is_unwrappable<test::SimpleInt>::value);
        CHECK(is_unwrappable<test::NestedInt>::value);
        CHECK(is_unwrappable<test::TripleNestedInt>::value);
        CHECK(is_unwrappable<test::MovableString>::value);
    }

    TEST_CASE("Enums are unwrappable")
    {
        CHECK(is_unwrappable<ScopedColor>::value);
        CHECK(is_unwrappable<UnscopedSize>::value);
    }

    TEST_CASE("Non-atlas non-enum types are NOT unwrappable")
    {
        CHECK_FALSE(is_unwrappable<int>::value);
        CHECK_FALSE(is_unwrappable<double>::value);
        CHECK_FALSE(is_unwrappable<std::string>::value);
        CHECK_FALSE(is_unwrappable<std::unique_ptr<int>>::value);
    }
}

// ======================================================================
// TEST SUITE: CONSTEXPR
// ======================================================================

TEST_SUITE("atlas::unwrap - Constexpr")
{
    TEST_CASE("Works in constexpr context with lvalue")
    {
        constexpr test::SimpleInt x{42};
        constexpr int result = atlas::unwrap(x);

        CHECK(result == 42);
    }

    TEST_CASE("Works in constexpr context with rvalue")
    {
        constexpr int result = atlas::unwrap(test::SimpleInt{42});

        CHECK(result == 42);
    }

    TEST_CASE("Enum works in constexpr context")
    {
        constexpr int result = atlas::unwrap(ScopedColor::Blue);

        CHECK(result == 3);
    }
}

// ======================================================================
// TEST SUITE: MOVE-ONLY TYPES
// ======================================================================

TEST_SUITE("atlas::unwrap - Move-Only Types")
{
    TEST_CASE("Lvalue returns reference (no move needed)")
    {
        test::MoveOnlyWrapper x{std::make_unique<int>(42)};

        auto & result = atlas::unwrap(x);

        CHECK(*result == 42);
        CHECK(std::is_same<decltype(result), std::unique_ptr<int> &>::value);
    }

    TEST_CASE("Const lvalue returns const reference")
    {
        test::MoveOnlyWrapper const x{std::make_unique<int>(42)};

        auto const & result = atlas::unwrap(x);

        CHECK(*result == 42);
        CHECK(std::is_same<decltype(result), std::unique_ptr<int> const &>::
                  value);
    }
}

// ======================================================================
// TEST SUITE: EDGE CASES
// ======================================================================

TEST_SUITE("atlas::unwrap - Edge Cases")
{
    TEST_CASE("Default constructed value")
    {
        test::SimpleInt x{};
        CHECK(atlas::unwrap(x) == 0);
    }

    TEST_CASE("Negative values")
    {
        test::SimpleInt x{-42};
        CHECK(atlas::unwrap(x) == -42);
    }

    TEST_CASE("Large values")
    {
        test::SimpleInt x{std::numeric_limits<int>::max()};
        CHECK(atlas::unwrap(x) == std::numeric_limits<int>::max());
    }

    TEST_CASE("Can modify through unwrap reference")
    {
        test::NestedInt x{test::SimpleInt{42}};

        // Get reference to SimpleInt
        auto & inner = atlas::unwrap(x);
        CHECK(std::is_same<decltype(inner), test::SimpleInt &>::value);

        // Modify the inner SimpleInt's value
        atlas::unwrap(inner) = 100;

        // Verify the change propagated
        CHECK(atlas::undress(x) == 100);
    }
}
