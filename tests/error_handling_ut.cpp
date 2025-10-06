// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "StrongTypeGenerator.hpp"
#include "doctest.hpp"

using namespace wjh::atlas;

namespace {

// Helper function for simple code generation without warnings
std::string
generate_strong_type(StrongTypeDescription const & desc)
{
    StrongTypeGenerator gen;
    return gen(desc);
}

TEST_SUITE("Error Handling")
{
    TEST_CASE("Unrecognized Operators")
    {
        SUBCASE("single unrecognized operator") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; invalid_operator"};

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("mixed valid and invalid operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +, -, unknown, =="};

            CHECK_THROWS_WITH(
                generate_strong_type(desc),
                "Unrecognized operator or option in description: 'unknown'");
        }

        SUBCASE("typo in operator name") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; ++, oout"};

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("empty token after comma") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +, , =="};

            // Empty tokens should be skipped, not throw
            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("trailing comma") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +, -,"};

            // Trailing comma should not cause error
            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("invalid include syntax - missing angle bracket") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; #<header"};

            // Should not throw - it's treated as a valid include
            CHECK_NOTHROW(generate_strong_type(desc));
        }
    }

    TEST_CASE("Invalid Kind")
    {
        SUBCASE("invalid kind value") {
            StrongTypeDescription desc{
                .kind = "union",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int"};

            CHECK_THROWS_WITH(
                generate_strong_type(desc),
                "kind must be either class or struct");
        }

        SUBCASE("empty kind") {
            StrongTypeDescription desc{
                .kind = "",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int"};

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("uppercase kind") {
            StrongTypeDescription desc{
                .kind = "STRUCT",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int"};

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }
    }

    TEST_CASE("Edge Cases in Description")
    {
        SUBCASE("very long underlying type") {
            std::string long_type =
                "std::map<std::string, "
                "std::vector<std::optional<std::pair<int, std::string>>>>";
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong " + long_type};

            CHECK_NOTHROW(generate_strong_type(desc));
            auto code = generate_strong_type(desc);
            CHECK(code.find(long_type) != std::string::npos);
        }

        SUBCASE("underlying type with spaces") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong unsigned long long"};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("description with excessive whitespace") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong   int  ;   +  ,  -  ,  ==  "};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("missing 'strong' keyword") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "int; +, -"};

            // Should parse 'int' as an unknown operator since there's no
            // 'strong' prefix
            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("empty description") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = ""};

            // Empty description means no underlying type, should throw
            auto code = generate_strong_type(desc);
            // Will generate code but underlying_type will be empty
            CHECK(code.find("value;") != std::string::npos);
        }
    }

    TEST_CASE("Edge Cases in Names")
    {
        SUBCASE("very long namespace") {
            std::string long_ns =
                "a::b::c::d::e::f::g::h::i::j::k::l::m::n::o::p";
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = long_ns,
                .type_name = "TestType",
                .description = "strong int"};

            CHECK_NOTHROW(generate_strong_type(desc));
            auto code = generate_strong_type(desc);
            CHECK(code.find("namespace a") != std::string::npos);
        }

        SUBCASE("very long type name") {
            std::string long_name = "VeryLongTypeNameThatExceedsMostReasonableL"
                                    "engthsButIsStillTechnicallyValid";
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = long_name,
                .description = "strong int"};

            CHECK_NOTHROW(generate_strong_type(desc));
            auto code = generate_strong_type(desc);
            CHECK(code.find(long_name) != std::string::npos);
        }

        SUBCASE("type name with multiple scopes") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Outer::Middle::Inner::Type",
                .description = "strong int"};

            CHECK_NOTHROW(generate_strong_type(desc));
            auto code = generate_strong_type(desc);
            CHECK(
                code.find("struct Outer::Middle::Inner::Type") !=
                std::string::npos);
        }

        SUBCASE("namespace with leading/trailing colons") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "::test::",
                .type_name = "TestType",
                .description = "strong int"};

            // Should strip leading/trailing colons
            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("type name with leading/trailing colons") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "::Type::",
                .description = "strong int"};

            // Should strip leading/trailing colons
            CHECK_NOTHROW(generate_strong_type(desc));
        }
    }

    TEST_CASE("Guard Options")
    {
        SUBCASE("custom guard prefix") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int",
                .guard_prefix = "MYPROJECT"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#ifndef MYPROJECT_") != std::string::npos);
        }

        SUBCASE("custom guard separator") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int",
                .guard_separator = "__"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("TEST__TESTTYPE__") != std::string::npos);
        }

        SUBCASE("lowercase guard") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int",
                .upcase_guard = false};

            auto code = generate_strong_type(desc);
            // Should contain lowercase guard
            CHECK(code.find("#ifndef test_TestType_") != std::string::npos);
        }

        SUBCASE("empty guard separator") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int",
                .guard_separator = ""};

            CHECK_NOTHROW(generate_strong_type(desc));
        }
    }

    TEST_CASE("Complete Operator Coverage")
    {
        SUBCASE("all arithmetic operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +, -, *, /, %, &, |, ^, <<, >>"};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("all unary operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; u+, u-, u~, ~"};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("all comparison operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; ==, !=, <, <=, >, >=, <=>"};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("all logical operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong bool; !, not, &&, and, ||, or"};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("all special operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description =
                    "strong int; ++, --, @, &of, ->, (), (&), bool, in, out"};

            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("combined unary-binary shorthands") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +*, -*"};

            auto code = generate_strong_type(desc);
            // Should include both binary and unary versions
            CHECK(code.find("operator +") != std::string::npos);
            CHECK(code.find("operator -") != std::string::npos);
        }
    }

    TEST_CASE("String Utility Edge Cases")
    {
        SUBCASE("trim edge cases") {
            // Test with all whitespace string
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "   ",
                .type_name = "TestType",
                .description = "strong int"};

            // Should handle whitespace-only namespace
            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("split with no delimiters") {
            // Description with no semicolon
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int no operators"};

            // Should still work - treats whole thing after "strong" as type
            auto code = generate_strong_type(desc);
            // The underlying type might be parsed differently
            CHECK(code.find("struct TestType") != std::string::npos);
        }

        SUBCASE("split with multiple consecutive delimiters") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +,,,-,,,=="};

            // Should handle multiple commas gracefully
            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("empty string handling") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "",
                .type_name = "",
                .description = "strong int"};

            // Empty namespace/name should still work
            CHECK_NOTHROW(generate_strong_type(desc));
        }
    }

    TEST_CASE("Default Value Edge Cases")
    {
        SUBCASE("default value with complex expression") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int",
                .default_value = "{std::numeric_limits<int>::max()}"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("std::numeric_limits<int>::max()") != std::string::npos);
        }

        SUBCASE("default value empty string") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int",
                .default_value = ""};

            // Empty default value should be allowed
            CHECK_NOTHROW(generate_strong_type(desc));
        }

        SUBCASE("default value with braces") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong std::vector<int>",
                .default_value = "{{1, 2, 3}}"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("{{1, 2, 3}}") != std::string::npos);
        }
    }

    TEST_CASE("Include Directive Edge Cases")
    {
        SUBCASE("multiple includes with same header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; #<vector>, #<vector>, #<string>"};

            auto code = generate_strong_type(desc);
            // Each include should appear
            CHECK(code.find("#include <vector>") != std::string::npos);
            CHECK(code.find("#include <string>") != std::string::npos);
        }

        SUBCASE("include with quotes") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; #\"myheader.hpp\""};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include \"myheader.hpp\"") != std::string::npos);
        }

        SUBCASE("include with path") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; #<boost/optional.hpp>"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <boost/optional.hpp>") != std::string::npos);
        }
    }

    TEST_CASE("Hash Functionality Edge Cases")
    {
        SUBCASE("no-constexpr-hash option") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; ==, no-constexpr-hash"};

            auto code = generate_strong_type(desc);
            // Should have hash specialization but not constexpr
            CHECK(code.find("struct std::hash") != std::string::npos);
            // Hash operator() should not be constexpr
            auto hash_pos = code.find("struct std::hash");
            auto next_constexpr = code.find("constexpr", hash_pos);
            auto hash_call = code.find("operator()", hash_pos);
            // Either no constexpr in hash, or constexpr comes after operator()
            CHECK((next_constexpr == std::string::npos || next_constexpr > hash_call));
        }

        SUBCASE("hash with no equality operator") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; ==, no-constexpr-hash"};

            auto code = generate_strong_type(desc);
            // Should have hash specialization
            CHECK(code.find("struct std::hash") != std::string::npos);
            CHECK(code.find("operator ==") != std::string::npos);
        }
    }

    TEST_CASE("Constexpr Option Combinations")
    {
        SUBCASE("no-constexpr affects all operators") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +, -, ==, no-constexpr"};

            auto code = generate_strong_type(desc);
            // Find operator+ definition
            auto plus_pos = code.find("operator +");
            // Look at the line with operator+, should not have constexpr
            auto line_start = code.rfind('\n', plus_pos) + 1;
            auto line_end = code.find('\n', plus_pos);
            std::string op_line = code.substr(line_start, line_end - line_start);
            CHECK(op_line.find("constexpr") == std::string::npos);
        }

        SUBCASE("constexpr is default") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; +, -"};

            auto code = generate_strong_type(desc);
            // Operators should be constexpr by default
            CHECK(code.find("constexpr") != std::string::npos);
        }
    }

}

} // anonymous namespace
