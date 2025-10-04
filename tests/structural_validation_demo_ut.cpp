// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// DEMONSTRATION: Shows improved structural validation vs string matching
//
// This file demonstrates the robustness improvement of using structural
// parsing. Compare these tests with the equivalent ones in
// strong_type_generator_ut.cpp

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

TEST_SUITE("Structural Validation Demo")
{
    TEST_CASE("Hash Support - Structural Validation")
    {
        CodeStructureParser parser;

        SUBCASE("hash with int type - validates complete structure") {
            auto desc = make_description(
                "struct",
                "test",
                "HashableInt",
                "strong int; ==, hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Structural validation is semantic, not syntactic
            CHECK(structure.has_hash_specialization);
            CHECK(structure.hash_is_constexpr);
            CHECK(structure.has_include("#include <functional>"));

            // Also validates type information
            CHECK(structure.type_name == "HashableInt");
            CHECK(structure.namespace_name == "test");
            CHECK(structure.kind == "struct");
        }

        SUBCASE("no-constexpr-hash - precise flag validation") {
            auto desc = make_description(
                "struct",
                "test",
                "RuntimeHash",
                "strong int; ==, no-constexpr-hash");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Validates semantic property, not string presence
            CHECK(structure.has_hash_specialization);
            CHECK_FALSE(structure.hash_is_constexpr); // NOT constexpr!
            CHECK(structure.has_include("#include <functional>"));
        }

        SUBCASE("no hash without explicit option - negative test") {
            auto desc =
                make_description("struct", "test", "NoHash", "strong int; ==");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Clean negative assertion
            CHECK_FALSE(structure.has_hash_specialization);
        }

        SUBCASE("no-constexpr affects both type and hash") {
            auto desc = make_description(
                "struct",
                "test",
                "NoConstexprWithHash",
                "strong int; ==, hash, no-constexpr");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Validates multiple related properties
            CHECK(structure.has_hash_specialization);
            CHECK_FALSE(structure.hash_is_constexpr);
            CHECK_FALSE(structure.has_constexpr_constructor);
        }
    }

    TEST_CASE("Constexpr Support - Structural Validation")
    {
        CodeStructureParser parser;

        SUBCASE("default has constexpr on all operations") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, -, ==");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Constructor should be constexpr by default
            CHECK(structure.has_constexpr_constructor);

            // Operators should also be constexpr
            auto plus_op = structure.find_operator("operator +");
            REQUIRE(plus_op.has_value());
            CHECK(plus_op->is_constexpr);

            auto eq_op = structure.find_operator("operator ==");
            REQUIRE(eq_op.has_value());
            CHECK(eq_op->is_constexpr);
        }

        SUBCASE("no-constexpr removes all constexpr") {
            auto desc = make_description(
                "struct",
                "test",
                "Value",
                "strong int; +, -, ==, no-constexpr");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Constructor should NOT be constexpr
            CHECK_FALSE(structure.has_constexpr_constructor);

            // Operators should also NOT be constexpr
            auto plus_op = structure.find_operator("operator +");
            REQUIRE(plus_op.has_value());
            CHECK_FALSE(plus_op->is_constexpr);
        }
    }

    TEST_CASE("Operator Validation - Structural Approach")
    {
        CodeStructureParser parser;

        SUBCASE("arithmetic operators - complete validation") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; +, -, *");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            // Verify specific operators exist
            auto plus_op = structure.find_operator("operator +");
            REQUIRE(plus_op.has_value());
            CHECK(plus_op->is_friend);
            CHECK(plus_op->is_constexpr);

            auto minus_op = structure.find_operator("operator -");
            REQUIRE(minus_op.has_value());

            auto mult_op = structure.find_operator("operator *");
            REQUIRE(mult_op.has_value());

            // Should also have compound assignment
            CHECK(structure.find_operator("operator +=").has_value());
            CHECK(structure.find_operator("operator -=").has_value());
            CHECK(structure.find_operator("operator *=").has_value());

            // Count arithmetic operators
            auto arith_count = structure.count_operators(
                [](auto const & op) { return op.is_arithmetic(); });
            CHECK(arith_count >= 6); // binary + compound
        }

        SUBCASE("comparison operators with friend qualifier") {
            auto desc = make_description(
                "struct",
                "test",
                "Comparable",
                "strong int; ==, !=");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            auto eq_op = structure.find_operator("operator ==");
            REQUIRE(eq_op.has_value());
            CHECK(eq_op->is_friend);

            auto neq_op = structure.find_operator("operator !=");
            REQUIRE(neq_op.has_value());
            CHECK(neq_op->is_friend);
        }
    }

    TEST_CASE("Type Structure - Complete Validation")
    {
        CodeStructureParser parser;

        SUBCASE("struct has correct visibility") {
            auto desc =
                make_description("struct", "test", "MyInt", "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.kind == "struct");
            CHECK(structure.type_name == "MyInt");
            CHECK(structure.namespace_name == "test");
            CHECK(structure.member_type == "int");
            CHECK(structure.member_name == "value");

            // struct should NOT have public: specifier
            CHECK_FALSE(structure.has_public_specifier);
        }

        SUBCASE("class has public specifier") {
            auto desc = make_description(
                "class",
                "test",
                "MyClass",
                "strong std::string");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.kind == "class");
            CHECK(structure.type_name == "MyClass");

            // class MUST have public: specifier
            CHECK(structure.has_public_specifier);
        }

        SUBCASE("default value parsing") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int",
                "42");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            REQUIRE(structure.member_default_value.has_value());
            CHECK(*structure.member_default_value == "42");
        }

        SUBCASE("no default value") {
            auto desc =
                make_description("struct", "test", "Regular", "strong int");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK_FALSE(structure.member_default_value.has_value());
        }
    }

    TEST_CASE("Include Detection - Semantic Validation")
    {
        CodeStructureParser parser;

        SUBCASE("auto-detected includes for std types") {
            auto desc =
                make_description("struct", "test", "Str", "strong std::string");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_include("#include <string>"));
        }

        SUBCASE("stream operator includes") {
            auto desc = make_description(
                "struct",
                "test",
                "Printable",
                "strong int; out, in");
            auto code = generate_strong_type(desc);
            auto structure = parser.parse(code);

            CHECK(structure.has_include("#include <ostream>"));
            CHECK(structure.has_include("#include <istream>"));
        }
    }
}

} // anonymous namespace
