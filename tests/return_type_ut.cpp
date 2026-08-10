// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive return type tests for atlas utilities
//
// Verifies that atlas_value_for, atlas::unwrap, atlas::undress, and
// atlas::cast<T> return the expected types for all value categories.
// Reference targets are only reachable from lvalue sources; from rvalues
// they would dangle, so those casts are verified to be rejected and value
// targets are verified to move instead.
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
// TEST SUITE: atlas_value_for RETURN TYPES
// ======================================================================

TEST_SUITE("Return Types - atlas_value_for")
{
    TEST_CASE("const lvalue returns const reference")
    {
        test::SimpleInt const x{42};
        CHECK(std::is_same<
            decltype(atlas_value_for(x)),
            int const &>::value);
    }

    TEST_CASE("mutable lvalue returns mutable reference")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas_value_for(x)),
            int &>::value);
    }

    TEST_CASE("rvalue returns by value")
    {
        CHECK(std::is_same<
            decltype(atlas_value_for(test::SimpleInt{42})),
            int>::value);
    }

    TEST_CASE("rvalue with ref_tag returns rvalue reference")
    {
        CHECK(std::is_same<
            decltype(atlas_value_for(
                test::SimpleInt{42},
                atlas::atlas_detail::ref_tag{})),
            int &&>::value);
    }
}

// ======================================================================
// TEST SUITE: atlas::unwrap RETURN TYPES
// ======================================================================

TEST_SUITE("Return Types - atlas::unwrap")
{
    TEST_CASE("SimpleInt const lvalue returns const reference")
    {
        test::SimpleInt const x{42};
        CHECK(std::is_same<
            decltype(atlas::unwrap(x)),
            int const &>::value);
    }

    TEST_CASE("SimpleInt mutable lvalue returns mutable reference")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas::unwrap(x)),
            int &>::value);
    }

    TEST_CASE("SimpleInt rvalue returns by value")
    {
        CHECK(std::is_same<
            decltype(atlas::unwrap(test::SimpleInt{42})),
            int>::value);
    }

    TEST_CASE("NestedInt const lvalue returns const reference to SimpleInt")
    {
        test::NestedInt const x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::unwrap(x)),
            test::SimpleInt const &>::value);
    }

    TEST_CASE("NestedInt mutable lvalue returns mutable reference to SimpleInt")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::unwrap(x)),
            test::SimpleInt &>::value);
    }

    TEST_CASE("NestedInt rvalue returns SimpleInt by value")
    {
        CHECK(std::is_same<
            decltype(atlas::unwrap(test::NestedInt{test::SimpleInt{42}})),
            test::SimpleInt>::value);
    }
}

// ======================================================================
// TEST SUITE: atlas::undress RETURN TYPES
// ======================================================================

TEST_SUITE("Return Types - atlas::undress")
{
    TEST_CASE("SimpleInt const lvalue returns int const&")
    {
        test::SimpleInt const x{42};
        CHECK(std::is_same<
            decltype(atlas::undress(x)),
            int const &>::value);
    }

    TEST_CASE("SimpleInt mutable lvalue returns int&")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas::undress(x)),
            int &>::value);
    }

    TEST_CASE("SimpleInt rvalue returns int by value")
    {
        CHECK(std::is_same<
            decltype(atlas::undress(test::SimpleInt{42})),
            int>::value);
    }

    TEST_CASE("NestedInt const lvalue returns int const&")
    {
        test::NestedInt const x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::undress(x)),
            int const &>::value);
    }

    TEST_CASE("NestedInt mutable lvalue returns int&")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::undress(x)),
            int &>::value);
    }

    TEST_CASE("NestedInt rvalue returns int by value")
    {
        CHECK(std::is_same<
            decltype(atlas::undress(test::NestedInt{test::SimpleInt{42}})),
            int>::value);
    }

    TEST_CASE("TripleNestedInt mutable lvalue returns int&")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        CHECK(std::is_same<
            decltype(atlas::undress(x)),
            int &>::value);
    }

    TEST_CASE("TripleNestedInt rvalue returns int by value")
    {
        CHECK(std::is_same<
            decltype(atlas::undress(
                test::TripleNestedInt{
                    test::NestedInt{test::SimpleInt{42}}})),
            int>::value);
    }
}

// ======================================================================
// TEST SUITE: atlas::cast<T> VALUE TARGETS
// ======================================================================

TEST_SUITE("Return Types - atlas::cast value targets")
{
    TEST_CASE("cast<int> from SimpleInt const lvalue returns int")
    {
        test::SimpleInt const x{42};
        CHECK(std::is_same<
            decltype(atlas::cast<int>(x)),
            int>::value);
    }

    TEST_CASE("cast<int> from SimpleInt mutable lvalue returns int")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas::cast<int>(x)),
            int>::value);
    }

    TEST_CASE("cast<int> from SimpleInt rvalue returns int")
    {
        CHECK(std::is_same<
            decltype(atlas::cast<int>(test::SimpleInt{42})),
            int>::value);
    }

    TEST_CASE("cast<int> from NestedInt mutable lvalue returns int")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<int>(x)),
            int>::value);
    }

    TEST_CASE("cast<int> from NestedInt rvalue returns int")
    {
        CHECK(std::is_same<
            decltype(atlas::cast<int>(
                test::NestedInt{test::SimpleInt{42}})),
            int>::value);
    }
}

// ======================================================================
// TEST SUITE: atlas::cast<T> REFERENCE TARGETS
// ======================================================================

TEST_SUITE("Return Types - atlas::cast reference targets")
{
    TEST_CASE("cast<int const&> from SimpleInt const lvalue")
    {
        test::SimpleInt const x{42};
        CHECK(std::is_same<
            decltype(atlas::cast<int const &>(x)),
            int const &>::value);
    }

    TEST_CASE("cast<int const&> from SimpleInt mutable lvalue")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas::cast<int const &>(x)),
            int const &>::value);
    }

    TEST_CASE("cast<int&> from SimpleInt mutable lvalue")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas::cast<int &>(x)),
            int &>::value);
    }

    TEST_CASE("cast<int&> from NestedInt mutable lvalue")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<int &>(x)),
            int &>::value);
    }

    TEST_CASE("cast<SimpleInt&> from NestedInt mutable lvalue")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<test::SimpleInt &>(x)),
            test::SimpleInt &>::value);
    }
}

// ======================================================================
// TEST SUITE: REFERENCE TARGETS REJECT RVALUE SOURCES
//
// cast<T const &> / cast<T &&> from an rvalue would return a reference
// into a value that dies at the end of the full expression, so those
// casts are ill-formed. Moving out of an expiring object is done with a
// value target, which drills by moving layer by layer.
// ======================================================================

namespace {

template <typename TargetT, typename SourceT, typename = void>
struct cast_is_valid : std::false_type
{};

template <typename TargetT, typename SourceT>
struct cast_is_valid<
    TargetT,
    SourceT,
    std::void_t<decltype(atlas::cast<TargetT>(std::declval<SourceT>()))>>
: std::true_type
{};

} // namespace

TEST_SUITE("Return Types - reference targets reject rvalue sources")
{
    TEST_CASE("cast<int&&> from rvalue is rejected, from any depth")
    {
        CHECK_FALSE(cast_is_valid<int &&, test::SimpleInt &&>::value);
        CHECK_FALSE(cast_is_valid<int &&, test::NestedInt &&>::value);
        CHECK_FALSE(cast_is_valid<int &&, test::TripleNestedInt &&>::value);
        CHECK_FALSE(
            cast_is_valid<test::SimpleInt &&, test::NestedInt &&>::value);
    }

    TEST_CASE("cast<int const&> from rvalue is rejected")
    {
        CHECK_FALSE(cast_is_valid<int const &, test::SimpleInt &&>::value);
        CHECK_FALSE(cast_is_valid<int const &, test::NestedInt &&>::value);
    }

    TEST_CASE("reference targets from lvalues remain valid")
    {
        CHECK(cast_is_valid<int const &, test::SimpleInt &>::value);
        CHECK(cast_is_valid<int &, test::SimpleInt &>::value);
        CHECK(cast_is_valid<int const &, test::SimpleInt const &>::value);
        CHECK(cast_is_valid<int &&, test::SimpleInt &>::value);
    }

    TEST_CASE("value target from rvalue moves layer by layer")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        CHECK(std::is_same<
            decltype(atlas::cast<int>(std::move(x))),
            int>::value);
        CHECK(atlas::cast<int>(std::move(x)) == 42);
    }
}
