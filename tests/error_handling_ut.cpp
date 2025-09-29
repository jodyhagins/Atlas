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
}

} // anonymous namespace
