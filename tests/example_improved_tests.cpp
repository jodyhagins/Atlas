// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// EXAMPLE FILE - Demonstrates improved testing approach
// This shows how tests SHOULD be written for robustness

#include "CodeStructureParser.hpp"
#include "StrongTypeGenerator.hpp"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::testing;

namespace {

auto
make_description(
    std::string kind = "struct",
    std::string type_namespace = "test",
    std::string type_name = "TestType",
    std::string description = "strong int",
    std::string default_value = "")
{
    return StrongTypeDescription{
        .kind = std::move(kind),
        .type_namespace = std::move(type_namespace),
        .type_name = std::move(type_name),
        .description = std::move(description),
        .default_value = std::move(default_value)};
}

TEST_SUITE("Improved Code Structure Tests")
{
    TEST_CASE("Basic struct generation - Structural validation")
    {
        auto desc = make_description("struct", "test", "MyInt", "strong int");
        auto code = generate_strong_type(desc);

        CodeStructureParser parser;
        auto structure = parser.parse(code);

        // Validate high-level structure
        CHECK(structure.kind == "struct");
        CHECK(structure.type_name == "MyInt");
        CHECK(structure.namespace_name == "test");
        CHECK(structure.full_qualified_name == "test::MyInt");

        // Validate member
        CHECK(structure.member_type == "int");
        CHECK(structure.member_name == "value");
        CHECK(not structure.member_default_value.has_value());

        // Struct should not have explicit public: specifier
        CHECK(not structure.has_public_specifier);

        // Should have guard
        CHECK(not structure.guard_name.empty());
        CHECK(structure.guard_name.find("TEST_MYINT") != std::string::npos);
    }

    TEST_CASE("Class generation - Visibility validation")
    {
        auto desc =
            make_description("class", "test", "MyClass", "strong std::string");
        auto code = generate_strong_type(desc);

        CodeStructureParser parser;
        auto structure = parser.parse(code);

        CHECK(structure.kind == "class");
        CHECK(structure.type_name == "MyClass");

        // Class MUST have public: specifier
        CHECK(structure.has_public_specifier);

        // Should auto-detect string header
        CHECK(structure.has_include("#include <string>"));
    }

    TEST_CASE("Arithmetic operators - Semantic validation")
    {
        auto desc =
            make_description("struct", "test", "Number", "strong int; +, -, *");
        auto code = generate_strong_type(desc);

        CodeStructureParser parser;
        auto structure = parser.parse(code);

        // Validate we have all expected operators
        auto plus_op = structure.find_operator("operator +");
        REQUIRE(plus_op.has_value());
        CHECK(plus_op->is_friend);

        auto minus_op = structure.find_operator("operator -");
        REQUIRE(minus_op.has_value());

        auto mult_op = structure.find_operator("operator *");
        REQUIRE(mult_op.has_value());

        // Should also have compound assignment operators
        auto plus_eq = structure.find_operator("operator +=");
        CHECK(plus_eq.has_value());

        // Count total arithmetic operators (binary + compound)
        auto arith_count = structure.count_operators(
            [](auto const & op) { return op.is_arithmetic(); });
        CHECK(arith_count >= 6); // +, -, *, +=, -=, *=
    }

    TEST_CASE("Hash support - Complete validation")
    {
        SUBCASE("With hash") {
            auto desc = make_description(
                "struct",
                "test",
                "Hashable",
                "strong int; ==, hash");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            CHECK(structure.has_hash_specialization);
            CHECK(structure.hash_is_constexpr); // Default is constexpr
            CHECK(structure.has_include("#include <functional>"));
        }

        SUBCASE("With no-constexpr-hash") {
            auto desc = make_description(
                "struct",
                "test",
                "RuntimeHash",
                "strong int; ==, no-constexpr-hash");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            CHECK(structure.has_hash_specialization);
            CHECK(not structure.hash_is_constexpr); // Should NOT be constexpr
        }

        SUBCASE("Without hash") {
            auto desc =
                make_description("struct", "test", "NoHash", "strong int; ==");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            CHECK(not structure.has_hash_specialization);
        }
    }

    TEST_CASE("Default values - Semantic validation")
    {
        SUBCASE("With default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int",
                "42");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == "42");
        }

        SUBCASE("Without default value") {
            auto desc =
                make_description("struct", "test", "Regular", "strong int");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            CHECK(not structure.member_default_value.has_value());
        }
    }

    TEST_CASE("Constexpr support - Complete validation")
    {
        SUBCASE("Default has constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, ==");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            CHECK(structure.has_constexpr_constructor);

            // Operators should be constexpr
            auto plus_op = structure.find_operator("operator +");
            REQUIRE(plus_op.has_value());
            CHECK(plus_op->is_constexpr);
        }

        SUBCASE("no-constexpr removes constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, ==, no-constexpr");
            auto code = generate_strong_type(desc);

            CodeStructureParser parser;
            auto structure = parser.parse(code);

            CHECK(not structure.has_constexpr_constructor);

            // Operators should NOT be constexpr
            auto plus_op = structure.find_operator("operator +");
            REQUIRE(plus_op.has_value());
            CHECK(not plus_op->is_constexpr);
        }
    }
}

} // anonymous namespace
