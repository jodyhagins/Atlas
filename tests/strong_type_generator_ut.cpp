// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "CodeStructureParser.hpp"
#include "StrongTypeGenerator.hpp"
#include "version.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"
#include "rapidcheck.hpp"

using namespace wjh::atlas::testing;
using wjh::atlas::testing::rc::check;

namespace {

using namespace wjh::atlas;

auto
make_description(
    std::string kind = "struct",
    std::string type_namespace = "test",
    std::string type_name = "TestType",
    std::string description = "strong int",
    std::string default_value = "",
    std::string guard_prefix = "",
    std::string guard_separator = "_",
    bool upcase_guard = true)
{
    return StrongTypeDescription{
        .kind = std::move(kind),
        .type_namespace = std::move(type_namespace),
        .type_name = std::move(type_name),
        .description = std::move(description),
        .default_value = std::move(default_value),
        .guard_prefix = std::move(guard_prefix),
        .guard_separator = std::move(guard_separator),
        .upcase_guard = upcase_guard};
}

// Helper function for simple code generation without warnings
std::string
generate_strong_type(StrongTypeDescription const & desc)
{
    StrongTypeGenerator gen;
    return gen(desc);
}

struct SplitCode
{
    std::string full_code;
    std::string preamble; // Everything before the marker
    std::string type_specific; // Everything after the marker
};

SplitCode
split_generated_code(std::string const & code)
{
    constexpr auto marker = "/// These are the droids you are looking for!";
    auto pos = code.find(marker);

    SplitCode result;
    result.full_code = code;
    if (pos != std::string::npos) {
        result.preamble = code.substr(0, pos);
        result.type_specific = code.substr(pos + std::strlen(marker));
    } else {
        // Fallback if marker not found
        result.type_specific = code;
    }
    return result;
}

TEST_SUITE("StrongTypeGenerator")
{
    TEST_CASE("Basic Structure Generation")
    {
        CodeStructureParser parser;

        SUBCASE("struct generation") {
            auto desc =
                make_description("struct", "test", "MyInt", "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Validate complete structure
            CHECK(structure.kind == "struct");
            CHECK(structure.type_name == "MyInt");
            CHECK(structure.namespace_name == "test");
            CHECK(structure.member_type == "int");
            CHECK(structure.member_name == "value");
            CHECK_FALSE(structure.member_default_value.has_value());

            // struct should NOT have public: or private: in type-specific code
            CHECK_FALSE(structure.has_public_specifier);
            CHECK_FALSE(structure.has_private_specifier);

            // Should have header guard
            CHECK_FALSE(structure.guard_name.empty());
            CHECK(structure.guard_name.find("TEST_MYINT") != std::string::npos);
        }

        SUBCASE("class generation") {
            auto desc = make_description(
                "class",
                "test::nested",
                "MyString",
                "strong std::string");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Validate complete structure
            CHECK(structure.kind == "class");
            CHECK(structure.type_name == "MyString");
            CHECK(structure.namespace_name == "test::nested");
            CHECK(structure.member_type == "std::string");

            // class MUST have public: specifier
            CHECK(structure.has_public_specifier);
            CHECK_FALSE(structure.has_private_specifier);

            // Should auto-include string header
            CHECK(structure.has_include("#include <string>"));
        }

        SUBCASE("namespace handling") {
            auto desc =
                make_description("struct", "a::b::c", "MyType", "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.namespace_name == "a::b::c");
            CHECK(structure.type_name == "MyType");
            CHECK(structure.full_qualified_name == "a::b::c::MyType");
        }
    }

    TEST_CASE("Header Guard Generation")
    {
        CodeStructureParser parser;

        SUBCASE("default guard generation") {
            auto desc =
                make_description("struct", "test", "MyType", "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Validate guard name contains expected parts
            CHECK_FALSE(structure.guard_name.empty());
            CHECK(structure.guard_name.find("TEST") != std::string::npos);
            CHECK(structure.guard_name.find("MYTYPE") != std::string::npos);
        }

        SUBCASE("custom guard prefix") {
            auto desc = make_description(
                "struct",
                "test",
                "MyType",
                "strong int",
                "", // default_value
                "CUSTOM"); // guard_prefix
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Should use CUSTOM prefix instead of namespace
            CHECK(structure.guard_name.find("CUSTOM") != std::string::npos);
            CHECK(structure.guard_name.find("TEST") == std::string::npos);
        }

        SUBCASE("custom guard separator") {
            auto desc = make_description(
                "struct",
                "test",
                "MyType",
                "strong int",
                "", // default_value
                "", // guard_prefix
                "__"); // guard_separator
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Should use __ as separator (double underscore)
            CHECK(
                structure.guard_name.find("TEST__MYTYPE__") !=
                std::string::npos);
        }

        SUBCASE("lowercase guard") {
            auto desc = make_description(
                "struct",
                "test",
                "MyType",
                "strong int",
                "", // default_value
                "", // guard_prefix
                "_", // guard_separator
                false); // upcase_guard
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Should preserve case (not all uppercase)
            CHECK(
                structure.guard_name.find("test_MyType_") != std::string::npos);
            // Should NOT be all uppercase
            CHECK(structure.guard_name != "TEST_MYTYPE_");
        }

        SUBCASE("header guard comes before NOTICE banner") {
            auto desc =
                make_description("struct", "test", "MyType", "strong int");
            auto code = generate_strong_type(desc);

            // Find first line
            auto first_line_end = code.find('\n');
            REQUIRE(first_line_end != std::string::npos);
            std::string first_line = code.substr(0, first_line_end);

            // First line must start with #ifndef
            CHECK(first_line.find("#ifndef") == 0);

            // Find second line
            auto second_line_start = first_line_end + 1;
            auto second_line_end = code.find('\n', second_line_start);
            REQUIRE(second_line_end != std::string::npos);
            std::string second_line = code.substr(
                second_line_start,
                second_line_end - second_line_start);

            // Second line must start with #define
            CHECK(second_line.find("#define") == 0);

            // NOTICE banner should come after header guard (not on first line)
            auto notice_pos = code.find("NOTICE");
            REQUIRE(notice_pos != std::string::npos);
            CHECK(notice_pos > second_line_end);
        }
    }

    TEST_CASE("Arithmetic Operators")
    {
        CodeStructureParser parser;

        SUBCASE("binary arithmetic operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; +, -, *, /, %");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            for (auto op : {"+", "-", "*", "/", "%"}) {
                // Check for compound assignment operator (friend function)
                auto compound = structure.find_operator(
                    std::string("operator ") + op + "=");
                REQUIRE(compound.has_value());
                CHECK(compound->name == std::string("operator ") + op + "=");
                CHECK(compound
                          ->is_friend); // friend function per template line 217
                CHECK(compound->is_constexpr); // should be constexpr by default

                // Check for binary operator (friend function)
                // We should find 2 instances: one is the binary operator itself
                auto all_ops = structure.find_all_operators(
                    std::string("operator ") + op);
                REQUIRE(
                    all_ops.size() ==
                    2); // Should have both compound and binary

                // Both should be friend and constexpr
                for (auto const & bin_op : all_ops) {
                    CHECK(bin_op.is_friend);
                    CHECK(bin_op.is_constexpr);
                }
            }
        }

        SUBCASE("unary arithmetic operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; u+, u-, u~");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Unary operators should be friend functions
            for (auto op : {"+", "-", "~"}) {
                auto unary = structure.find_operator(
                    std::string("operator ") + op);
                REQUIRE(unary.has_value());
                CHECK(unary->is_friend); // friend function
                CHECK(unary->is_constexpr);
            }
        }

        SUBCASE("combined binary and unary operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; +*, -*");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Should have compound, binary, AND unary forms for +* and -*
            auto plus_ops = structure.find_all_operators("operator +");
            REQUIRE(plus_ops.size() >= 2); // At least binary and unary

            auto plus_eq = structure.find_operator("operator +=");
            REQUIRE(plus_eq.has_value());
            CHECK(plus_eq->is_friend); // compound is friend function

            auto minus_ops = structure.find_all_operators("operator -");
            REQUIRE(minus_ops.size() >= 2); // At least binary and unary

            auto minus_eq = structure.find_operator("operator -=");
            REQUIRE(minus_eq.has_value());
            CHECK(minus_eq->is_friend); // compound is friend function
        }

        SUBCASE("bitwise operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Bits",
                "strong int; &, |, ^, <<, >>");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            for (auto op : {"&", "|", "^", "<<", ">>"}) {
                // Check for compound assignment operator (friend function)
                auto compound = structure.find_operator(
                    std::string("operator ") + op + "=");
                REQUIRE(compound.has_value());
                CHECK(compound->is_friend); // friend function
                CHECK(compound->is_constexpr);

                // Check for binary operator (both forms should exist)
                auto all_ops = structure.find_all_operators(
                    std::string("operator ") + op);
                REQUIRE(all_ops.size() == 2); // compound and binary

                // Both should be friend and constexpr
                for (auto const & bin_op : all_ops) {
                    CHECK(bin_op.is_friend);
                    CHECK(bin_op.is_constexpr);
                }
            }
        }
    }

    TEST_CASE("Comparison Operators")
    {
        CodeStructureParser parser;

        SUBCASE("relational operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; ==, !=, <, <=, >, >=");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            for (auto op : {"==", "!=", "<", "<=", ">", ">="}) {
                // All comparison operators should be friend functions
                auto cmp_op = structure.find_operator(
                    std::string("operator ") + op);
                REQUIRE(cmp_op.has_value());
                CHECK(cmp_op->is_friend);
                CHECK(cmp_op->is_constexpr);
            }
        }

        SUBCASE("spaceship operator") {
            auto desc =
                make_description("struct", "test", "Number", "strong int; <=>");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            auto spaceship = structure.find_operator("operator <=>");
            REQUIRE(spaceship.has_value());
            CHECK(spaceship->is_friend);
            CHECK(spaceship->is_default);
        }
    }

    TEST_CASE("Special Operators")
    {
        CodeStructureParser parser;

        SUBCASE("increment/decrement operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int; ++, --");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            auto inc = structure.find_operator("operator ++");
            REQUIRE(inc.has_value());
            CHECK(inc->is_friend);
            CHECK(inc->is_constexpr);

            auto dec = structure.find_operator("operator --");
            REQUIRE(dec.has_value());
            CHECK(dec->is_friend);
            CHECK(dec->is_constexpr);
        }

        SUBCASE("indirection operator") {
            auto desc =
                make_description("struct", "test", "Ptr", "strong int*; @");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            auto deref = structure.find_operator("operator *");
            REQUIRE(deref.has_value());
            CHECK(deref->is_constexpr);
        }

        SUBCASE("address-of operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Ref",
                "strong int; &of, ->");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            auto addr = structure.find_operator("operator &");
            REQUIRE(addr.has_value());
            CHECK(addr->is_constexpr);

            auto arrow = structure.find_operator("operator ->");
            REQUIRE(arrow.has_value());
            CHECK(arrow->is_constexpr);
        }

        SUBCASE("call operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Callable",
                "strong int; (), (&)");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Both call operators should exist
            auto call_ops = structure.find_all_operators("operator ()");
            REQUIRE(
                call_ops.size() >= 1); // At least nullary or template version
        }

        SUBCASE("bool conversion") {
            CodeStructureParser parser;
            auto desc = make_description(
                "struct",
                "test",
                "BoolConv",
                "strong int; bool");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Bool conversion is a special operator
            auto bool_op = structure.find_operator("operator bool");
            CHECK(bool_op.has_value());
        }
    }

    TEST_CASE("Stream Operators")
    {
        CodeStructureParser parser;

        SUBCASE("output stream operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Printable",
                "strong int; out");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_include("#include <ostream>"));
            auto stream_op = structure.find_operator("operator <<");
            CHECK(stream_op.has_value());
            CHECK(stream_op->is_friend);
        }

        SUBCASE("input stream operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Readable",
                "strong int; in");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_include("#include <istream>"));
            auto stream_op = structure.find_operator("operator >>");
            CHECK(stream_op.has_value());
            CHECK(stream_op->is_friend);
        }

        SUBCASE("both stream operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Streamable",
                "strong int; in, out");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_include("#include <istream>"));
            CHECK(structure.has_include("#include <ostream>"));
            CHECK(structure.find_operator("operator <<").has_value());
            CHECK(structure.find_operator("operator >>").has_value());
        }
    }

    TEST_CASE("Automatic Header Detection")
    {
        CodeStructureParser parser;

        SUBCASE("standard string types") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Str",
                "strong std::string"));
            CHECK(parser.parse(code).has_include("#include <string>"));
        }

        SUBCASE("standard containers") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Vec",
                "strong std::vector<int>"));
            CHECK(parser.parse(code).has_include("#include <vector>"));
        }

        SUBCASE("standard optional") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Opt",
                "strong std::optional<int>"));
            CHECK(parser.parse(code).has_include("#include <optional>"));
        }

        SUBCASE("chrono types") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Time",
                "strong std::chrono::seconds"));
            CHECK(parser.parse(code).has_include("#include <chrono>"));
        }
    }

    TEST_CASE("Custom Headers")
    {
        CodeStructureParser parser;

        SUBCASE("custom header with quotes") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Custom",
                "strong MyType; #\"my/header.hpp\""));
            CHECK(parser.parse(code).has_include("#include \"my/header.hpp\""));
        }

        SUBCASE("custom header with single quotes") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Custom",
                "strong MyType; #'my/header.hpp'"));
            CHECK(parser.parse(code).has_include("#include \"my/header.hpp\""));
        }

        SUBCASE("custom header with angle brackets") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Custom",
                "strong MyType; #<system/header>"));
            CHECK(parser.parse(code).has_include("#include <system/header>"));
        }
    }

    TEST_CASE("Default Value Support")
    {
        CodeStructureParser parser;

        SUBCASE("integer default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Counter",
                "strong int",
                "42"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == "42");
        }

        SUBCASE("double default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Pi",
                "strong double",
                "3.14159"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == "3.14159");
        }

        SUBCASE("string default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Name",
                "strong std::string",
                R"("hello")"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == R"("hello")");
        }

        SUBCASE("complex expression default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Numbers",
                "strong std::vector<int>",
                "std::vector<int>{1, 2, 3}"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(
                *structure.member_default_value == "std::vector<int>{1, 2, 3}");
        }

        SUBCASE("string with commas") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "CommaString",
                "strong std::string",
                R"("42,43")"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == R"("42,43")");
        }

        SUBCASE("default value with other operators") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Score",
                "strong int; +, -, ==, !=",
                "100"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == "100");

            // Verify operators also present
            CHECK(structure.find_operator("operator +=").has_value());
            CHECK(structure.find_operator("operator -=").has_value());
            CHECK(structure.find_operator("operator ==").has_value());
            CHECK(structure.find_operator("operator !=").has_value());
        }

        SUBCASE("no default value uses default construction") {
            auto code = generate_strong_type(
                make_description("struct", "test", "Regular", "strong int"));
            auto structure = parser.parse(code);

            CHECK_FALSE(structure.member_default_value.has_value());
            CHECK(structure.member_type == "int");
        }

        SUBCASE("negative default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Negative",
                "strong int",
                "-42"));
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == "-42");
        }
    }

    TEST_CASE("Default Initialization Code Generation")
    {
        CodeStructureParser parser;

        SUBCASE("explicit non-zero default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithDefault",
                "strong int; ==",
                "42"));

            // Verify the generated code contains the correct initialization
            CHECK(code.find("int value{42};") != std::string::npos);
        }

        SUBCASE("explicit zero default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithZero",
                "strong int; ==",
                "0"));

            // Verify zero is explicitly initialized
            CHECK(code.find("int value{0};") != std::string::npos);
        }

        SUBCASE("missing default value has no initializer") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithoutDefault",
                "strong int; =="));

            // Should have declaration without initializer
            CHECK(code.find("int value;") != std::string::npos);
            // Should NOT have brace initialization
            CHECK(code.find("int value{") == std::string::npos);
        }

        SUBCASE("empty string default value has no initializer") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithEmptyString",
                "strong int; ==",
                ""));

            // Empty default_value should behave like missing
            CHECK(code.find("int value;") != std::string::npos);
            CHECK(code.find("int value{") == std::string::npos);
        }

        SUBCASE("string type with default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithStringDefault",
                "strong std::string; ==",
                R"("hello")"));

            // Verify string initialization
            CHECK(
                code.find(R"(std::string value{"hello"};)") !=
                std::string::npos);
        }

        SUBCASE("complex initializer with braces") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithComplexDefault",
                "strong std::vector<int>; ==",
                "{1, 2, 3}"));

            // Verify complex initialization (note double braces in generated
            // code)
            CHECK(
                code.find("std::vector<int> value{{1, 2, 3}};") !=
                std::string::npos);
        }

        SUBCASE("double type with decimal default") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Money",
                "strong double",
                "0.0"));

            CHECK(code.find("double value{0.0};") != std::string::npos);
        }

        SUBCASE("denominator with non-zero default") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "Denominator",
                "strong long",
                "1"));

            // Verify non-zero default is honored
            CHECK(code.find("long value{1};") != std::string::npos);
            // Should NOT default to zero
            CHECK(code.find("long value{0}") == std::string::npos);
        }
    }

    TEST_CASE("Error Handling")
    {
        CodeStructureParser parser;

        SUBCASE("invalid kind throws exception") {
            auto desc =
                make_description("invalid", "test", "Bad", "strong int");

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("empty description still generates basic structure") {
            auto code = generate_strong_type(
                make_description("struct", "test", "Empty", "strong int; "));
            auto structure = parser.parse(code);

            CHECK(structure.kind == "struct");
            CHECK(structure.type_name == "Empty");
            CHECK(structure.member_type == "int");
            CHECK(structure.member_name == "value");
        }
    }

    TEST_CASE("Complex Type Descriptions")
    {
        CodeStructureParser parser;

        SUBCASE("all operators together") {
            auto desc = make_description(
                "struct",
                "test",
                "Complete",
                "strong int; +, -, *, /, %, &, |, ^, <<, >>, ==, !=, <, <=, >, "
                ">=, <=>, ++, --, bool, out, in");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Structural validation - check operators are generated
            CHECK(structure.find_operator("operator +").has_value());
            CHECK(structure.find_operator("operator ==").has_value());
            CHECK(structure.find_operator("operator <=>").has_value());
            CHECK(structure.find_operator("operator ++").has_value());

            // Check includes are present
            CHECK(structure.has_include("#include <ostream>"));
            CHECK(structure.has_include("#include <istream>"));
        }

        SUBCASE("nested namespace and complex type") {
            auto desc = make_description(
                "class",
                "company::product::module",
                "Container::Element",
                "strong std::unique_ptr<Data>; ->, bool, ==, !=");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Structural validation
            CHECK(structure.namespace_name == "company::product::module");
            CHECK(structure.kind == "class");
            CHECK(structure.type_name == "Container::Element");
            CHECK(structure.member_type == "std::unique_ptr<Data>");
            CHECK(structure.member_name == "value");
        }
    }

    TEST_CASE("Hash Support")
    {
        CodeStructureParser parser;

        SUBCASE("hash with int type") {
            auto desc = make_description(
                "struct",
                "test",
                "HashableInt",
                "strong int; ==, hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Structural validation - semantic, not syntactic
            CHECK(structure.has_hash_specialization);
            CHECK(structure.hash_is_constexpr);
            CHECK(structure.has_include("#include <functional>"));
            CHECK(structure.type_name == "HashableInt");
            CHECK(structure.namespace_name == "test");
        }

        SUBCASE("hash with string type") {
            auto desc = make_description(
                "struct",
                "test",
                "HashableString",
                "strong std::string; ==, hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_hash_specialization);
            CHECK(structure.hash_is_constexpr);
            CHECK(structure.has_include("#include <functional>"));
            CHECK(structure.has_include("#include <string>"));
        }

        SUBCASE("hash with namespaced type") {
            auto desc = make_description(
                "struct",
                "my::deep::ns",
                "HashableValue",
                "strong unsigned; ==, hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_hash_specialization);
            CHECK(structure.namespace_name == "my::deep::ns");
            CHECK(structure.type_name == "HashableValue");
            CHECK(
                structure.full_qualified_name == "my::deep::ns::HashableValue");
        }

        SUBCASE("no hash without explicit option") {
            auto desc =
                make_description("struct", "test", "NoHash", "strong int; ==");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK_FALSE(structure.has_hash_specialization);
        }

        SUBCASE("no-constexpr-hash generates hash without constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "RuntimeHash",
                "strong int; ==, no-constexpr-hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Has hash but NOT constexpr
            CHECK(structure.has_hash_specialization);
            CHECK_FALSE(structure.hash_is_constexpr);
            CHECK(structure.has_include("#include <functional>"));
        }

        SUBCASE("regular hash has constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "ConstexprHash",
                "strong int; ==, hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_hash_specialization);
            CHECK(structure.hash_is_constexpr);
        }

        SUBCASE("no-constexpr removes constexpr from hash too") {
            auto desc = make_description(
                "struct",
                "test",
                "NoConstexprWithHash",
                "strong int; ==, hash, no-constexpr");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Hash exists but without constexpr
            CHECK(structure.has_hash_specialization);
            CHECK_FALSE(structure.hash_is_constexpr);
            // Type should also not have constexpr constructor
            CHECK_FALSE(structure.has_constexpr_constructor);
        }
    }

    TEST_CASE("Constexpr Code Generation")
    {
        CodeStructureParser parser;

        SUBCASE("default constexpr on all operations") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, -, ==, !=, bool");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Constructor should be constexpr by default
            CHECK(structure.has_constexpr_constructor);

            // Operators should be constexpr
            auto plus_eq = structure.find_operator("operator +=");
            REQUIRE(plus_eq.has_value());
            CHECK(plus_eq->is_constexpr);

            auto minus_eq = structure.find_operator("operator -=");
            REQUIRE(minus_eq.has_value());
            CHECK(minus_eq->is_constexpr);

            auto eq_op = structure.find_operator("operator ==");
            REQUIRE(eq_op.has_value());
            CHECK(eq_op->is_constexpr);
        }

        SUBCASE("no-constexpr removes all constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, -, ==, !=, bool, no-constexpr");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Constructor should NOT be constexpr
            CHECK_FALSE(structure.has_constexpr_constructor);

            // Operators should NOT be constexpr
            auto plus_eq = structure.find_operator("operator +=");
            REQUIRE(plus_eq.has_value());
            CHECK_FALSE(plus_eq->is_constexpr);

            auto eq_op = structure.find_operator("operator ==");
            REQUIRE(eq_op.has_value());
            CHECK_FALSE(eq_op->is_constexpr);
        }

        SUBCASE("stream operators are never constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; in, out");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Constructor should be constexpr (streams don't affect it)
            CHECK(structure.has_constexpr_constructor);

            // Note: Stream operators won't be in the operator list
            // since they're specialized (ostream&, istream&)
            // Just verify includes are present
            CHECK(structure.has_include("#include <ostream>"));
            CHECK(structure.has_include("#include <istream>"));
        }

        SUBCASE("no-constexpr-hash leaves operations constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, ==, no-constexpr-hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Operations should have constexpr
            CHECK(structure.has_constexpr_constructor);

            auto plus_eq = structure.find_operator("operator +=");
            REQUIRE(plus_eq.has_value());
            CHECK(plus_eq->is_constexpr);

            auto eq_op = structure.find_operator("operator ==");
            REQUIRE(eq_op.has_value());
            CHECK(eq_op->is_constexpr);

            // Hash should NOT have constexpr
            CHECK(structure.has_hash_specialization);
            CHECK_FALSE(structure.hash_is_constexpr);
        }
    }

    TEST_CASE("Multi-Type File Generation")
    {
        SUBCASE("preamble appears exactly once") {
            // Create multiple type descriptions
            std::vector<StrongTypeDescription> descriptions = {
                make_description("struct", "test", "Type1", "strong int; +, -"),
                make_description(
                    "struct",
                    "test",
                    "Type2",
                    "strong double; *, /"),
                make_description(
                    "struct",
                    "test",
                    "Type3",
                    "strong std::string; ==, !=")};

            auto code =
                generate_strong_types_file(descriptions, "EXAMPLE", "_", true);

            // The preamble guard should appear exactly 3 times: #ifndef,
            // #define, and #endif
            std::string preamble_guard =
                "WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90";

            size_t count = 0;
            size_t pos = 0;
            while ((pos = code.find(preamble_guard, pos)) != std::string::npos)
            {
                ++count;
                pos += preamble_guard.length();
            }

            // Should appear exactly 3 times: #ifndef, #define at start, and
            // #endif at end
            CHECK(count == 3);

            // Verify the preamble marker appears exactly once
            std::string preamble_marker =
                "These are the droids you are looking for!";
            count = 0;
            pos = 0;
            while ((pos = code.find(preamble_marker, pos)) != std::string::npos)
            {
                ++count;
                pos += preamble_marker.length();
            }
            CHECK(count == 1);

            // Verify strong_type_tag is defined exactly once
            std::string strong_type_tag = "struct strong_type_tag";
            count = 0;
            pos = 0;
            while ((pos = code.find(strong_type_tag, pos)) != std::string::npos)
            {
                ++count;
                pos += strong_type_tag.length();
            }
            CHECK(count == 1);
        }

        SUBCASE("all types are present in generated file") {
            std::vector<StrongTypeDescription> descriptions = {
                make_description("struct", "ns1", "TypeA", "strong int"),
                make_description("struct", "ns2", "TypeB", "strong double"),
                make_description("struct", "ns3", "TypeC", "strong float")};

            auto code = generate_strong_types_file(descriptions, "", "_", true);

            // Verify all three types are present
            CHECK(code.find("struct TypeA") != std::string::npos);
            CHECK(code.find("struct TypeB") != std::string::npos);
            CHECK(code.find("struct TypeC") != std::string::npos);

            // Verify all three namespaces are present
            CHECK(code.find("namespace ns1") != std::string::npos);
            CHECK(code.find("namespace ns2") != std::string::npos);
            CHECK(code.find("namespace ns3") != std::string::npos);
        }
    }

    TEST_CASE("Property-Based Tests")
    {
        SUBCASE("all generated guards are unique") {
            check(
                "Guards are unique for different types",
                [](int seed1, int seed2) {
                    RC_PRE(seed1 != seed2);

                    auto name1 = "Type" +
                        std::to_string(std::abs(seed1) % 1000);
                    auto name2 = "Type" +
                        std::to_string(std::abs(seed2) % 1000);

                    if (name1 == name2) {
                        return;
                    }

                    auto desc1 =
                        make_description("struct", "test", name1, "strong int");
                    auto desc2 =
                        make_description("struct", "test", name2, "strong int");

                    auto code1 = generate_strong_type(desc1);
                    auto code2 = generate_strong_type(desc2);

                    std::regex guard_regex{R"(#ifndef\s+([A-Z_0-9]+))"};
                    std::smatch match1, match2;

                    RC_ASSERT(std::regex_search(code1, match1, guard_regex));
                    RC_ASSERT(std::regex_search(code2, match2, guard_regex));
                    RC_ASSERT(match1[1] != match2[1]);
                });
        }

        SUBCASE("operator selection is consistent") {
            CodeStructureParser parser;
            check("Requested operators appear in generated code", [&parser]() {
                auto operators = *::rc::gen::container<
                    std::vector<std::string>>(
                    ::rc::gen::element("+", "-", "*", "==", "!=", "++", "out"));

                if (operators.empty()) {
                    return;
                }

                std::string desc_str = "strong int";
                for (size_t i = 0; i < operators.size(); ++i) {
                    if (i == 0) {
                        desc_str += "; ";
                    } else {
                        desc_str += ", ";
                    }
                    desc_str += operators[i];
                }

                auto desc =
                    make_description("struct", "test", "TestOp", desc_str);
                auto code = generate_strong_type(desc);
                auto structure = parser.parse(code);

                for (auto const & op : operators) {
                    if (op == "out") {
                        RC_ASSERT(structure.has_include("#include <ostream>"));
                    } else {
                        RC_ASSERT(structure.find_operator("operator " + op)
                                      .has_value());
                    }
                }
            });
        }
    }

    TEST_CASE("C++11 Compatibility")
    {
        CodeStructureParser parser;

        SUBCASE("type traits use C++11 syntax") {
            auto desc = make_description(
                "struct",
                "test",
                "TypeTraitsTest",
                "strong int");
            auto code = generate_strong_type(desc);

            // Should use std::enable_if<...>::type not std::enable_if_t<...>
            CHECK(code.find("std::enable_if_t") == std::string::npos);
            CHECK(code.find("typename std::enable_if<") != std::string::npos);
            CHECK(code.find(">::type") != std::string::npos);

            // Should use std::is_constructible<...>::value not
            // std::is_constructible_v<...>
            CHECK(code.find("std::is_constructible_v") == std::string::npos);
            CHECK(code.find("std::is_constructible<") != std::string::npos);
            CHECK(code.find(">::value") != std::string::npos);
        }

        SUBCASE("subscript operator uses C++11 trailing return type") {
            auto desc = make_description(
                "struct",
                "test",
                "SubscriptTest",
                "strong std::vector<int>; []");
            auto code = generate_strong_type(desc);

            // Should NOT use decltype(auto) which is C++14
            // Look in the #else branch (non-C++23) for the single-argument
            // version
            auto subscript_if = code.find("__cpp_multidimensional_subscript");
            REQUIRE(subscript_if != std::string::npos);
            auto else_pos = code.find("#else", subscript_if);
            REQUIRE(else_pos != std::string::npos);
            auto endif_pos = code.find("#endif", else_pos);
            REQUIRE(endif_pos != std::string::npos);
            std::string cpp11_section = code.substr(
                else_pos,
                endif_pos - else_pos);

            // Should not have decltype(auto) in C++11 section
            CHECK(cpp11_section.find("decltype(auto)") == std::string::npos);

            // Should have trailing return type: auto ... -> decltype(...)
            CHECK(cpp11_section.find("auto operator []") != std::string::npos);
            CHECK(
                cpp11_section.find("-> decltype(value[") != std::string::npos);
        }

        SUBCASE("callable operator has C++11 fallback") {
            auto desc = make_description(
                "struct",
                "test",
                "CallableTest",
                "strong int; (&)");
            auto code = generate_strong_type(desc);

            // Should have feature detection for std::invoke
            CHECK(code.find("__cpp_lib_invoke") != std::string::npos);
            CHECK(
                code.find("#if defined(__cpp_lib_invoke)") !=
                std::string::npos);

            // Should have C++17 path with std::invoke
            CHECK(code.find("std::invoke") != std::string::npos);

            // Should have C++11 fallback with direct call
            auto else_pos = code.find("#else", code.find("__cpp_lib_invoke"));
            REQUIRE(else_pos != std::string::npos);
            auto endif_pos = code.find("#endif", else_pos);
            REQUIRE(endif_pos != std::string::npos);
            std::string cpp11_section = code.substr(
                else_pos,
                endif_pos - else_pos);

            // C++11 section should have direct call inv(value)
            CHECK(cpp11_section.find("inv)(value)") != std::string::npos);
            // And should NOT have std::invoke in the fallback
            CHECK(cpp11_section.find("std::invoke") == std::string::npos);
        }

        SUBCASE("nested namespaces use C++11 syntax") {
            auto desc = make_description(
                "struct",
                "foo::bar::baz",
                "NestedTest",
                "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Parser should extract full nested namespace
            CHECK(structure.namespace_name == "foo::bar::baz");
            CHECK(structure.full_qualified_name == "foo::bar::baz::NestedTest");

            // Should use C++11 nested syntax, not C++17 inline syntax
            CHECK(code.find("namespace foo::bar::baz {") == std::string::npos);

            // Should have properly nested C++11 declarations
            CHECK(code.find("namespace foo {") != std::string::npos);
            CHECK(code.find("namespace bar {") != std::string::npos);
            CHECK(code.find("namespace baz {") != std::string::npos);

            // Should have properly nested C++11 closings in reverse order
            auto baz_close = code.find("} // namespace baz");
            auto bar_close = code.find("} // namespace bar");
            auto foo_close = code.find("} // namespace foo");
            REQUIRE(baz_close != std::string::npos);
            REQUIRE(bar_close != std::string::npos);
            REQUIRE(foo_close != std::string::npos);
            CHECK(baz_close < bar_close);
            CHECK(bar_close < foo_close);
        }

        SUBCASE("single namespace works correctly") {
            auto desc = make_description(
                "struct",
                "single",
                "SingleTest",
                "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.namespace_name == "single");
            CHECK(structure.full_qualified_name == "single::SingleTest");
            CHECK(code.find("namespace single {") != std::string::npos);
            CHECK(code.find("} // namespace single") != std::string::npos);
        }

        SUBCASE("no namespace works correctly") {
            auto desc =
                make_description("struct", "", "NoNamespaceTest", "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.namespace_name.empty());
            CHECK(structure.full_qualified_name == "NoNamespaceTest");
            // Should not have any namespace declarations
            auto droids = code.find("/// These are the droids");
            auto first_ns = code.find("namespace ", droids);
            auto first_struct = code.find("struct NoNamespaceTest", droids);
            // Either no namespace, or struct comes before any namespace
            CHECK((first_ns == std::string::npos || first_struct < first_ns));
        }
    }

    TEST_CASE("Version Information")
    {
        SUBCASE("version constants are defined") {
            CHECK(wjh::atlas::codegen::version_major >= 0);
            CHECK(wjh::atlas::codegen::version_minor >= 0);
            CHECK(wjh::atlas::codegen::version_patch >= 0);
        }

        SUBCASE("version string format is correct") {
            std::string version = wjh::atlas::codegen::version_string;

            // Should be in format "MAJOR.MINOR.PATCH"
            auto first_dot = version.find('.');
            auto last_dot = version.rfind('.');

            CHECK(first_dot != std::string::npos);
            CHECK(last_dot != std::string::npos);
            CHECK(first_dot != last_dot);
        }

        SUBCASE("generated code includes version") {
            auto desc =
                make_description("struct", "test", "TestType", "strong int");
            auto code = generate_strong_type(desc);

            // Should contain version information in header comment
            CHECK(
                code.find("Atlas Strong Type Generator v") !=
                std::string::npos);
            CHECK(
                code.find(wjh::atlas::codegen::version_string) !=
                std::string::npos);
        }

        SUBCASE("generated code includes DO NOT EDIT warning") {
            auto desc =
                make_description("struct", "test", "TestType", "strong int");
            auto code = generate_strong_type(desc);

            CHECK(code.find("DO NOT EDIT") != std::string::npos);
        }

        SUBCASE("multi-file generation includes version") {
            std::vector<StrongTypeDescription> descriptions = {
                make_description("struct", "test", "Type1", "strong int"),
                make_description("struct", "test", "Type2", "strong double")};
            auto code = generate_strong_types_file(descriptions);

            // Should contain version in the banner
            CHECK(
                code.find("Atlas Strong Type Generator v") !=
                std::string::npos);
            CHECK(
                code.find(wjh::atlas::codegen::version_string) !=
                std::string::npos);
        }
    }

    TEST_CASE("Warning System")
    {
        SUBCASE("no warnings for spaceship alone") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            CHECK(warnings.empty());
        }

        SUBCASE("warning for spaceship with equality operators") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, !=, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE(warnings.size() >= 1);
            bool found_equality_warning = false;
            for (auto const & w : warnings) {
                if (w.message.find("'==' and '!='") != std::string::npos &&
                    w.message.find("redundant") != std::string::npos)
                {
                    found_equality_warning = true;
                    CHECK(w.type_name == "test::TestType");
                }
            }
            CHECK(found_equality_warning);
        }

        SUBCASE("warning for spaceship with relational operators") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; <, <=, >, >=, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE(warnings.size() >= 1);
            bool found_relational_warning = false;
            for (auto const & w : warnings) {
                if (w.message.find("'<', '<=', '>', '>='") !=
                        std::string::npos &&
                    w.message.find("redundant") != std::string::npos)
                {
                    found_relational_warning = true;
                    CHECK(w.type_name == "test::TestType");
                }
            }
            CHECK(found_relational_warning);
        }

        SUBCASE("both warnings for spaceship with all comparison operators") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, !=, <, <=, >, >=, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE(warnings.size() == 2);
        }

        SUBCASE("no warning when only == != specified without spaceship") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, !=");
            gen(desc);
            auto warnings = gen.get_warnings();

            CHECK(warnings.empty());
        }

        SUBCASE("no warning when only relational operators without spaceship") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; <, <=, >, >=");
            gen(desc);
            auto warnings = gen.get_warnings();

            CHECK(warnings.empty());
        }

        SUBCASE("warning includes correct type name with namespace") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "my::nested::namespace",
                "MyType",
                "strong double; ==, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE_FALSE(warnings.empty());
            CHECK(warnings[0].type_name == "my::nested::namespace::MyType");
        }

        SUBCASE("warning includes correct type name without namespace") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "",
                "GlobalType",
                "strong int; <, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE_FALSE(warnings.empty());
            CHECK(warnings[0].type_name == "GlobalType");
        }

        SUBCASE("clear_warnings() clears collected warnings") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, <=>");
            gen(desc);
            CHECK_FALSE(gen.get_warnings().empty());

            gen.clear_warnings();
            CHECK(gen.get_warnings().empty());
        }

        SUBCASE("warnings accumulate across multiple generations") {
            StrongTypeGenerator gen;

            auto desc1 = make_description(
                "struct",
                "test",
                "Type1",
                "strong int; ==, <=>");
            gen(desc1);
            CHECK(gen.get_warnings().size() == 1);

            auto desc2 = make_description(
                "struct",
                "test",
                "Type2",
                "strong int; <, <=>");
            gen(desc2);
            CHECK(gen.get_warnings().size() == 2);
        }
    }
}

} // anonymous namespace
