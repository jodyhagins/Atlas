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
// This test specifically validates the ref_tag mechanism that allows
// atlas::cast<T&&> to return references to live storage rather than
// dangling references to temporaries.
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

    TEST_CASE("cast<int&&> from SimpleInt rvalue")
    {
        test::SimpleInt x{42};
        CHECK(std::is_same<
            decltype(atlas::cast<int &&>(std::move(x))),
            int &&>::value);
    }

    TEST_CASE("cast<int&> from NestedInt mutable lvalue")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<int &>(x)),
            int &>::value);
    }

    TEST_CASE("cast<int&&> from NestedInt rvalue")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<int &&>(std::move(x))),
            int &&>::value);
    }

    TEST_CASE("cast<int&&> from TripleNestedInt rvalue")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        CHECK(std::is_same<
            decltype(atlas::cast<int &&>(std::move(x))),
            int &&>::value);
    }

    TEST_CASE("cast<SimpleInt&> from NestedInt mutable lvalue")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<test::SimpleInt &>(x)),
            test::SimpleInt &>::value);
    }

    TEST_CASE("cast<SimpleInt&&> from NestedInt rvalue")
    {
        test::NestedInt x{test::SimpleInt{42}};
        CHECK(std::is_same<
            decltype(atlas::cast<test::SimpleInt &&>(std::move(x))),
            test::SimpleInt &&>::value);
    }
}

// ======================================================================
// TEST SUITE: NO-DANGLING VERIFICATION
// ======================================================================

TEST_SUITE("Return Types - no-dangling verification")
{
    TEST_CASE("reference cast points to live member storage (SimpleInt)")
    {
        test::SimpleInt x{42};
        int * addr = &atlas::undress(x);
        int && ref = atlas::cast<int &&>(std::move(x));
        CHECK(&ref == addr);
    }

    TEST_CASE("reference cast points to live member storage (NestedInt)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        int * addr = &atlas::undress(x);
        int && ref = atlas::cast<int &&>(std::move(x));
        CHECK(&ref == addr);
    }

    TEST_CASE("reference cast points to live member storage (TripleNestedInt)")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        int * addr = &atlas::undress(x);
        int && ref = atlas::cast<int &&>(std::move(x));
        CHECK(&ref == addr);
    }

    TEST_CASE("intermediate level reference cast (NestedInt to SimpleInt&&)")
    {
        test::NestedInt x{test::SimpleInt{42}};
        test::SimpleInt * addr = &atlas::cast<test::SimpleInt &>(x);
        test::SimpleInt && ref = atlas::cast<test::SimpleInt &&>(std::move(x));
        CHECK(&ref == addr);
    }
}
