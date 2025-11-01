// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive runtime test for arithmetic modes
// Tests all operations, all modes, all types

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "arithmetic_modes_test_types.hpp"

#include <cmath>
#include <limits>

#include "doctest.hpp"

// ======================================================================
// CHECKED ARITHMETIC TESTS
// ======================================================================

TEST_SUITE("Checked Arithmetic Mode")
{
    TEST_CASE("Checked Signed Int - Normal operations")
    {
        test::CheckedInt8 a{10};
        test::CheckedInt8 b{20};
        auto c = a + b;
        CHECK(static_cast<std::int8_t>(c) == 30);
    }

    TEST_CASE("Checked Signed Int - Addition overflow")
    {
        test::CheckedInt8 a{127};
        test::CheckedInt8 b{1};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Signed Int - Subtraction underflow")
    {
        test::CheckedInt8 a{-128};
        test::CheckedInt8 b{1};
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("Checked Signed Int - Multiplication overflow")
    {
        test::CheckedInt8 a{100};
        test::CheckedInt8 b{2};
        CHECK_THROWS_AS(a * b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Signed Int - Multiplication underflow")
    {
        test::CheckedInt8 a{-128};
        test::CheckedInt8 b{2};
        CHECK_THROWS_AS(a * b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("Checked Signed Int - Division by zero")
    {
        test::CheckedInt8 a{10};
        test::CheckedInt8 b{0};
        CHECK_THROWS_AS(a / b, atlas::CheckedDivisionByZeroError);
    }

    TEST_CASE("Checked Signed Int - INT_MIN / -1 overflow")
    {
        test::CheckedInt8 a{-128};
        test::CheckedInt8 b{-1};
        CHECK_THROWS_AS(a / b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Signed Int - Modulo by zero")
    {
        test::CheckedInt8 a{10};
        test::CheckedInt8 b{0};
        CHECK_THROWS_AS(a % b, atlas::CheckedDivisionByZeroError);
    }

    TEST_CASE("Checked Signed Int - INT_MIN % -1 overflow")
    {
        test::CheckedInt8 a{-128};
        test::CheckedInt8 b{-1};
        CHECK_THROWS_AS(a % b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Unsigned Int - Normal operations")
    {
        test::CheckedUInt8 a{10};
        test::CheckedUInt8 b{20};
        auto c = a + b;
        CHECK(static_cast<std::uint8_t>(c) == 30);
    }

    TEST_CASE("Checked Unsigned Int - Addition overflow")
    {
        test::CheckedUInt8 a{255};
        test::CheckedUInt8 b{1};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Unsigned Int - Subtraction underflow")
    {
        test::CheckedUInt8 a{0};
        test::CheckedUInt8 b{1};
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("Checked Unsigned Int - Multiplication overflow")
    {
        test::CheckedUInt8 a{200};
        test::CheckedUInt8 b{2};
        CHECK_THROWS_AS(a * b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Unsigned Int - Division by zero")
    {
        test::CheckedUInt8 a{10};
        test::CheckedUInt8 b{0};
        CHECK_THROWS_AS(a / b, atlas::CheckedDivisionByZeroError);
    }

    TEST_CASE("Checked Float - Normal operations")
    {
        test::CheckedFloat a{1.5f};
        test::CheckedFloat b{2.5f};
        auto c = a + b;
        CHECK(static_cast<float>(c) == 4.0f);
    }

    TEST_CASE("Checked Float - Overflow to infinity")
    {
        test::CheckedFloat a{std::numeric_limits<float>::max()};
        test::CheckedFloat b{std::numeric_limits<float>::max()};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Float - Division by zero")
    {
        test::CheckedFloat a{1.0f};
        test::CheckedFloat b{0.0f};
        CHECK_THROWS_AS(a / b, atlas::CheckedDivisionByZeroError);
    }

    TEST_CASE("Checked Float - 0/0 produces NaN")
    {
        test::CheckedFloat a{0.0f};
        test::CheckedFloat b{0.0f};
        CHECK_THROWS_AS(a / b, atlas::CheckedDivisionByZeroError);
    }

    TEST_CASE("Checked Modulo - Normal operations")
    {
        test::CheckedInt8 a{10};
        test::CheckedInt8 b{3};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 1);
    }

    TEST_CASE("Checked Modulo - Negative dividend")
    {
        test::CheckedInt8 a{-10};
        test::CheckedInt8 b{3};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == -1);
    }

    TEST_CASE("Checked Modulo - Unsigned normal")
    {
        test::CheckedUInt8 a{10};
        test::CheckedUInt8 b{3};
        auto c = a % b;
        CHECK(static_cast<std::uint8_t>(c) == 1);
    }

    TEST_CASE("Checked Modulo - Division by zero throws")
    {
        test::CheckedInt8 a{10};
        test::CheckedInt8 b{0};
        CHECK_THROWS_AS(a % b, atlas::CheckedDivisionByZeroError);
    }

    TEST_CASE("Checked Modulo - INT_MIN % -1 throws")
    {
        test::CheckedInt8 a{-128};
        test::CheckedInt8 b{-1};
        CHECK_THROWS_AS(a % b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked 32-bit - INT_MAX + 1 overflow")
    {
        test::CheckedInt a{2147483647};
        test::CheckedInt b{1};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked 32-bit - INT_MIN - 1 underflow")
    {
        test::CheckedInt a{-2147483647 - 1};
        test::CheckedInt b{1};
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("Checked 32-bit - INT_MIN / -1 overflow")
    {
        test::CheckedInt a{-2147483647 - 1};
        test::CheckedInt b{-1};
        CHECK_THROWS_AS(a / b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked 32-bit - INT_MIN % -1 overflow")
    {
        test::CheckedInt a{-2147483647 - 1};
        test::CheckedInt b{-1};
        CHECK_THROWS_AS(a % b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked 32-bit - UINT_MAX + 1 overflow")
    {
        test::CheckedUInt a{4294967295U};
        test::CheckedUInt b{1};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked 32-bit - 0 - 1 underflow")
    {
        test::CheckedUInt a{0};
        test::CheckedUInt b{1};
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("Checked Double - Normal operations")
    {
        test::CheckedDouble a{1.5};
        test::CheckedDouble b{2.5};
        auto c = a + b;
        CHECK(static_cast<double>(c) == 4.0);
    }

    TEST_CASE("Checked Double - Infinity arithmetic throws")
    {
        test::CheckedDouble a{std::numeric_limits<double>::infinity()};
        test::CheckedDouble b{1.0};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Double - Negative infinity throws")
    {
        test::CheckedDouble a{-std::numeric_limits<double>::infinity()};
        test::CheckedDouble b{1.0};
        CHECK_THROWS_AS(a + b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("Checked Double - NaN propagation throws")
    {
        test::CheckedDouble a{std::numeric_limits<double>::quiet_NaN()};
        test::CheckedDouble b{1.0};
        CHECK_THROWS_AS(a + b, atlas::CheckedInvalidOperationError);
    }

    TEST_CASE("Checked Double - Very large multiplication")
    {
        test::CheckedDouble a{1e308};
        test::CheckedDouble b{10.0};
        CHECK_THROWS_AS(a * b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Double - Very large negative multiplication")
    {
        test::CheckedDouble a{-1e308};
        test::CheckedDouble b{10.0};
        CHECK_THROWS_AS(a * b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Float - Infinity throws")
    {
        test::CheckedFloat a{std::numeric_limits<float>::infinity()};
        test::CheckedFloat b{1.0f};
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Float - NaN throws")
    {
        test::CheckedFloat a{std::numeric_limits<float>::quiet_NaN()};
        test::CheckedFloat b{1.0f};
        CHECK_THROWS_AS(a * b, atlas::CheckedInvalidOperationError);
    }

    TEST_CASE("Checked Chain - Multiple additions overflow")
    {
        test::CheckedInt8 a{50};
        test::CheckedInt8 b{50};
        test::CheckedInt8 c{50};
        CHECK_THROWS_AS(a + b + c, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Chain - Multiple multiplications overflow")
    {
        test::CheckedInt8 a{50};
        test::CheckedInt8 b{2};
        test::CheckedInt8 c{3};
        CHECK_THROWS_AS(a * b * c, atlas::CheckedOverflowError);
    }

    TEST_CASE("Checked Chain - Complex expression normal")
    {
        test::CheckedInt8 a{10};
        test::CheckedInt8 b{5};
        test::CheckedInt8 c{3};
        auto d = (a + b) * c;
        CHECK(static_cast<std::int8_t>(d) == 45);
    }
}

// ======================================================================
// SATURATING ARITHMETIC TESTS
// ======================================================================

TEST_SUITE("Saturating Arithmetic Mode")
{
    TEST_CASE("Saturating Signed Int - Normal operations")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{20};
        auto c = a + b;
        CHECK(static_cast<std::int8_t>(c) == 30);
    }

    TEST_CASE("Saturating Signed Int - Addition saturates to max")
    {
        test::SaturatingInt8 a{127};
        test::SaturatingInt8 b{1};
        auto c = a + b;
        CHECK(static_cast<std::int8_t>(c) == 127);
    }

    TEST_CASE("Saturating Signed Int - Subtraction saturates to min")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{1};
        auto c = a - b;
        CHECK(static_cast<std::int8_t>(c) == -128);
    }

    TEST_CASE("Saturating Signed Int - Multiplication saturates to max")
    {
        test::SaturatingInt8 a{100};
        test::SaturatingInt8 b{2};
        auto c = a * b;
        CHECK(static_cast<std::int8_t>(c) == 127);
    }

    TEST_CASE("Saturating Signed Int - Multiplication saturates to min")
    {
        test::SaturatingInt8 a{-100};
        test::SaturatingInt8 b{2};
        auto c = a * b;
        CHECK(static_cast<std::int8_t>(c) == -128);
    }

    TEST_CASE(
        "Saturating Signed Int - Division by positive zero saturates to max")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{0};
        auto c = a / b;
        CHECK(static_cast<std::int8_t>(c) == 127);
    }

    TEST_CASE(
        "Saturating Signed Int - Division by negative zero saturates to min")
    {
        test::SaturatingInt8 a{-10};
        test::SaturatingInt8 b{0};
        auto c = a / b;
        CHECK(static_cast<std::int8_t>(c) == -128);
    }

    TEST_CASE("Saturating Signed Int - 0 / 0 returns 0")
    {
        test::SaturatingInt8 a{0};
        test::SaturatingInt8 b{0};
        auto c = a / b;
        CHECK(static_cast<std::int8_t>(c) == 0);
    }

    TEST_CASE("Saturating Signed Int - INT_MIN / -1 saturates to max")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{-1};
        auto c = a / b;
        CHECK(static_cast<std::int8_t>(c) == 127);
    }

    TEST_CASE("Saturating Unsigned Int - Normal operations")
    {
        test::SaturatingUInt8 a{10};
        test::SaturatingUInt8 b{20};
        auto c = a + b;
        CHECK(static_cast<std::uint8_t>(c) == 30);
    }

    TEST_CASE("Saturating Unsigned Int - Addition saturates to max")
    {
        test::SaturatingUInt8 a{255};
        test::SaturatingUInt8 b{1};
        auto c = a + b;
        CHECK(static_cast<std::uint8_t>(c) == 255);
    }

    TEST_CASE("Saturating Unsigned Int - Subtraction saturates to zero")
    {
        test::SaturatingUInt8 a{0};
        test::SaturatingUInt8 b{1};
        auto c = a - b;
        CHECK(static_cast<std::uint8_t>(c) == 0);
    }

    TEST_CASE("Saturating Unsigned Int - Division by zero saturates to max")
    {
        test::SaturatingUInt8 a{10};
        test::SaturatingUInt8 b{0};
        auto c = a / b;
        CHECK(static_cast<std::uint8_t>(c) == 255);
    }

    TEST_CASE("Saturating Unsigned Int - 0 / 0 returns 0")
    {
        test::SaturatingUInt8 a{0};
        test::SaturatingUInt8 b{0};
        auto c = a / b;
        CHECK(static_cast<std::uint8_t>(c) == 0);
    }

    TEST_CASE("Saturating Float - Normal operations")
    {
        test::SaturatingFloat a{1.5f};
        test::SaturatingFloat b{2.5f};
        auto c = a + b;
        CHECK(static_cast<float>(c) == 4.0f);
    }

    TEST_CASE("Saturating Float - Overflow saturates to max")
    {
        test::SaturatingFloat a{std::numeric_limits<float>::max()};
        test::SaturatingFloat b{std::numeric_limits<float>::max()};
        auto c = a + b;
        CHECK(static_cast<float>(c) == std::numeric_limits<float>::max());
    }

    TEST_CASE("Saturating Float - Division by +0 saturates to max")
    {
        test::SaturatingFloat a{5.0f};
        test::SaturatingFloat b{0.0f};
        auto c = a / b;
        CHECK(static_cast<float>(c) == std::numeric_limits<float>::max());
    }

    TEST_CASE("Saturating Float - Division by -0 saturates to lowest")
    {
        test::SaturatingFloat a{5.0f};
        test::SaturatingFloat b{-0.0f};
        auto c = a / b;
        CHECK(static_cast<float>(c) == std::numeric_limits<float>::lowest());
    }

    TEST_CASE("Saturating Float - Negative / -0 saturates to max")
    {
        test::SaturatingFloat a{-5.0f};
        test::SaturatingFloat b{-0.0f};
        auto c = a / b;
        CHECK(static_cast<float>(c) == std::numeric_limits<float>::max());
    }

    TEST_CASE("Saturating Float - 0 / 0 returns 0")
    {
        test::SaturatingFloat a{0.0f};
        test::SaturatingFloat b{0.0f};
        auto c = a / b;
        CHECK(static_cast<float>(c) == 0.0f);
    }

    TEST_CASE("Saturating Modulo - Signed normal")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{3};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 1);
    }

    TEST_CASE("Saturating Modulo - Signed negative dividend")
    {
        test::SaturatingInt8 a{-10};
        test::SaturatingInt8 b{3};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == -1);
    }

    TEST_CASE("Saturating Modulo - Signed negative divisor")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{-3};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 1);
    }

    TEST_CASE("Saturating Modulo - Signed modulo by zero does not throw")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{0};
        CHECK_NOTHROW(auto c = a % b);
    }

    TEST_CASE("Saturating Modulo - INT_MIN % -1 does not throw")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{-1};
        CHECK_NOTHROW(auto c = a % b);
    }

    TEST_CASE("Saturating Modulo - Unsigned normal")
    {
        test::SaturatingUInt8 a{10};
        test::SaturatingUInt8 b{3};
        auto c = a % b;
        CHECK(static_cast<std::uint8_t>(c) == 1);
    }

    TEST_CASE("Saturating Modulo - Unsigned large values")
    {
        test::SaturatingUInt8 a{255};
        test::SaturatingUInt8 b{10};
        auto c = a % b;
        CHECK(static_cast<std::uint8_t>(c) == 5);
    }

    TEST_CASE("Saturating Modulo - Unsigned modulo by zero does not throw")
    {
        test::SaturatingUInt8 a{10};
        test::SaturatingUInt8 b{0};
        CHECK_NOTHROW(auto c = a % b);
    }

    TEST_CASE("Saturating Modulo - Larger signed types")
    {
        test::SaturatingInt a{1000};
        test::SaturatingInt b{7};
        auto c = a % b;
        CHECK(static_cast<int>(c) == 6);
    }

    TEST_CASE("Saturating Modulo - Larger unsigned types")
    {
        test::SaturatingUInt a{12345};
        test::SaturatingUInt b{100};
        auto c = a % b;
        CHECK(static_cast<unsigned int>(c) == 45);
    }

    TEST_CASE("Saturating No-Throw - Addition overflow")
    {
        test::SaturatingInt8 a{127};
        test::SaturatingInt8 b{1};
        CHECK_NOTHROW(auto c = a + b);
    }

    TEST_CASE("Saturating No-Throw - Subtraction underflow")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{1};
        CHECK_NOTHROW(auto c = a - b);
    }

    TEST_CASE("Saturating No-Throw - Multiplication overflow")
    {
        test::SaturatingInt8 a{100};
        test::SaturatingInt8 b{2};
        CHECK_NOTHROW(auto c = a * b);
    }

    TEST_CASE("Saturating No-Throw - Division by zero")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{0};
        CHECK_NOTHROW(auto c = a / b);
    }

    TEST_CASE("Saturating No-Throw - Modulo by zero")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{0};
        CHECK_NOTHROW(auto c = a % b);
    }

    TEST_CASE("Saturating No-Throw - INT_MIN / -1")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{-1};
        CHECK_NOTHROW(auto c = a / b);
    }

    TEST_CASE("Saturating No-Throw - INT_MIN % -1")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{-1};
        CHECK_NOTHROW(auto c = a % b);
    }

    TEST_CASE("Saturating No-Throw - Unsigned operations")
    {
        CHECK_NOTHROW(test::SaturatingUInt8{255} + test::SaturatingUInt8{1});
        CHECK_NOTHROW(test::SaturatingUInt8{0} - test::SaturatingUInt8{1});
        CHECK_NOTHROW(test::SaturatingUInt8{200} * test::SaturatingUInt8{2});
        CHECK_NOTHROW(test::SaturatingUInt8{10} / test::SaturatingUInt8{0});
        CHECK_NOTHROW(test::SaturatingUInt8{10} % test::SaturatingUInt8{0});
    }

    TEST_CASE("Saturating No-Throw - Floating-point operations")
    {
        CHECK_NOTHROW(
            test::SaturatingFloat{std::numeric_limits<float>::max()} +
            test::SaturatingFloat{std::numeric_limits<float>::max()});
        CHECK_NOTHROW(
            test::SaturatingFloat{1.0f} / test::SaturatingFloat{0.0f});
        CHECK_NOTHROW(
            test::SaturatingFloat{1.0f} / test::SaturatingFloat{-0.0f});
        CHECK_NOTHROW(
            test::SaturatingFloat{0.0f} / test::SaturatingFloat{0.0f});
        CHECK_NOTHROW(
            test::SaturatingDouble{1e308} + test::SaturatingDouble{1e308});
    }

    TEST_CASE("Saturating 32-bit - INT_MAX + 1 saturates")
    {
        test::SaturatingInt a{2147483647};
        test::SaturatingInt b{1};
        auto c = a + b;
        CHECK(static_cast<int>(c) == 2147483647);
    }

    TEST_CASE("Saturating 32-bit - INT_MIN - 1 saturates")
    {
        test::SaturatingInt a{-2147483647 - 1};
        test::SaturatingInt b{1};
        auto c = a - b;
        CHECK(static_cast<int>(c) == -2147483647 - 1);
    }

    TEST_CASE("Saturating 32-bit - INT_MIN / -1 saturates")
    {
        test::SaturatingInt a{-2147483647 - 1};
        test::SaturatingInt b{-1};
        auto c = a / b;
        CHECK(static_cast<int>(c) == 2147483647);
    }

    TEST_CASE("Saturating 32-bit - UINT_MAX + 1 saturates")
    {
        test::SaturatingUInt a{4294967295U};
        test::SaturatingUInt b{1};
        auto c = a + b;
        CHECK(static_cast<unsigned int>(c) == 4294967295U);
    }

    TEST_CASE("Saturating 32-bit - 0 - 1 saturates to 0")
    {
        test::SaturatingUInt a{0};
        test::SaturatingUInt b{1};
        auto c = a - b;
        CHECK(static_cast<unsigned int>(c) == 0);
    }

    TEST_CASE("Saturating Double - Infinity saturates")
    {
        test::SaturatingDouble a{std::numeric_limits<double>::infinity()};
        test::SaturatingDouble b{1.0};
        auto c = a + b;
        CHECK(static_cast<double>(c) == std::numeric_limits<double>::max());
    }

    TEST_CASE("Saturating Double - NaN does not throw")
    {
        test::SaturatingDouble a{std::numeric_limits<double>::quiet_NaN()};
        test::SaturatingDouble b{1.0};
        CHECK_NOTHROW(auto c = a + b);
    }

    TEST_CASE("Saturating Double - Very large multiplication")
    {
        test::SaturatingDouble a{1e308};
        test::SaturatingDouble b{10.0};
        auto c = a * b;
        CHECK(static_cast<double>(c) == std::numeric_limits<double>::max());
    }

    TEST_CASE("Saturating Chain - Multiple additions saturate")
    {
        test::SaturatingInt8 a{100};
        test::SaturatingInt8 b{100};
        test::SaturatingInt8 c{100};
        auto d = a + b + c;
        CHECK(static_cast<std::int8_t>(d) == 127);
    }

    TEST_CASE("Saturating Chain - Negative additions saturate")
    {
        test::SaturatingInt8 a{-50};
        test::SaturatingInt8 b{-50};
        test::SaturatingInt8 c{-50};
        auto d = a + b + c;
        CHECK(static_cast<std::int8_t>(d) == -128);
    }

    TEST_CASE("Saturating Chain - Complex expression")
    {
        test::SaturatingInt8 a{100};
        test::SaturatingInt8 b{50};
        test::SaturatingInt8 c{2};
        auto d = (a + b) * c;
        CHECK(static_cast<std::int8_t>(d) == 127);
    }

    // Saturating Remainder Tests
    TEST_CASE("Saturating Signed Int - Normal remainder")
    {
        test::SaturatingInt8 a{17};
        test::SaturatingInt8 b{5};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 2);
    }

    TEST_CASE("Saturating Signed Int - Negative remainder")
    {
        test::SaturatingInt8 a{-17};
        test::SaturatingInt8 b{5};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == -2);
    }

    TEST_CASE("Saturating Signed Int - Remainder by zero returns 0")
    {
        test::SaturatingInt8 a{10};
        test::SaturatingInt8 b{0};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 0);
    }

    TEST_CASE("Saturating Signed Int - 0 % 0 returns 0")
    {
        test::SaturatingInt8 a{0};
        test::SaturatingInt8 b{0};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 0);
    }

    TEST_CASE("Saturating Signed Int - INT_MIN % -1 returns 0")
    {
        test::SaturatingInt8 a{-128};
        test::SaturatingInt8 b{-1};
        auto c = a % b;
        CHECK(static_cast<std::int8_t>(c) == 0);
    }

    TEST_CASE("Saturating Unsigned Int - Normal remainder")
    {
        test::SaturatingUInt8 a{17};
        test::SaturatingUInt8 b{5};
        auto c = a % b;
        CHECK(static_cast<std::uint8_t>(c) == 2);
    }

    TEST_CASE("Saturating Unsigned Int - Remainder by zero returns 0")
    {
        test::SaturatingUInt8 a{10};
        test::SaturatingUInt8 b{0};
        auto c = a % b;
        CHECK(static_cast<std::uint8_t>(c) == 0);
    }

    TEST_CASE("Saturating Unsigned Int - 0 % 0 returns 0")
    {
        test::SaturatingUInt8 a{0};
        test::SaturatingUInt8 b{0};
        auto c = a % b;
        CHECK(static_cast<std::uint8_t>(c) == 0);
    }
}

// ======================================================================
// WRAPPING ARITHMETIC TESTS
// ======================================================================

TEST_SUITE("Wrapping Arithmetic Mode")
{
    TEST_CASE("Wrapping Signed Int - Normal operations")
    {
        test::WrappingInt8 a{10};
        test::WrappingInt8 b{20};
        auto c = a + b;
        CHECK(static_cast<std::int8_t>(c) == 30);
    }

    TEST_CASE("Wrapping Signed Int - Addition wraps to negative")
    {
        test::WrappingInt8 a{127};
        test::WrappingInt8 b{1};
        auto c = a + b;
        CHECK(static_cast<std::int8_t>(c) == -128);
    }

    TEST_CASE("Wrapping Signed Int - Subtraction wraps to positive")
    {
        test::WrappingInt8 a{-128};
        test::WrappingInt8 b{1};
        auto c = a - b;
        CHECK(static_cast<std::int8_t>(c) == 127);
    }

    TEST_CASE("Wrapping Signed Int - Multiplication wraps")
    {
        test::WrappingInt8 a{100};
        test::WrappingInt8 b{2};
        auto c = a * b;
        CHECK(static_cast<std::int8_t>(c) == static_cast<std::int8_t>(200));
    }

    TEST_CASE("Wrapping Unsigned Int - Normal operations")
    {
        test::WrappingUInt8 a{10};
        test::WrappingUInt8 b{20};
        auto c = a + b;
        CHECK(static_cast<std::uint8_t>(c) == 30);
    }

    TEST_CASE("Wrapping Unsigned Int - Addition wraps to zero")
    {
        test::WrappingUInt8 a{255};
        test::WrappingUInt8 b{1};
        auto c = a + b;
        CHECK(static_cast<std::uint8_t>(c) == 0);
    }

    TEST_CASE("Wrapping Unsigned Int - Subtraction wraps to max")
    {
        test::WrappingUInt8 a{0};
        test::WrappingUInt8 b{1};
        auto c = a - b;
        CHECK(static_cast<std::uint8_t>(c) == 255);
    }

    TEST_CASE("Wrapping Unsigned Int - Multiplication wraps")
    {
        test::WrappingUInt8 a{200};
        test::WrappingUInt8 b{2};
        auto c = a * b;
        CHECK(static_cast<std::uint8_t>(c) == static_cast<std::uint8_t>(400));
    }

    TEST_CASE("Wrapping No-Throw - All operations")
    {
        CHECK_NOTHROW(test::WrappingInt8{127} + test::WrappingInt8{1});
        CHECK_NOTHROW(test::WrappingInt8{-128} - test::WrappingInt8{1});
        CHECK_NOTHROW(test::WrappingInt8{100} * test::WrappingInt8{2});
        CHECK_NOTHROW(test::WrappingUInt8{255} + test::WrappingUInt8{1});
        CHECK_NOTHROW(test::WrappingUInt8{0} - test::WrappingUInt8{1});
        CHECK_NOTHROW(test::WrappingUInt8{200} * test::WrappingUInt8{2});
    }

    TEST_CASE("Wrapping 32-bit - INT_MAX + 1 wraps")
    {
        test::WrappingInt a{2147483647};
        test::WrappingInt b{1};
        auto c = a + b;
        CHECK(static_cast<int>(c) == -2147483647 - 1);
    }

    TEST_CASE("Wrapping 32-bit - INT_MIN - 1 wraps")
    {
        test::WrappingInt a{-2147483647 - 1};
        test::WrappingInt b{1};
        auto c = a - b;
        CHECK(static_cast<int>(c) == 2147483647);
    }

    TEST_CASE("Wrapping 32-bit - UINT_MAX + 1 wraps")
    {
        test::WrappingUInt a{4294967295U};
        test::WrappingUInt b{1};
        auto c = a + b;
        CHECK(static_cast<unsigned int>(c) == 0);
    }

    TEST_CASE("Wrapping 32-bit - 0 - 1 wraps")
    {
        test::WrappingUInt a{0};
        test::WrappingUInt b{1};
        auto c = a - b;
        CHECK(static_cast<unsigned int>(c) == 4294967295U);
    }

    TEST_CASE("Wrapping Chain - Multiple additions")
    {
        test::WrappingInt8 a{100};
        test::WrappingInt8 b{100};
        test::WrappingInt8 c{100};
        auto d = a + b + c;
        CHECK(static_cast<std::int8_t>(d) == 44);
    }

    TEST_CASE("Wrapping Chain - Unsigned multiple additions")
    {
        test::WrappingUInt8 a{200};
        test::WrappingUInt8 b{200};
        test::WrappingUInt8 c{200};
        auto d = a + b + c;
        CHECK(static_cast<std::uint8_t>(d) == 88);
    }

    TEST_CASE("Wrapping Chain - Complex expression")
    {
        test::WrappingInt8 a{100};
        test::WrappingInt8 b{50};
        test::WrappingInt8 c{2};
        auto d = (a + b) * c;
        CHECK(static_cast<std::int8_t>(d) == static_cast<std::int8_t>(300));
    }
}

// ======================================================================
// EDGE CASES
// ======================================================================

TEST_SUITE("Edge Cases")
{
    TEST_CASE("Larger types work correctly")
    {
        test::CheckedInt a{1000000};
        test::CheckedInt b{2000000};
        auto c = a + b;
        CHECK(static_cast<int>(c) == 3000000);
    }

    TEST_CASE("Double precision works")
    {
        test::CheckedDouble a{1.5};
        test::CheckedDouble b{2.5};
        auto c = a + b;
        CHECK(static_cast<double>(c) == 4.0);
    }
}
