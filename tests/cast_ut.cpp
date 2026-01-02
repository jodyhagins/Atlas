// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive tests for atlas::cast<T>()
//
// Tests:
// - Direct cast (no drilling needed)
// - Single-level drilling
// - Multi-level drilling
// - Reference casts
// - SFINAE behavior (invalid cast)
// - Constexpr compatibility
// - Non-atlas types
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include "to_underlying_test_types.hpp"

#include <string>
#include <type_traits>

// ======================================================================
// TEST SUITE: DIRECT CAST (NO DRILLING)
// ======================================================================

TEST_SUITE("atlas::cast - Direct Cast")
{
    TEST_CASE("Cast int to double")
    {
        int x = 42;
        auto result = atlas::cast<double>(x);

        CHECK(result == 42.0);
        CHECK(std::is_same<decltype(result), double>::value);
    }

    TEST_CASE("Cast double to int")
    {
        double x = 3.14;
        auto result = atlas::cast<int>(x);

        CHECK(result == 3);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Cast to same type")
    {
        int x = 42;
        auto result = atlas::cast<int>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }
}

// ======================================================================
// TEST SUITE: SINGLE LEVEL DRILLING
// ======================================================================

TEST_SUITE("atlas::cast - Single Level Drilling")
{
    TEST_CASE("Cast SimpleInt to int")
    {
        test::SimpleInt x{42};
        auto result = atlas::cast<int>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Cast SimpleInt to double")
    {
        test::SimpleInt x{42};
        auto result = atlas::cast<double>(x);

        CHECK(result == 42.0);
        CHECK(std::is_same<decltype(result), double>::value);
    }

    TEST_CASE("Cast const SimpleInt to int")
    {
        test::SimpleInt const x{42};
        auto result = atlas::cast<int>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Cast rvalue SimpleInt to int")
    {
        auto result = atlas::cast<int>(test::SimpleInt{42});

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }
}

// ======================================================================
// TEST SUITE: MULTI LEVEL DRILLING
// ======================================================================

TEST_SUITE("atlas::cast - Multi Level Drilling")
{
    TEST_CASE("Cast NestedInt to int (two levels)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        auto result = atlas::cast<int>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Cast NestedInt to double (two levels)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        auto result = atlas::cast<double>(x);

        CHECK(result == 42.0);
        CHECK(std::is_same<decltype(result), double>::value);
    }

    TEST_CASE("Cast TripleNestedInt to int (three levels)")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        auto result = atlas::cast<int>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int>::value);
    }

    TEST_CASE("Cast NestedInt to SimpleInt (one level)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        auto result = atlas::cast<test::SimpleInt>(x);

        CHECK(atlas::to_underlying(result) == 42);
        CHECK(std::is_same<decltype(result), test::SimpleInt>::value);
    }

    TEST_CASE("Cast TripleNestedInt to NestedInt (one level)")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        auto result = atlas::cast<test::NestedInt>(x);

        CHECK(atlas::to_underlying(result) == 42);
        CHECK(std::is_same<decltype(result), test::NestedInt>::value);
    }

    TEST_CASE("Cast TripleNestedInt to SimpleInt (two levels)")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        auto result = atlas::cast<test::SimpleInt>(x);

        CHECK(atlas::to_underlying(result) == 42);
        CHECK(std::is_same<decltype(result), test::SimpleInt>::value);
    }
}

// ======================================================================
// TEST SUITE: REFERENCE CASTS
// ======================================================================

TEST_SUITE("atlas::cast - Reference Casts")
{
    TEST_CASE("Cast SimpleInt to int& (mutable reference)")
    {
        test::SimpleInt x{42};
        auto & result = atlas::cast<int &>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Modify through reference
        result = 100;
        CHECK(atlas::to_underlying(x) == 100);
    }

    TEST_CASE("Cast const SimpleInt to int const&")
    {
        test::SimpleInt const x{42};
        auto const & result = atlas::cast<int const &>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int const &>::value);
    }

    TEST_CASE("Cast NestedInt to int& (two levels)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        auto & result = atlas::cast<int &>(x);

        CHECK(result == 42);
        CHECK(std::is_same<decltype(result), int &>::value);

        // Modify through reference
        result = 100;
        CHECK(atlas::to_underlying(x) == 100);
    }

    TEST_CASE("Cast NestedInt to SimpleInt& (one level)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        auto & result = atlas::cast<test::SimpleInt &>(x);

        CHECK(atlas::to_underlying(result) == 42);
        CHECK(std::is_same<decltype(result), test::SimpleInt &>::value);
    }
}

// ======================================================================
// TEST SUITE: CONSTEXPR
// ======================================================================

TEST_SUITE("atlas::cast - Constexpr")
{
    TEST_CASE("Constexpr cast with lvalue")
    {
        constexpr test::SimpleInt x{42};
        constexpr int result = atlas::cast<int>(x);

        CHECK(result == 42);
    }

    TEST_CASE("Constexpr cast with rvalue")
    {
        constexpr int result = atlas::cast<int>(test::SimpleInt{42});

        CHECK(result == 42);
    }

    TEST_CASE("Constexpr cast nested type")
    {
        constexpr test::NestedInt x{test::SimpleInt{42}};
        constexpr int result = atlas::cast<int>(x);

        CHECK(result == 42);
    }
}

// ======================================================================
// TEST SUITE: NON-ATLAS TYPES
// ======================================================================

TEST_SUITE("atlas::cast - Non-Atlas Types")
{
    TEST_CASE("Cast primitive to primitive")
    {
        int x = 42;
        auto result = atlas::cast<double>(x);

        CHECK(result == 42.0);
        CHECK(std::is_same<decltype(result), double>::value);
    }

    TEST_CASE("Cast const primitive")
    {
        int const x = 42;
        auto result = atlas::cast<double>(x);

        CHECK(result == 42.0);
    }

    TEST_CASE("Cast std::string by value")
    {
        std::string x = "hello";
        auto result = atlas::cast<std::string>(x);

        CHECK(result == "hello");
        CHECK(std::is_same<decltype(result), std::string>::value);
    }
}

// ======================================================================
// TEST SUITE: SFINAE BEHAVIOR
// ======================================================================

namespace {
// Helper to test SFINAE - returns true if cast is valid
template <typename To, typename From>
constexpr auto is_castable_impl(int)
    -> decltype(atlas::cast<To>(std::declval<From>()), std::true_type{})
{
    return {};
}

template <typename To, typename From>
constexpr std::false_type is_castable_impl(...)
{
    return {};
}

template <typename To, typename From>
constexpr bool is_castable()
{
    return decltype(is_castable_impl<To, From>(0))::value;
}

struct Unrelated {};
} // namespace

TEST_SUITE("atlas::cast - SFINAE Behavior")
{
    TEST_CASE("Valid casts are detected")
    {
        CHECK(is_castable<int, test::SimpleInt>());
        CHECK(is_castable<double, test::SimpleInt>());
        CHECK(is_castable<int, test::NestedInt>());
        CHECK(is_castable<test::SimpleInt, test::NestedInt>());
    }

    TEST_CASE("Invalid casts fail SFINAE cleanly")
    {
        // Cannot cast SimpleInt to an unrelated type
        CHECK_FALSE(is_castable<Unrelated, test::SimpleInt>());
        CHECK_FALSE(is_castable<Unrelated, test::NestedInt>());
        CHECK_FALSE(is_castable<Unrelated, int>());
    }
}
