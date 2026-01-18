// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Tests for nested Atlas type size preservation (EBO)
//
// This test verifies the key requirement from PRD-templated-strong-type-tag.md:
// Nested Atlas types must not cause size inflation.
//
// Background:
// When an Atlas type wraps another Atlas type, both inherit from
// strong_type_tag<T>. With a non-templated strong_type_tag, the compiler
// cannot apply Empty Base Optimization because two subobjects of the same
// type cannot occupy the same address. This causes size inflation.
//
// With templated strong_type_tag<T>, each type has a unique base class,
// allowing EBO to work correctly, preserving the expected sizes.
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "nested_type_size_test_types.hpp"

#include <cstdint>
#include <type_traits>

#include "doctest.hpp"

// ======================================================================
// TEST SUITE: SIZE VERIFICATION - THE KEY PRD REQUIREMENT
// ======================================================================

TEST_SUITE("Nested Atlas Type Sizes - EBO Verification")
{
    // These static_asserts run at compile time - if any fail, compilation
    // fails. This is the primary test from the PRD.

    TEST_CASE("16-bit nested types maintain expected size")
    {
        // ParticipantId wraps uint16_t - should be 2 bytes
        static_assert(
            sizeof(test::nested::ParticipantId) == sizeof(std::uint16_t),
            "ParticipantId should be same size as uint16_t");
        CHECK(sizeof(test::nested::ParticipantId) == 2);

        // SourceParticipantId wraps ParticipantId - should ALSO be 2 bytes
        // This is the KEY test from the PRD - nested types must not inflate
        static_assert(
            sizeof(test::nested::SourceParticipantId) ==
                sizeof(test::nested::ParticipantId),
            "SourceParticipantId should be same size as ParticipantId (EBO)");
        CHECK(sizeof(test::nested::SourceParticipantId) == 2);

        // Third level nesting - still 2 bytes
        static_assert(
            sizeof(test::nested::TargetSourceParticipantId) ==
                sizeof(std::uint16_t),
            "TargetSourceParticipantId should be same size as uint16_t (EBO)");
        CHECK(sizeof(test::nested::TargetSourceParticipantId) == 2);
    }

    TEST_CASE("32-bit nested types maintain expected size")
    {
        static_assert(
            sizeof(test::nested::SessionId) == sizeof(std::uint32_t),
            "SessionId should be same size as uint32_t");
        CHECK(sizeof(test::nested::SessionId) == 4);

        static_assert(
            sizeof(test::nested::ClientSessionId) ==
                sizeof(test::nested::SessionId),
            "ClientSessionId should be same size as SessionId (EBO)");
        CHECK(sizeof(test::nested::ClientSessionId) == 4);
    }

    TEST_CASE("64-bit nested types maintain expected size")
    {
        static_assert(
            sizeof(test::nested::OrderId) == sizeof(std::uint64_t),
            "OrderId should be same size as uint64_t");
        CHECK(sizeof(test::nested::OrderId) == 8);

        static_assert(
            sizeof(test::nested::MarketOrderId) ==
                sizeof(test::nested::OrderId),
            "MarketOrderId should be same size as OrderId (EBO)");
        CHECK(sizeof(test::nested::MarketOrderId) == 8);
    }

    TEST_CASE("8-bit nested types maintain expected size")
    {
        static_assert(
            sizeof(test::nested::Flag) == sizeof(std::uint8_t),
            "Flag should be same size as uint8_t");
        CHECK(sizeof(test::nested::Flag) == 1);

        static_assert(
            sizeof(test::nested::StatusFlag) == sizeof(test::nested::Flag),
            "StatusFlag should be same size as Flag (EBO)");
        CHECK(sizeof(test::nested::StatusFlag) == 1);
    }
}

// ======================================================================
// TEST SUITE: FUNCTIONAL VERIFICATION
// Ensure nested types work correctly beyond just size
// ======================================================================

TEST_SUITE("Nested Atlas Type Functionality")
{
    TEST_CASE("Construction and value access work for nested types")
    {
        // Construct nested types
        test::nested::ParticipantId inner{42};
        test::nested::SourceParticipantId outer{inner};

        // Verify values are preserved
        CHECK(atlas::undress(inner) == 42);
        CHECK(atlas::undress(outer) == 42);
    }

    TEST_CASE("Three-level nesting works correctly")
    {
        test::nested::ParticipantId p{100};
        test::nested::SourceParticipantId sp{p};
        test::nested::TargetSourceParticipantId tsp{sp};

        CHECK(atlas::undress(tsp) == 100);
    }

    TEST_CASE("atlas::unwrap works for one level of nesting")
    {
        test::nested::SourceParticipantId sp{test::nested::ParticipantId{77}};

        // unwrap should get the immediate inner type
        auto inner = atlas::unwrap(sp);
        static_assert(
            std::is_same<decltype(inner), test::nested::ParticipantId>::value,
            "unwrap should return ParticipantId");
        CHECK(atlas::undress(inner) == 77);
    }

    TEST_CASE("atlas::cast works between nested types")
    {
        test::nested::TargetSourceParticipantId tsp{
            test::nested::SourceParticipantId{test::nested::ParticipantId{99}}};

        // Cast to intermediate type
        auto sp = atlas::cast<test::nested::SourceParticipantId>(tsp);
        CHECK(atlas::undress(sp) == 99);

        // Cast to innermost Atlas type
        auto p = atlas::cast<test::nested::ParticipantId>(tsp);
        CHECK(atlas::undress(p) == 99);

        // Cast all the way to primitive
        auto val = atlas::cast<std::uint16_t>(tsp);
        CHECK(val == 99);
    }

    TEST_CASE("is_atlas_type trait works for nested types")
    {
        static_assert(
            atlas::is_atlas_type<test::nested::ParticipantId>::value,
            "ParticipantId should be an atlas type");
        static_assert(
            atlas::is_atlas_type<test::nested::SourceParticipantId>::value,
            "SourceParticipantId should be an atlas type");
        static_assert(
            atlas::is_atlas_type<
                test::nested::TargetSourceParticipantId>::value,
            "TargetSourceParticipantId should be an atlas type");

        CHECK(atlas::is_atlas_type<test::nested::ParticipantId>::value);
        CHECK(atlas::is_atlas_type<test::nested::SourceParticipantId>::value);
        CHECK(atlas::is_atlas_type<
              test::nested::TargetSourceParticipantId>::value);
    }
}
