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

// Define test enum BEFORE including generated types (they reference it)
namespace test {
enum class Color : int
{
    Red = 1,
    Green = 2,
    Blue = 3
};
}

#include "undress_test_types.hpp"

#include <string>
#include <type_traits>

#include "doctest.hpp"

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

        CHECK(atlas::undress(result) == 42);
        CHECK(std::is_same<decltype(result), test::SimpleInt>::value);
    }

    TEST_CASE("Cast TripleNestedInt to NestedInt (one level)")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        auto result = atlas::cast<test::NestedInt>(x);

        CHECK(atlas::undress(result) == 42);
        CHECK(std::is_same<decltype(result), test::NestedInt>::value);
    }

    TEST_CASE("Cast TripleNestedInt to SimpleInt (two levels)")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        auto result = atlas::cast<test::SimpleInt>(x);

        CHECK(atlas::undress(result) == 42);
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
        CHECK(atlas::undress(x) == 100);
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
        CHECK(atlas::undress(x) == 100);
    }

    TEST_CASE("Cast NestedInt to SimpleInt& (one level)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        auto & result = atlas::cast<test::SimpleInt &>(x);

        CHECK(atlas::undress(result) == 42);
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
// TEST SUITE: TYPE WITH EXPLICIT CAST OPERATOR
// ======================================================================

namespace {
// A type unrelated to the wrapped int - used to prove cast tries the type first
struct UnrelatedTarget
{
    int marker;

    constexpr explicit UnrelatedTarget(int m)
    : marker{m}
    { }
};

// An atlas-like type that wraps int but also has a cast operator to
// UnrelatedTarget
// This proves atlas::cast tries the type itself before drilling down
struct TypeWithCastOperator
: private atlas::strong_type_tag<TypeWithCastOperator>
{
    int value;
    using atlas_value_type = int;

    constexpr explicit TypeWithCastOperator(int v)
    : value{v}
    { }

    // Cast operator to UnrelatedTarget - uses a DIFFERENT value to prove
    // we used this operator rather than drilling to the int
    constexpr explicit operator UnrelatedTarget () const
    {
        return UnrelatedTarget{value * 1000 + 42};
    }

    // Hidden friend for atlas machinery
    friend constexpr int const & atlas_value_for(
        TypeWithCastOperator const & self) noexcept
    {
        return self.value;
    }

    friend constexpr int & atlas_value_for(TypeWithCastOperator & self) noexcept
    {
        return self.value;
    }
};
} // anonymous namespace

TEST_SUITE("atlas::cast - Type With Cast Operator")
{
    TEST_CASE("Cast uses type's own cast operator before drilling")
    {
        TypeWithCastOperator x{7};

        // Cast to UnrelatedTarget should use the type's cast operator
        // NOT drill down to int (which can't cast to UnrelatedTarget)
        auto result = atlas::cast<UnrelatedTarget>(x);

        // The cast operator returns value * 1000 + 42
        // So for value=7, we expect 7042
        CHECK(result.marker == 7042);
    }

    TEST_CASE("Cast still drills when type itself can't cast")
    {
        TypeWithCastOperator x{7};

        // Cast to int - the type itself can't be directly cast to int
        // (no operator int()), so it drills down to the wrapped int
        auto result = atlas::cast<int>(x);

        CHECK(result == 7);
    }

    TEST_CASE("Cast to double drills through to wrapped int")
    {
        TypeWithCastOperator x{7};

        // Type has no operator double(), so drills to int, then casts to double
        auto result = atlas::cast<double>(x);

        CHECK(result == 7.0);
    }
}

// ======================================================================
// TEST SUITE: SFINAE BEHAVIOR
// ======================================================================

namespace {
// Helper to test SFINAE - returns true if cast is valid
template <typename To, typename From>
constexpr auto
is_castable_impl(int)
-> decltype(atlas::cast<To>(std::declval<From>()), std::true_type{})
{
    return {};
}

template <typename To, typename From>
constexpr std::false_type
is_castable_impl(...)
{
    return {};
}

template <typename To, typename From>
constexpr bool
is_castable()
{
    return decltype(is_castable_impl<To, From>(0))::value;
}

struct Unrelated
{ };
} // anonymous namespace

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
