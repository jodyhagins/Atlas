// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file GenerationPipeline_ut.cpp
 * Integration tests for the entire code generation pipeline
 *
 * These tests verify that all components of the generation architecture
 * work together correctly:
 * - TemplateRegistry: Registers and retrieves templates
 * - ITemplate implementations: Operators, features, specializations
 * - TemplateOrchestrator: Coordinates rendering
 * - ClassInfo: Data model for code generation
 * - GuardGenerator: Header guards and SHA1 hashing
 * - OperatorParser: Parses description strings
 *
 * Unlike unit tests which test components in isolation, these tests
 * verify the end-to-end pipeline from StrongTypeDescription to
 * generated code.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/GuardGenerator.hpp"
#include "atlas/generation/core/TemplateOrchestrator.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

#include <algorithm>
#include <regex>
#include <string>
#include <vector>

#include "tests/doctest.hpp"

namespace {

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Create a StrongTypeDescription with sensible defaults for testing
 */
StrongTypeDescription
make_description(
    std::string kind = "struct",
    std::string type_namespace = "test",
    std::string type_name = "TestType",
    std::string description = "strong int",
    std::string default_value = "",
    int cpp_standard = 20)
{
    StrongTypeDescription desc;
    desc.kind = std::move(kind);
    desc.type_namespace = std::move(type_namespace);
    desc.type_name = std::move(type_name);
    desc.description = std::move(description);
    desc.default_value = std::move(default_value);
    desc.cpp_standard = cpp_standard;
    desc.guard_prefix = "";
    desc.guard_separator = "_";
    desc.upcase_guard = true;
    return desc;
}

/**
 * Generate code using StrongTypeGenerator
 */
std::string
generate(StrongTypeDescription const & desc)
{
    StrongTypeGenerator gen;
    return gen(desc);
}

/**
 * Check if a string contains a substring
 */
bool
contains(std::string_view text, std::string_view substring)
{
    return text.find(substring) != std::string_view::npos;
}

/**
 * Check if string contains all substrings
 */
bool
contains_all(
    std::string_view text,
    std::initializer_list<std::string_view> substrings)
{
    return std::all_of(
        substrings.begin(),
        substrings.end(),
        [&](auto const & s) { return contains(text, s); });
}

/**
 * Count occurrences of a substring
 */
size_t
count_occurrences(std::string_view text, std::string_view substring)
{
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(substring, pos)) != std::string_view::npos) {
        ++count;
        pos += substring.length();
    }
    return count;
}

/**
 * Extract all lines matching a regex pattern
 */
std::vector<std::string>
extract_matching_lines(std::string const & text, std::string const & pattern)
{
    std::vector<std::string> matches;
    std::istringstream stream(text);
    std::string line;
    std::regex regex(pattern);

    while (std::getline(stream, line)) {
        if (std::regex_search(line, regex)) {
            matches.push_back(line);
        }
    }

    return matches;
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_SUITE("Code Generation Pipeline Integration")
{
    TEST_CASE("End-to-End: Simple arithmetic type")
    {
        auto desc = make_description(
            "struct",
            "test",
            "Distance",
            "strong double; +, -, *, /");

        auto code = generate(desc);

        SUBCASE("generates valid structure") {
            CHECK(contains_all(
                code,
                {"struct Distance", "double value", "test::"}));
        }

        SUBCASE("generates all requested operators") {
            CHECK(contains_all(
                code,
                {"operator+", "operator-", "operator*", "operator/"}));
        }

        SUBCASE("generates correct header guards") {
            CHECK(contains(code, "#ifndef TEST_DISTANCE_HPP"));
            CHECK(contains(code, "#define TEST_DISTANCE_HPP"));
            CHECK(contains(code, "#endif // TEST_DISTANCE_HPP"));
        }

        SUBCASE("includes required headers") {
            // Arithmetic operators need <limits>
            CHECK(contains(code, "#include <limits>"));
        }

        SUBCASE("generates constexpr by default") {
            // Check that operators are constexpr
            auto lines = extract_matching_lines(code, R"(operator[+\-*/])");
            CHECK(lines.size() > 0);
            for (auto const & line : lines) {
                CHECK(contains(line, "constexpr"));
            }
        }
    }

    TEST_CASE("End-to-End: Comparison operators")
    {
        auto desc = make_description(
            "struct",
            "test",
            "Score",
            "strong int; ==, !=, <, <=, >, >=");

        auto code = generate(desc);

        SUBCASE("generates relational operators") {
            CHECK(contains_all(
                code,
                {"operator==",
                 "operator!=",
                 "operator<",
                 "operator<=",
                 "operator>",
                 "operator>="}));
        }

        SUBCASE("relational operators use friend syntax") {
            // Relational operators are implemented as friends
            auto lines = extract_matching_lines(
                code,
                R"(friend.*operator[<>=!])");
            CHECK(lines.size() >= 6);
        }
    }

    TEST_CASE("End-to-End: Spaceship operator")
    {
        auto desc = make_description(
            "struct",
            "test",
            "Orderable",
            "strong int; <=>",
            "",
            20);

        auto code = generate(desc);

        SUBCASE("generates spaceship operator") {
            CHECK(contains(code, "operator<=>"));
        }

        SUBCASE("includes <compare> header") {
            CHECK(contains(code, "#include <compare>"));
        }

        SUBCASE("uses auto return type") {
            CHECK(contains(code, "auto operator<=>"));
        }
    }

    TEST_CASE("End-to-End: Arithmetic modes")
    {
        SUBCASE("checked mode") {
            auto desc = make_description(
                "struct",
                "test",
                "SafeInt",
                "strong int; +, checked");

            auto code = generate(desc);

            CHECK(contains(code, "checked_"));
            CHECK(contains(code, "#include <stdexcept>"));
            CHECK(contains(code, "std::overflow_error"));
        }

        SUBCASE("saturating mode") {
            auto desc = make_description(
                "struct",
                "test",
                "SatInt",
                "strong int; +, saturating");

            auto code = generate(desc);

            CHECK(contains(code, "saturating_"));
            CHECK(contains(code, "std::numeric_limits"));
        }

        SUBCASE("wrapping mode") {
            auto desc = make_description(
                "struct",
                "test",
                "WrapInt",
                "strong int; +, wrapping");

            auto code = generate(desc);

            CHECK(contains(code, "wrapping_"));
        }
    }

    TEST_CASE("End-to-End: I/O operators")
    {
        auto desc = make_description(
            "struct",
            "test",
            "Streamable",
            "strong int; <<, >>");

        auto code = generate(desc);

        SUBCASE("generates stream operators") {
            CHECK(contains(code, "operator<<"));
            CHECK(contains(code, "operator>>"));
        }

        SUBCASE("includes <ostream> and <istream>") {
            CHECK(contains(code, "#include <ostream>"));
            CHECK(contains(code, "#include <istream>"));
        }

        SUBCASE("operators are friend functions") {
            CHECK(contains(code, "friend std::ostream&"));
            CHECK(contains(code, "friend std::istream&"));
        }
    }

    TEST_CASE("End-to-End: Logical operators")
    {
        auto desc = make_description(
            "struct",
            "test",
            "LogicalType",
            "strong bool; !, &&, ||");

        auto code = generate(desc);

        SUBCASE("generates logical operators using keywords") {
            CHECK(contains(code, "operator not"));
            CHECK(contains(code, "operator and"));
            CHECK(contains(code, "operator or"));
        }

        SUBCASE("includes warning about short-circuit evaluation") {
            CHECK(contains(code, "short-circuit"));
        }
    }

    TEST_CASE("End-to-End: Access operators")
    {
        auto desc = make_description(
            "struct",
            "test",
            "Pointer",
            "strong std::unique_ptr<int>; ->, *");

        auto code = generate(desc);

        SUBCASE("generates arrow operator") {
            CHECK(contains(code, "operator->"));
            CHECK(contains(code, "arrow_impl"));
        }

        SUBCASE("generates indirection operator") {
            CHECK(contains(code, "operator*"));
            CHECK(contains(code, "indirection_impl"));
        }

        SUBCASE("includes <memory>") {
            CHECK(contains(code, "#include <memory>"));
        }
    }

    TEST_CASE("End-to-End: Functional operators")
    {
        SUBCASE("nullary operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Callable",
                "strong int; ()");

            auto code = generate(desc);

            CHECK(contains(code, "operator()"));
        }

        SUBCASE("callable operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Invocable",
                "strong int; (&)");

            auto code = generate(desc);

            CHECK(contains(code, "operator&"));
            CHECK(contains(code, "std::invoke"));
        }

        SUBCASE("subscript operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Indexable",
                "strong std::vector<int>; []");

            auto code = generate(desc);

            CHECK(contains(code, "operator[]"));
        }

        SUBCASE("address-of operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Addressable",
                "strong int; &of");

            auto code = generate(desc);

            CHECK(contains(code, "operator&"));
            CHECK(contains(code, "std::addressof"));
        }
    }

    TEST_CASE("End-to-End: Cast operators")
    {
        SUBCASE("bool conversion") {
            auto desc = make_description(
                "struct",
                "test",
                "BoolConvertible",
                "strong int; bool");

            auto code = generate(desc);

            CHECK(contains(code, "operator bool"));
            CHECK(contains(code, "explicit"));
        }

        SUBCASE("explicit cast") {
            auto desc = make_description(
                "struct",
                "test",
                "ExplicitCast",
                "strong int; cast<double>");

            auto code = generate(desc);

            CHECK(contains(code, "operator double"));
            CHECK(contains(code, "explicit"));
        }

        SUBCASE("implicit cast") {
            auto desc = make_description(
                "struct",
                "test",
                "ImplicitCast",
                "strong int; icast<double>");

            auto code = generate(desc);

            CHECK(contains(code, "operator double"));
            // Should NOT contain explicit
            auto op_line = extract_matching_lines(code, "operator double");
            CHECK(op_line.size() > 0);
            CHECK_FALSE(contains(op_line[0], "explicit"));
        }
    }

    TEST_CASE("End-to-End: Specializations")
    {
        SUBCASE("hash specialization") {
            auto desc = make_description(
                "struct",
                "test",
                "Hashable",
                "strong int; hash");

            auto code = generate(desc);

            CHECK(contains(code, "namespace std"));
            CHECK(contains(code, "template<>"));
            CHECK(contains(code, "struct hash<test::Hashable>"));
        }

        SUBCASE("formatter specialization") {
            auto desc = make_description(
                "struct",
                "test",
                "Formattable",
                "strong int; format",
                "",
                20);

            auto code = generate(desc);

            CHECK(contains(code, "template<>"));
            CHECK(contains(code, "formatter<test::Formattable>"));
            CHECK(contains(code, "#include <format>"));
        }
    }

    TEST_CASE("End-to-End: Features")
    {
        SUBCASE("named constants") {
            auto desc = make_description(
                "struct",
                "test",
                "WithConstants",
                "strong int; constant(zero, 0), constant(one, 1)");

            auto code = generate(desc);

            CHECK(contains(code, "inline constexpr WithConstants zero"));
            CHECK(contains(code, "inline constexpr WithConstants one"));
        }

        SUBCASE("forwarded member functions") {
            auto desc = make_description(
                "struct",
                "test",
                "StringWrapper",
                "strong std::string; forward(size), forward(empty)");

            auto code = generate(desc);

            CHECK(contains(code, "size()"));
            CHECK(contains(code, "empty()"));
        }

        SUBCASE("iterator support") {
            auto desc = make_description(
                "struct",
                "test",
                "Container",
                "strong std::vector<int>; iterable");

            auto code = generate(desc);

            CHECK(contains(code, "begin()"));
            CHECK(contains(code, "end()"));
            CHECK(contains(code, "iterator"));
            CHECK(contains(code, "const_iterator"));
        }

        SUBCASE("template assignment") {
            auto desc = make_description(
                "struct",
                "test",
                "Assignable",
                "strong std::string; assign");

            auto code = generate(desc);

            CHECK(contains(code, "template <typename T>"));
            CHECK(contains(code, "operator=(T&&"));
            CHECK(contains(code, "std::assignable_from"));
        }
    }

    TEST_CASE("End-to-End: Complex kitchen sink")
    {
        auto desc = make_description(
            "struct",
            "test",
            "KitchenSink",
            "strong int; +, -, *, /, ==, !=, <, <=, >, >=, ++, --, "
            "<<, >>, hash, format, constant(zero, 0)",
            "42",
            20);

        auto code = generate(desc);

        SUBCASE("generates all requested features") {
            CHECK(contains_all(
                code,
                {"operator+",
                 "operator-",
                 "operator*",
                 "operator/",
                 "operator==",
                 "operator!=",
                 "operator<",
                 "operator<=",
                 "operator>",
                 "operator>=",
                 "operator++",
                 "operator--",
                 "operator<<",
                 "operator>>",
                 "hash<test::KitchenSink>",
                 "formatter<test::KitchenSink>",
                 "inline constexpr KitchenSink zero"}));
        }

        SUBCASE("includes all required headers") {
            CHECK(contains_all(
                code,
                {"#include <limits>",
                 "#include <ostream>",
                 "#include <istream>",
                 "#include <functional>",
                 "#include <format>"}));
        }

        SUBCASE("uses default value") {
            CHECK(contains(code, "int value{42}"));
        }
    }

    TEST_CASE("End-to-End: Guard generation")
    {
        SUBCASE("default guard") {
            auto desc = make_description(
                "struct",
                "my::nested::ns",
                "MyType",
                "strong int");

            auto code = generate(desc);

            CHECK(contains(code, "#ifndef MY_NESTED_NS_MYTYPE_HPP"));
            CHECK(contains(code, "#define MY_NESTED_NS_MYTYPE_HPP"));
        }

        SUBCASE("custom guard prefix") {
            auto desc = make_description();
            desc.type_namespace = "test";
            desc.type_name = "Custom";
            desc.description = "strong int";
            desc.guard_prefix = "ATLAS";

            auto code = generate(desc);

            CHECK(contains(code, "#ifndef ATLAS_TEST_CUSTOM_HPP"));
        }

        SUBCASE("custom guard separator") {
            auto desc = make_description();
            desc.type_namespace = "test";
            desc.type_name = "Custom";
            desc.description = "strong int";
            desc.guard_separator = "__";

            auto code = generate(desc);

            CHECK(contains(code, "#ifndef TEST__CUSTOM__HPP"));
        }
    }

    TEST_CASE("End-to-End: C++ standard version handling")
    {
        SUBCASE("C++11 features") {
            auto desc = make_description(
                "struct",
                "test",
                "Cpp11Type",
                "strong int; +",
                "",
                11);

            auto code = generate(desc);

            // Should use C++11 compatible code
            CHECK_FALSE(contains(code, "constexpr auto operator+"));
        }

        SUBCASE("C++20 features") {
            auto desc = make_description(
                "struct",
                "test",
                "Cpp20Type",
                "strong int; <=>",
                "",
                20);

            auto code = generate(desc);

            // Should use C++20 spaceship operator
            CHECK(contains(code, "operator<=>"));
            CHECK(contains(code, "#include <compare>"));
        }
    }

    TEST_CASE("End-to-End: Class vs struct")
    {
        SUBCASE("struct keyword") {
            auto desc =
                make_description("struct", "test", "MyStruct", "strong int");

            auto code = generate(desc);

            CHECK(contains(code, "struct MyStruct"));
        }

        SUBCASE("class keyword") {
            auto desc =
                make_description("class", "test", "MyClass", "strong int");

            auto code = generate(desc);

            CHECK(contains(code, "class MyClass"));
        }
    }

    TEST_CASE("End-to-End: Namespace handling")
    {
        SUBCASE("nested namespace") {
            auto desc = make_description(
                "struct",
                "foo::bar::baz",
                "Type",
                "strong int");

            auto code = generate(desc);

            CHECK(contains(code, "namespace foo::bar::baz"));
        }

        SUBCASE("global namespace") {
            auto desc =
                make_description("struct", "", "GlobalType", "strong int");

            auto code = generate(desc);

            // Should not have namespace declaration
            CHECK_FALSE(contains(code, "namespace ::"));
        }
    }

    TEST_CASE("Pipeline: TemplateRegistry integration")
    {
        auto & registry = TemplateRegistry::instance();

        SUBCASE("registry contains expected templates") {
            // Verify some key templates are registered
            std::vector<std::string> template_ids;

            ClassInfo info;
            // Make a type that would trigger all templates
            info.arithmetic_binary_operators.push_back(Operator("+"));
            info.relational_operators.push_back(Operator("<"));
            info.has_relational_operators = true;

            registry.visit_applicable(info, [&](ITemplate const & tmpl) {
                template_ids.push_back(tmpl.id());
            });

            // Should have at least addition and relational templates
            CHECK(template_ids.size() >= 2);
        }
    }
}

} // anonymous namespace
