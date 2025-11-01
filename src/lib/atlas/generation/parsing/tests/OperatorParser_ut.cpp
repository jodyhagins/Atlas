// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file OperatorParser_ut.cpp
 * Comprehensive unit tests for OperatorParser
 *
 * Tests cover:
 * - Arithmetic binary operator classification
 * - Arithmetic unary operator classification
 * - Relational operator classification
 * - Cast syntax parsing (explicit, implicit, and shorthand)
 * - Error handling for invalid cast syntax
 * - Boundary cases and edge conditions
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/parsing/OperatorParser.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

#include "tests/doctest.hpp"

namespace {

using namespace wjh::atlas::generation;

// ============================================================================
// Arithmetic Binary Operator Tests
// ============================================================================

TEST_CASE("is_arithmetic_binary_operator - standard operators")
{
    SUBCASE("addition") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("+"));
    }

    SUBCASE("subtraction") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("-"));
    }

    SUBCASE("multiplication") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("*"));
    }

    SUBCASE("division") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("/"));
    }

    SUBCASE("modulo") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("%"));
    }
}

TEST_CASE("is_arithmetic_binary_operator - bitwise operators")
{
    SUBCASE("bitwise and") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("&"));
    }

    SUBCASE("bitwise or") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("|"));
    }

    SUBCASE("bitwise xor") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("^"));
    }

    SUBCASE("left shift") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("<<"));
    }

    SUBCASE("right shift") {
        CHECK(OperatorParser::is_arithmetic_binary_operator(">>"));
    }
}

TEST_CASE("is_arithmetic_binary_operator - special combined operators")
{
    SUBCASE("plus with unary (+*)") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("+*"));
    }

    SUBCASE("minus with unary (-*)") {
        CHECK(OperatorParser::is_arithmetic_binary_operator("-*"));
    }
}

TEST_CASE("is_arithmetic_binary_operator - negative cases")
{
    SUBCASE("increment (++)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("++"));
    }

    SUBCASE("decrement (--)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("--"));
    }

    SUBCASE("equality (==)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("=="));
    }

    SUBCASE("not equal (!=)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("!="));
    }

    SUBCASE("less than (<)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("<"));
    }

    SUBCASE("greater than (>)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator(">"));
    }

    SUBCASE("spaceship (<=>)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("<=>"));
    }

    SUBCASE("logical not (!)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("!"));
    }

    SUBCASE("logical and (&&)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("&&"));
    }

    SUBCASE("logical or (||)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("||"));
    }

    SUBCASE("unary plus (u+)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("u+"));
    }

    SUBCASE("unary minus (u-)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("u-"));
    }

    SUBCASE("bitwise not (~)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("~"));
    }

    SUBCASE("empty string") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator(""));
    }

    SUBCASE("random text") {
        CHECK_FALSE(OperatorParser::is_arithmetic_binary_operator("foo"));
    }
}

// ============================================================================
// Arithmetic Unary Operator Tests
// ============================================================================

TEST_CASE("is_arithmetic_unary_operator - recognized operators")
{
    SUBCASE("unary plus (u+)") {
        CHECK(OperatorParser::is_arithmetic_unary_operator("u+"));
    }

    SUBCASE("unary minus (u-)") {
        CHECK(OperatorParser::is_arithmetic_unary_operator("u-"));
    }

    SUBCASE("unary bitwise not (u~)") {
        CHECK(OperatorParser::is_arithmetic_unary_operator("u~"));
    }

    SUBCASE("bitwise not (~)") {
        CHECK(OperatorParser::is_arithmetic_unary_operator("~"));
    }
}

TEST_CASE("is_arithmetic_unary_operator - negative cases")
{
    SUBCASE("binary plus (+)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("+"));
    }

    SUBCASE("binary minus (-)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("-"));
    }

    SUBCASE("multiplication (*)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("*"));
    }

    SUBCASE("logical not (!)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("!"));
    }

    SUBCASE("increment (++)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("++"));
    }

    SUBCASE("combined plus (+*)") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("+*"));
    }

    SUBCASE("empty string") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator(""));
    }

    SUBCASE("random text") {
        CHECK_FALSE(OperatorParser::is_arithmetic_unary_operator("bar"));
    }
}

// ============================================================================
// Relational Operator Tests
// ============================================================================

TEST_CASE("is_relational_operator - equality operators")
{
    SUBCASE("equal (==)") {
        CHECK(OperatorParser::is_relational_operator("=="));
    }

    SUBCASE("not equal (!=)") {
        CHECK(OperatorParser::is_relational_operator("!="));
    }
}

TEST_CASE("is_relational_operator - ordering operators")
{
    SUBCASE("less than (<)") {
        CHECK(OperatorParser::is_relational_operator("<"));
    }

    SUBCASE("less than or equal (<=)") {
        CHECK(OperatorParser::is_relational_operator("<="));
    }

    SUBCASE("greater than (>)") {
        CHECK(OperatorParser::is_relational_operator(">"));
    }

    SUBCASE("greater than or equal (>=)") {
        CHECK(OperatorParser::is_relational_operator(">="));
    }
}

TEST_CASE("is_relational_operator - negative cases")
{
    SUBCASE("spaceship (<=>)") {
        // Spaceship is handled separately, not in relational_operators
        CHECK_FALSE(OperatorParser::is_relational_operator("<=>"));
    }

    SUBCASE("addition (+)") {
        CHECK_FALSE(OperatorParser::is_relational_operator("+"));
    }

    SUBCASE("subtraction (-)") {
        CHECK_FALSE(OperatorParser::is_relational_operator("-"));
    }

    SUBCASE("multiplication (*)") {
        CHECK_FALSE(OperatorParser::is_relational_operator("*"));
    }

    SUBCASE("logical and (&&)") {
        CHECK_FALSE(OperatorParser::is_relational_operator("&&"));
    }

    SUBCASE("logical or (||)") {
        CHECK_FALSE(OperatorParser::is_relational_operator("||"));
    }

    SUBCASE("left shift (<<)") {
        CHECK_FALSE(OperatorParser::is_relational_operator("<<"));
    }

    SUBCASE("right shift (>>)") {
        CHECK_FALSE(OperatorParser::is_relational_operator(">>"));
    }

    SUBCASE("empty string") {
        CHECK_FALSE(OperatorParser::is_relational_operator(""));
    }

    SUBCASE("random text") {
        CHECK_FALSE(OperatorParser::is_relational_operator("baz"));
    }
}

// ============================================================================
// Cast Syntax Parsing Tests
// ============================================================================

TEST_CASE("parse_cast_syntax - explicit cast with 'cast<Type>' syntax")
{
    SUBCASE("simple type") {
        bool is_implicit = true; // Set to wrong value to verify it's changed
        auto result = OperatorParser::parse_cast_syntax(
            "cast<int>",
            is_implicit);
        CHECK(result == "int");
        CHECK_FALSE(is_implicit);
    }

    SUBCASE("type with whitespace") {
        bool is_implicit = true;
        auto result = OperatorParser::parse_cast_syntax(
            "cast< double >",
            is_implicit);
        CHECK(result == "double");
        CHECK_FALSE(is_implicit);
    }

    SUBCASE("qualified type") {
        bool is_implicit = true;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<std::string>",
            is_implicit);
        CHECK(result == "std::string");
        CHECK_FALSE(is_implicit);
    }

    SUBCASE("template type") {
        bool is_implicit = true;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<std::vector<int>>",
            is_implicit);
        CHECK(result == "std::vector<int>");
        CHECK_FALSE(is_implicit);
    }

    SUBCASE("complex nested template") {
        bool is_implicit = true;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<std::map<std::string, std::vector<int>>>",
            is_implicit);
        CHECK(result == "std::map<std::string, std::vector<int>>");
        CHECK_FALSE(is_implicit);
    }
}

TEST_CASE("parse_cast_syntax - explicit cast with 'explicit_cast<Type>' syntax")
{
    SUBCASE("simple type") {
        bool is_implicit = true;
        auto result = OperatorParser::parse_cast_syntax(
            "explicit_cast<float>",
            is_implicit);
        CHECK(result == "float");
        CHECK_FALSE(is_implicit);
    }

    SUBCASE("type with whitespace") {
        bool is_implicit = true;
        auto result = OperatorParser::parse_cast_syntax(
            "explicit_cast<  long  >",
            is_implicit);
        CHECK(result == "long");
        CHECK_FALSE(is_implicit);
    }
}

TEST_CASE("parse_cast_syntax - implicit cast with 'implicit_cast<Type>' syntax")
{
    SUBCASE("simple type") {
        bool is_implicit = false; // Set to wrong value to verify it's changed
        auto result = OperatorParser::parse_cast_syntax(
            "implicit_cast<bool>",
            is_implicit);
        CHECK(result == "bool");
        CHECK(is_implicit);
    }

    SUBCASE("type with whitespace") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "implicit_cast<  char  >",
            is_implicit);
        CHECK(result == "char");
        CHECK(is_implicit);
    }

    SUBCASE("qualified type") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "implicit_cast<std::string_view>",
            is_implicit);
        CHECK(result == "std::string_view");
        CHECK(is_implicit);
    }
}

TEST_CASE("parse_cast_syntax - non-cast operators return empty string")
{
    SUBCASE("arithmetic operator") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax("+", is_implicit);
        CHECK(result.empty());
    }

    SUBCASE("relational operator") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax("==", is_implicit);
        CHECK(result.empty());
    }

    SUBCASE("increment operator") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax("++", is_implicit);
        CHECK(result.empty());
    }

    SUBCASE("random text") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax("foobar", is_implicit);
        CHECK(result.empty());
    }

    SUBCASE("empty string") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax("", is_implicit);
        CHECK(result.empty());
    }

    SUBCASE("almost cast syntax") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "castint>",
            is_implicit);
        CHECK(result.empty());
    }
}

TEST_CASE("parse_cast_syntax - error cases throw std::invalid_argument")
{
    SUBCASE("missing closing bracket") {
        bool is_implicit = false;
        bool threw = false;
        try {
            static_cast<void>(
                OperatorParser::parse_cast_syntax("cast<int", is_implicit));
        } catch (std::invalid_argument const &) {
            threw = true;
        }
        CHECK(threw);
    }

    SUBCASE("missing closing bracket with explicit_cast") {
        bool is_implicit = false;
        bool threw = false;
        try {
            static_cast<void>(OperatorParser::parse_cast_syntax(
                "explicit_cast<double",
                is_implicit));
        } catch (std::invalid_argument const &) {
            threw = true;
        }
        CHECK(threw);
    }

    SUBCASE("missing closing bracket with implicit_cast") {
        bool is_implicit = false;
        bool threw = false;
        try {
            static_cast<void>(OperatorParser::parse_cast_syntax(
                "implicit_cast<bool",
                is_implicit));
        } catch (std::invalid_argument const &) {
            threw = true;
        }
        CHECK(threw);
    }

    SUBCASE("empty angle brackets") {
        bool is_implicit = false;
        bool threw = false;
        try {
            static_cast<void>(
                OperatorParser::parse_cast_syntax("cast<>", is_implicit));
        } catch (std::invalid_argument const &) {
            threw = true;
        }
        CHECK(threw);
    }
}

TEST_CASE("parse_cast_syntax - whitespace trimming")
{
    SUBCASE("leading whitespace in type") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<  int>",
            is_implicit);
        CHECK(result == "int");
    }

    SUBCASE("trailing whitespace in type") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<int  >",
            is_implicit);
        CHECK(result == "int");
    }

    SUBCASE("both leading and trailing whitespace") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<  int  >",
            is_implicit);
        CHECK(result == "int");
    }

    SUBCASE("tabs and spaces") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<\t int \t>",
            is_implicit);
        CHECK(result == "int");
    }

    SUBCASE("qualified type with whitespace") {
        bool is_implicit = false;
        auto result = OperatorParser::parse_cast_syntax(
            "cast<  std::string  >",
            is_implicit);
        CHECK(result == "std::string");
    }
}

// ============================================================================
// ArithmeticMode Enum Tests
// ============================================================================

TEST_CASE("ArithmeticMode enum values exist")
{
    // Ensure all enum values are accessible
    SUBCASE("Default mode") {
        auto mode = ArithmeticMode::Default;
        CHECK(mode == ArithmeticMode::Default);
    }

    SUBCASE("Checked mode") {
        auto mode = ArithmeticMode::Checked;
        CHECK(mode == ArithmeticMode::Checked);
    }

    SUBCASE("Saturating mode") {
        auto mode = ArithmeticMode::Saturating;
        CHECK(mode == ArithmeticMode::Saturating);
    }

    SUBCASE("Wrapping mode") {
        auto mode = ArithmeticMode::Wrapping;
        CHECK(mode == ArithmeticMode::Wrapping);
    }
}

TEST_CASE("ArithmeticMode enum values are distinct")
{
    CHECK(ArithmeticMode::Default != ArithmeticMode::Checked);
    CHECK(ArithmeticMode::Default != ArithmeticMode::Saturating);
    CHECK(ArithmeticMode::Default != ArithmeticMode::Wrapping);
    CHECK(ArithmeticMode::Checked != ArithmeticMode::Saturating);
    CHECK(ArithmeticMode::Checked != ArithmeticMode::Wrapping);
    CHECK(ArithmeticMode::Saturating != ArithmeticMode::Wrapping);
}

// ============================================================================
// Constexpr Tests
// ============================================================================

TEST_CASE("Operator classification functions are constexpr")
{
    // These tests verify that the functions can be used in constexpr contexts
    SUBCASE("is_arithmetic_binary_operator is constexpr") {
        constexpr bool result = OperatorParser::is_arithmetic_binary_operator(
            "+");
        CHECK(result);
    }

    SUBCASE("is_arithmetic_unary_operator is constexpr") {
        constexpr bool result = OperatorParser::is_arithmetic_unary_operator(
            "u+");
        CHECK(result);
    }

    SUBCASE("is_relational_operator is constexpr") {
        constexpr bool result = OperatorParser::is_relational_operator("==");
        CHECK(result);
    }
}

// ============================================================================
// Static Array Access Tests
// ============================================================================

TEST_CASE("Static operator arrays are accessible")
{
    SUBCASE("arithmetic_binary_op_tags array") {
        auto const & arr = OperatorParser::arithmetic_binary_op_tags;
        CHECK(arr.size() == 12);
        // Verify some entries
        CHECK(std::find(arr.begin(), arr.end(), "+") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "-") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "+*") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "-*") != arr.end());
    }

    SUBCASE("arithmetic_unary_operators array") {
        auto const & arr = OperatorParser::arithmetic_unary_operators;
        CHECK(arr.size() == 4);
        // Verify all entries
        CHECK(std::find(arr.begin(), arr.end(), "u+") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "u-") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "u~") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "~") != arr.end());
    }

    SUBCASE("relational_operators array") {
        auto const & arr = OperatorParser::relational_operators;
        CHECK(arr.size() == 6);
        // Verify all entries
        CHECK(std::find(arr.begin(), arr.end(), "==") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "!=") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "<") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), "<=") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), ">") != arr.end());
        CHECK(std::find(arr.begin(), arr.end(), ">=") != arr.end());
    }
}

} // anonymous namespace
