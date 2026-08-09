// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Tests for the public atlas type-level traits and concepts:
//
// - is_atlas_type / AtlasTypeC - is this an atlas type at all
// - unwrap_t - one layer removed, SFINAE-friendly
// - undress_t - root type, identity for non-atlas non-enum types
// - is_atlas_of / AtlasOfC - atlas type with a specific root
// - is_string_atlas / StringAtlasC - root is std::string
// - is_arithmetic_atlas / ArithmeticAtlasC - root is arithmetic, not bool
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
} // namespace test

#include "undress_test_types.hpp"

#include <string>
#include <type_traits>

#include "doctest.hpp"

namespace {

// Detection idiom used to prove unwrap_t is SFINAE-friendly rather than a
// hard error for types that have no layer to remove.
template <typename T, typename = void>
struct has_unwrap_t
: std::false_type
{ };

template <typename T>
struct has_unwrap_t<T, std::void_t<atlas::unwrap_t<T>>>
: std::true_type
{ };

template <typename T>
constexpr bool same_as_int = std::is_same_v<T, int>;

// If the category concepts did not subsume AtlasTypeC these overloads would be
// ambiguous rather than resolving to the most constrained candidate.
template <atlas::AtlasTypeC T>
int
which(T const &)
{
    return 1;
}

template <atlas::StringAtlasC T>
int
which(T const &)
{
    return 2;
}

template <atlas::ArithmeticAtlasC T>
int
which(T const &)
{
    return 3;
}

} // namespace

TEST_CASE("atlas::is_atlas_type")
{
    SUBCASE("true for generated types")
    {
        CHECK(atlas::is_atlas_type<test::SimpleInt>::value);
        CHECK(atlas::is_atlas_type<test::NestedInt>::value);
        CHECK(atlas::is_atlas_type<test::MovableString>::value);
    }

    SUBCASE("ignores cv-qualification and references")
    {
        CHECK(atlas::is_atlas_type<test::SimpleInt const>::value);
        CHECK(atlas::is_atlas_type<test::SimpleInt &>::value);
        CHECK(atlas::is_atlas_type<test::SimpleInt const &>::value);
        CHECK(atlas::is_atlas_type<test::SimpleInt &&>::value);
    }

    SUBCASE("false for raw types and enums")
    {
        CHECK_FALSE(atlas::is_atlas_type<int>::value);
        CHECK_FALSE(atlas::is_atlas_type<std::string>::value);
        CHECK_FALSE(atlas::is_atlas_type<test::Color>::value);
    }

    SUBCASE("AtlasTypeC agrees with the trait")
    {
        static_assert(atlas::AtlasTypeC<test::SimpleInt>);
        static_assert(atlas::AtlasTypeC<test::SimpleInt const &>);
        static_assert(not atlas::AtlasTypeC<int>);
        static_assert(not atlas::AtlasTypeC<test::Color>);
    }
}

TEST_CASE("atlas::unwrap_t")
{
    SUBCASE("removes exactly one layer")
    {
        static_assert(std::is_same_v<atlas::unwrap_t<test::SimpleInt>, int>);
        static_assert(
            std::is_same_v<atlas::unwrap_t<test::NestedInt>, test::SimpleInt>);
        static_assert(std::is_same_v<
            atlas::unwrap_t<test::TripleNestedInt>,
            test::NestedInt>);
    }

    SUBCASE("strips cv-qualification and references from the source")
    {
        static_assert(
            std::is_same_v<atlas::unwrap_t<test::SimpleInt const &>, int>);
        static_assert(
            std::is_same_v<atlas::unwrap_t<test::SimpleInt &&>, int>);
    }

    SUBCASE("drills an enum to its underlying type")
    {
        static_assert(std::is_same_v<atlas::unwrap_t<test::Color>, int>);
    }

    SUBCASE("mirrors atlas::unwrap - one layer only for wrapped enums")
    {
        static_assert(std::is_same_v<
            atlas::unwrap_t<test::WrappedColor>,
            test::Color>);
    }

    SUBCASE("is SFINAE-friendly for types with no layer to remove")
    {
        CHECK(has_unwrap_t<test::SimpleInt>::value);
        CHECK(has_unwrap_t<test::Color>::value);
        CHECK_FALSE(has_unwrap_t<int>::value);
        CHECK_FALSE(has_unwrap_t<std::string>::value);
    }
}

TEST_CASE("atlas::undress_t")
{
    SUBCASE("drills through every layer")
    {
        static_assert(std::is_same_v<atlas::undress_t<test::SimpleInt>, int>);
        static_assert(std::is_same_v<atlas::undress_t<test::NestedInt>, int>);
        static_assert(
            std::is_same_v<atlas::undress_t<test::TripleNestedInt>, int>);
    }

    SUBCASE("is the identity for non-atlas, non-enum types")
    {
        static_assert(std::is_same_v<atlas::undress_t<int>, int>);
        static_assert(
            std::is_same_v<atlas::undress_t<std::string>, std::string>);
    }

    SUBCASE("strips cv-qualification and references")
    {
        static_assert(
            std::is_same_v<atlas::undress_t<test::TripleNestedInt const &>, int>);
        static_assert(std::is_same_v<atlas::undress_t<int const &>, int>);
    }

    SUBCASE("reaches the underlying type of a wrapped enum")
    {
        static_assert(std::is_same_v<atlas::undress_t<test::Color>, int>);
        static_assert(
            std::is_same_v<atlas::undress_t<test::WrappedColor>, int>);
        static_assert(
            std::is_same_v<atlas::undress_t<test::NestedWrappedColor>, int>);
    }

    SUBCASE("reaches std::string through nesting")
    {
        static_assert(std::is_same_v<
            atlas::undress_t<test::MovableString>,
            std::string>);
        static_assert(std::is_same_v<
            atlas::undress_t<test::NestedString>,
            std::string>);
    }

    SUBCASE("agrees with the value returned by atlas::undress")
    {
        test::TripleNestedInt x{test::NestedInt{test::SimpleInt{42}}};
        static_assert(same_as_int<
            std::remove_cvref_t<decltype(atlas::undress(x))>>);
        CHECK(atlas::undress(x) == 42);
    }
}

TEST_CASE("atlas::is_atlas_of and AtlasOfC")
{
    SUBCASE("matches the root type, not an intermediate layer")
    {
        CHECK(atlas::is_atlas_of<test::TripleNestedInt, int>::value);
        CHECK_FALSE(
            atlas::is_atlas_of<test::TripleNestedInt, test::NestedInt>::value);
    }

    SUBCASE("false when the source is not an atlas type")
    {
        CHECK_FALSE(atlas::is_atlas_of<int, int>::value);
        CHECK_FALSE(atlas::is_atlas_of<test::Color, int>::value);
    }

    SUBCASE("concept agrees with the trait")
    {
        static_assert(atlas::AtlasOfC<test::TripleNestedInt, int>);
        static_assert(atlas::AtlasOfC<test::MovableString, std::string>);
        static_assert(not atlas::AtlasOfC<int, int>);
    }
}

TEST_CASE("atlas::is_string_atlas and StringAtlasC")
{
    SUBCASE("true for direct and nested string wrappers")
    {
        CHECK(atlas::is_string_atlas<test::MovableString>::value);
        CHECK(atlas::is_string_atlas<test::NestedString>::value);
        static_assert(atlas::StringAtlasC<test::MovableString>);
        static_assert(atlas::StringAtlasC<test::NestedString>);
    }

    SUBCASE("false for raw std::string and for non-string wrappers")
    {
        CHECK_FALSE(atlas::is_string_atlas<std::string>::value);
        CHECK_FALSE(atlas::is_string_atlas<test::SimpleInt>::value);
        static_assert(not atlas::StringAtlasC<std::string>);
        static_assert(not atlas::StringAtlasC<test::SimpleInt>);
    }
}

TEST_CASE("atlas::is_arithmetic_atlas and ArithmeticAtlasC")
{
    SUBCASE("true for integral and floating point roots")
    {
        CHECK(atlas::is_arithmetic_atlas<test::SimpleInt>::value);
        CHECK(atlas::is_arithmetic_atlas<test::TripleNestedInt>::value);
        CHECK(atlas::is_arithmetic_atlas<test::Ratio>::value);
        static_assert(atlas::ArithmeticAtlasC<test::SimpleInt>);
        static_assert(atlas::ArithmeticAtlasC<test::TripleNestedInt>);
        static_assert(atlas::ArithmeticAtlasC<test::Ratio>);
    }

    SUBCASE("bool is excluded")
    {
        CHECK(std::is_arithmetic_v<bool>);
        CHECK_FALSE(atlas::is_arithmetic_atlas<test::BoolFlag>::value);
        static_assert(not atlas::ArithmeticAtlasC<test::BoolFlag>);
    }

    SUBCASE("false for non-arithmetic roots and for raw types")
    {
        CHECK_FALSE(atlas::is_arithmetic_atlas<test::MovableString>::value);
        CHECK_FALSE(atlas::is_arithmetic_atlas<int>::value);
        static_assert(not atlas::ArithmeticAtlasC<test::MovableString>);
        static_assert(not atlas::ArithmeticAtlasC<int>);
    }

    SUBCASE("a wrapped enum is not arithmetic - it is not an atlas root")
    {
        // WrappedColor undresses to int, so it IS arithmetic. This documents
        // that enums drill through rather than blocking the trait.
        CHECK(atlas::is_arithmetic_atlas<test::WrappedColor>::value);
    }
}

TEST_CASE("category concepts subsume AtlasTypeC")
{
    CHECK(which(test::MovableString{"hello"}) == 2);
    CHECK(which(test::SimpleInt{42}) == 3);
    CHECK(which(test::MoveOnlyWrapper{}) == 1);
}
