// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// C++20-Specific atlas::Nilable Tests
//
// This test suite specifically tests C++20 features:
// - Spaceship operator support
// - Fallback to < when spaceship not available
// - SFINAE disabling of operators when T doesn't support them
// - Mixed comparisons between different operator sets
//
// These tests are conditionally compiled based on C++20 availability.
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "nilable_cpp20_test_types.hpp"

#include <optional>
#include <type_traits>

#include "doctest.hpp"

// ======================================================================
// TEST SUITE: SPACESHIP OPERATOR SUPPORT
// ======================================================================

#if defined(__cpp_impl_three_way_comparison) && \
    (__cpp_impl_three_way_comparison >= 201907)

TEST_SUITE("Nilable Spaceship Operators (C++20)")
{
    TEST_CASE("Types with spaceship use spaceship comparison")
    {
        atlas::Nilable<test::WithSpaceship> opt1(test::WithSpaceship{10});
        atlas::Nilable<test::WithSpaceship> opt2(test::WithSpaceship{20});

        // Spaceship should work
        CHECK((opt1 <=> opt2) < 0);
        CHECK((opt2 <=> opt1) > 0);
        CHECK((opt1 <=> opt1) == 0);

        // Synthesized operators should work
        CHECK(opt1 < opt2);
        CHECK(opt1 <= opt2);
        CHECK(opt2 > opt1);
        CHECK(opt2 >= opt1);
        CHECK(opt1 <= opt1);
        CHECK(opt1 >= opt1);
    }

    TEST_CASE("Empty optionals compare correctly with spaceship")
    {
        atlas::Nilable<test::WithSpaceship> empty1;
        atlas::Nilable<test::WithSpaceship> empty2;
        atlas::Nilable<test::WithSpaceship> full(test::WithSpaceship{42});

        CHECK((empty1 <=> empty2) == 0);
        CHECK((empty1 <=> full) < 0);
        CHECK((full <=> empty1) > 0);
    }

    TEST_CASE("Spaceship with nullopt")
    {
        atlas::Nilable<test::WithSpaceship> empty;
        atlas::Nilable<test::WithSpaceship> full(test::WithSpaceship{42});

        CHECK((empty <=> std::nullopt) == 0);
        CHECK((full <=> std::nullopt) > 0);
        CHECK((std::nullopt <=> empty) == 0);
        CHECK((std::nullopt <=> full) < 0);
    }

    TEST_CASE("Spaceship with value")
    {
        atlas::Nilable<test::WithSpaceship> opt(test::WithSpaceship{42});

        CHECK((opt <=> test::WithSpaceship{42}) == 0);
        CHECK((opt <=> test::WithSpaceship{10}) > 0);
        CHECK((opt <=> test::WithSpaceship{50}) < 0);
    }

    TEST_CASE("Spaceship with different sentinel values")
    {
        atlas::Nilable<test::SpaceshipNegative> opt1(
            test::SpaceshipNegative{10});
        atlas::Nilable<test::SpaceshipNegative> opt2(
            test::SpaceshipNegative{20});
        atlas::Nilable<test::SpaceshipNegative> empty;

        CHECK((opt1 <=> opt2) < 0);
        CHECK((empty <=> opt1) < 0);
    }

    TEST_CASE("Spaceship with string type")
    {
        atlas::Nilable<test::SpaceshipString> opt1(
            test::SpaceshipString{std::string("apple")});
        atlas::Nilable<test::SpaceshipString> opt2(
            test::SpaceshipString{std::string("banana")});
        atlas::Nilable<test::SpaceshipString> empty;

        CHECK((opt1 <=> opt2) < 0);
        CHECK((opt2 <=> opt1) > 0);
        CHECK((empty <=> opt1) < 0);
    }

    TEST_CASE("Spaceship with constrained type")
    {
        atlas::Nilable<test::SpaceshipBounded> opt1(test::SpaceshipBounded{25});
        atlas::Nilable<test::SpaceshipBounded> opt2(test::SpaceshipBounded{75});

        CHECK((opt1 <=> opt2) < 0);
        CHECK((opt2 <=> opt1) > 0);
    }
}

#endif // C++20 spaceship

// ======================================================================
// TEST SUITE: FALLBACK TO < IN C++20
// ======================================================================

TEST_SUITE("Optional < Fallback (All C++ versions)")
{
    TEST_CASE("Types with only < work in C++20")
    {
        atlas::Nilable<test::OnlyLessThan> opt1(test::OnlyLessThan{10});
        atlas::Nilable<test::OnlyLessThan> opt2(test::OnlyLessThan{20});

        // operator< should work
        CHECK(opt1 < opt2);
        CHECK_FALSE(opt2 < opt1);
        CHECK_FALSE(opt1 < opt1);

        // Derived operators should work
        CHECK(opt1 <= opt2);
        CHECK(opt2 > opt1);
        CHECK(opt2 >= opt1);
        CHECK(opt1 <= opt1);
        CHECK(opt1 >= opt1);
    }

    TEST_CASE("Empty optionals with < only")
    {
        atlas::Nilable<test::OnlyLessThan> empty1;
        atlas::Nilable<test::OnlyLessThan> empty2;
        atlas::Nilable<test::OnlyLessThan> full(test::OnlyLessThan{42});

        CHECK_FALSE(empty1 < empty2);
        CHECK(empty1 < full);
        CHECK_FALSE(full < empty1);
    }

    TEST_CASE("Equality with types that have <")
    {
        atlas::Nilable<test::OnlyLessThan> opt1(test::OnlyLessThan{42});
        atlas::Nilable<test::OnlyLessThan> opt2(test::OnlyLessThan{42});
        atlas::Nilable<test::OnlyLessThan> opt3(test::OnlyLessThan{17});

        CHECK(opt1 == opt2);
        CHECK_FALSE(opt1 == opt3);
    }
}

// ======================================================================
// TEST SUITE: SFINAE FOR MISSING OPERATORS
// ======================================================================

TEST_SUITE("Optional SFINAE for Missing Operators")
{
    TEST_CASE("Types with only == work for equality")
    {
        atlas::Nilable<test::OnlyEquality> opt1(test::OnlyEquality{42});
        atlas::Nilable<test::OnlyEquality> opt2(test::OnlyEquality{42});
        atlas::Nilable<test::OnlyEquality> opt3(test::OnlyEquality{17});
        atlas::Nilable<test::OnlyEquality> empty;

        // Equality should work
        CHECK(opt1 == opt2);
        CHECK_FALSE(opt1 == opt3);
        CHECK_FALSE(opt1 == empty);
        CHECK(empty == empty);

        // These should NOT compile (operator< not available)
        // Uncommenting these should cause compilation errors:
        // CHECK(opt1 < opt2);  // Should fail - OnlyEquality has no <
        // CHECK(opt1 <= opt2); // Should fail
    }

    TEST_CASE("Types with no comparison operators")
    {
        atlas::Nilable<test::NoComparison> opt1(test::NoComparison{42});
        atlas::Nilable<test::NoComparison> opt2(test::NoComparison{42});
        atlas::Nilable<test::NoComparison> empty;

        // Only has_value and reset should work
        CHECK(opt1.has_value());
        CHECK_FALSE(empty.has_value());

        opt1.reset();
        CHECK_FALSE(opt1.has_value());

        // These should NOT compile (no comparison operators)
        // Uncommenting these should cause compilation errors:
        // CHECK(opt1 == opt2);  // Should fail - NoComparison has no ==
        // CHECK(opt1 < opt2);   // Should fail - NoComparison has no <
    }
}

// ======================================================================
// TEST SUITE: MIXED COMPARISONS
// ======================================================================

#if defined(__cpp_impl_three_way_comparison) && \
    (__cpp_impl_three_way_comparison >= 201907)

TEST_SUITE("Mixed Comparisons (C++20)")
{
    TEST_CASE("Cannot compare Optional<Spaceship> with Optional<OnlyLessThan>")
    {
        atlas::Nilable<test::WithSpaceship> spaceship_opt(
            test::WithSpaceship{42});
        atlas::Nilable<test::OnlyLessThan> lessthan_opt(test::OnlyLessThan{42});

        // These should NOT compile (different types)
        // They're not comparable because the underlying types are different
        // Uncommenting should cause compilation error:
        // CHECK(spaceship_opt == lessthan_opt);  // Should fail - different
        // types CHECK(spaceship_opt < lessthan_opt);   // Should fail -
        // different types

        // But each can compare with itself
        CHECK(spaceship_opt == spaceship_opt);
        CHECK(lessthan_opt == lessthan_opt);
    }

    TEST_CASE("Spaceship and < both work on same Optional type")
    {
        // WithSpaceship has spaceship, so it should use that
        atlas::Nilable<test::WithSpaceship> opt1(test::WithSpaceship{10});
        atlas::Nilable<test::WithSpaceship> opt2(test::WithSpaceship{20});

        // Both should work, but spaceship is preferred in C++20
        auto spaceship_result = opt1 <=> opt2;
        CHECK(spaceship_result < 0);

        // operator< still works (synthesized from spaceship in C++20)
        CHECK(opt1 < opt2);
    }

    TEST_CASE("Only < works for OnlyLessThan in C++20")
    {
        atlas::Nilable<test::OnlyLessThan> opt1(test::OnlyLessThan{10});
        atlas::Nilable<test::OnlyLessThan> opt2(test::OnlyLessThan{20});

        // operator< works (explicit operator)
        CHECK(opt1 < opt2);

        // operator<=> should NOT be available
        // This should fail to compile if uncommented:
        // auto result = opt1 <=> opt2;  // Should fail - OnlyLessThan has no
        // <=>
    }
}

#endif // C++20 mixed comparisons

// ======================================================================
// TEST SUITE: TYPE TRAIT VERIFICATION
// ======================================================================

TEST_SUITE("Optional Type Traits")
{
    TEST_CASE("can_be_nilable detects types correctly")
    {
        CHECK(atlas::can_be_nilable<test::WithSpaceship>::value);
        CHECK(atlas::can_be_nilable<test::OnlyLessThan>::value);
        CHECK(atlas::can_be_nilable<test::OnlyEquality>::value);
        CHECK(atlas::can_be_nilable<test::NoComparison>::value);

        // All have nil_value, so all can be optional
        CHECK(atlas::can_be_nilable<test::SpaceshipNegative>::value);
        CHECK(atlas::can_be_nilable<test::SpaceshipString>::value);
        CHECK(atlas::can_be_nilable<test::SpaceshipBounded>::value);
    }

#if defined(__cpp_impl_three_way_comparison) && \
    (__cpp_impl_three_way_comparison >= 201907)

    TEST_CASE("Spaceship detection via std::three_way_comparable_with")
    {
        // WithSpaceship should be three-way comparable
        CHECK(std::three_way_comparable<test::WithSpaceship>);

        // OnlyLessThan should NOT be three-way comparable
        CHECK_FALSE(std::three_way_comparable<test::OnlyLessThan>);
    }

#endif
}

// ======================================================================
// TEST SUITE: INTEROP WITH STD::OPTIONAL
// ======================================================================

TEST_SUITE("Interop with std::optional")
{
    TEST_CASE("Compare atlas::Nilable with std::optional (spaceship types)")
    {
        atlas::Nilable<test::WithSpaceship> atlas_opt(test::WithSpaceship{42});
        std::optional<test::WithSpaceship> std_opt(test::WithSpaceship{42});

        CHECK(atlas_opt == std_opt);
        CHECK(std_opt == atlas_opt);

#if defined(__cpp_impl_three_way_comparison) && \
    (__cpp_impl_three_way_comparison >= 201907)
        CHECK((atlas_opt <=> std_opt) == 0);
        CHECK((std_opt <=> atlas_opt) == 0);
#endif
    }

    TEST_CASE("Compare atlas::Nilable with std::optional (< only types)")
    {
        atlas::Nilable<test::OnlyLessThan> atlas_opt(test::OnlyLessThan{42});
        std::optional<test::OnlyLessThan> std_opt(test::OnlyLessThan{42});

        CHECK(atlas_opt == std_opt);
        CHECK(std_opt == atlas_opt);

        CHECK_FALSE(atlas_opt < std_opt);
        CHECK(atlas_opt <= std_opt);
    }
}

// ======================================================================
// TEST SUITE: PERFORMANCE/SIZE VERIFICATION
// ======================================================================

TEST_SUITE("Optional Size and Performance")
{
    TEST_CASE("Optional has same size as wrapped type")
    {
        CHECK(
            sizeof(atlas::Nilable<test::WithSpaceship>) ==
            sizeof(test::WithSpaceship));
        CHECK(
            sizeof(atlas::Nilable<test::OnlyLessThan>) ==
            sizeof(test::OnlyLessThan));
        CHECK(
            sizeof(atlas::Nilable<test::SpaceshipString>) ==
            sizeof(test::SpaceshipString));
    }

    TEST_CASE("Optional has user-defined move operations")
    {
        // WithSpaceship wraps int, should be trivial
        CHECK(std::is_trivially_copyable<test::WithSpaceship>::value);
        // Optional is NOT trivially copyable because it has custom move
        // operations that reset moved-from objects to nil_value
        CHECK_FALSE(std::is_trivially_copyable<
              atlas::Nilable<test::WithSpaceship>>::value);
        // But it should still be copyable and movable
        CHECK(std::is_copy_constructible<
              atlas::Nilable<test::WithSpaceship>>::value);
        CHECK(std::is_move_constructible<
              atlas::Nilable<test::WithSpaceship>>::value);
        CHECK(std::is_copy_assignable<
              atlas::Nilable<test::WithSpaceship>>::value);
        CHECK(std::is_move_assignable<
              atlas::Nilable<test::WithSpaceship>>::value);
    }
}
