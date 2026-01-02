// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "atlas/InteractionGenerator.hpp"
#include "atlas/version.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"
#include "rapidcheck.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::testing;
using wjh::atlas::testing::rc::check;

namespace {

bool
contains(std::string_view code, std::string_view pattern)
{
    return code.find(pattern) != std::string_view::npos;
}

} // anonymous namespace

TEST_SUITE("InteractionGenerator")
{
    TEST_CASE("Header guard customization")
    {
        InteractionFileDescription desc;
        desc.guard_prefix = "MY_PROJECT_INTERACTIONS";
        desc.guard_separator = "__";
        desc.upcase_guard = true;

        InteractionDescription interaction{
            .op_symbol = "+",
            .lhs_type = "A",
            .rhs_type = "B",
            .result_type = "C",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Check custom guard prefix and separator
        CHECK(contains(code, "MY_PROJECT_INTERACTIONS__"));

        // Header guard should be on first line (before NOTICE banner)
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

        // NOTICE banner should come after header guard
        auto notice_pos = code.find("NOTICE");
        REQUIRE(notice_pos != std::string::npos);
        CHECK(notice_pos > second_line_end);
    }

    TEST_CASE("Property: Generated code always has matching header guards")
    {
        check(
            "header guards match",
            [](std::string const & ns,
               std::string const & lhs,
               std::string const & rhs) {
                RC_PRE(not ns.empty() && not lhs.empty() && not rhs.empty());
                RC_PRE(ns.find('\n') == std::string::npos);
                RC_PRE(lhs.find('\n') == std::string::npos);
                RC_PRE(rhs.find('\n') == std::string::npos);

                InteractionFileDescription desc;
                InteractionDescription interaction{
                    .op_symbol = "+",
                    .lhs_type = lhs,
                    .rhs_type = rhs,
                    .result_type = lhs,
                    .symmetric = false,
                    .lhs_is_template = false,
                    .rhs_is_template = false,
                    .is_constexpr = true,
                    .interaction_namespace = ns,
                    .value_access = "atlas::value"};
                desc.interactions.push_back(interaction);

                auto code = generate_interactions(desc);

                // Find #ifndef line
                auto ifndef_pos = code.find("#ifndef ");
                RC_ASSERT(ifndef_pos != std::string::npos);

                auto ifndef_eol = code.find('\n', ifndef_pos);
                std::string ifndef_line = code.substr(
                    ifndef_pos + 8,
                    ifndef_eol - ifndef_pos - 8);

                // Trim whitespace
                ifndef_line.erase(0, ifndef_line.find_first_not_of(" \t"));
                ifndef_line.erase(ifndef_line.find_last_not_of(" \t") + 1);

                // Find #define line
                auto define_pos = code.find("#define ", ifndef_pos);
                RC_ASSERT(define_pos != std::string::npos);

                auto define_eol = code.find('\n', define_pos);
                std::string define_line = code.substr(
                    define_pos + 8,
                    define_eol - define_pos - 8);

                // Trim whitespace
                define_line.erase(0, define_line.find_first_not_of(" \t"));
                define_line.erase(define_line.find_last_not_of(" \t") + 1);

                // Find #endif line with comment
                auto endif_pos = code.rfind("#endif");
                RC_ASSERT(endif_pos != std::string::npos);

                auto endif_eol = code.find('\n', endif_pos);
                std::string endif_line = code.substr(
                    endif_pos,
                    endif_eol - endif_pos);

                // Guards should match
                RC_ASSERT(ifndef_line == define_line);
                RC_ASSERT(contains(endif_line, ifndef_line));
            });
    }

    TEST_CASE("Property: Symmetric operators generate both directions")
    {
        check(
            "symmetric generates both orders",
            [](std::string const & lhs, std::string const & rhs) {
                RC_PRE(not lhs.empty() && not rhs.empty());
                RC_PRE(lhs != rhs); // Different types
                RC_PRE(lhs.find(' ') == std::string::npos);
                RC_PRE(rhs.find(' ') == std::string::npos);

                InteractionFileDescription desc;
                InteractionDescription interaction{
                    .op_symbol = "*",
                    .lhs_type = lhs,
                    .rhs_type = rhs,
                    .result_type = lhs,
                    .symmetric = true,
                    .lhs_is_template = false,
                    .rhs_is_template = false,
                    .is_constexpr = true,
                    .interaction_namespace = "",
                    .value_access = "atlas::value"};
                desc.interactions.push_back(interaction);

                auto code = generate_interactions(desc);

                // Both directions should exist
                std::string forward = "operator*(" + lhs + " lhs, " + rhs +
                    " rhs)";
                std::string reverse = "operator*(" + rhs + " lhs, " + lhs +
                    " rhs)";

                RC_ASSERT(contains(code, forward));
                RC_ASSERT(contains(code, reverse));
            });
    }

    TEST_CASE("Property: Asymmetric operators generate only one direction")
    {
        check(
            "asymmetric generates one order only",
            [](std::string const & lhs, std::string const & rhs) {
                RC_PRE(not lhs.empty() && not rhs.empty());
                RC_PRE(lhs != rhs);
                RC_PRE(lhs.find(' ') == std::string::npos);
                RC_PRE(rhs.find(' ') == std::string::npos);

                InteractionFileDescription desc;
                InteractionDescription interaction{
                    .op_symbol = "+",
                    .lhs_type = lhs,
                    .rhs_type = rhs,
                    .result_type = lhs,
                    .symmetric = false,
                    .lhs_is_template = false,
                    .rhs_is_template = false,
                    .is_constexpr = true,
                    .interaction_namespace = "",
                    .value_access = "atlas::value"};
                desc.interactions.push_back(interaction);

                auto code = generate_interactions(desc);

                // Forward direction should exist
                std::string forward = "operator+(" + lhs + " lhs, " + rhs +
                    " rhs)";
                RC_ASSERT(contains(code, forward));

                // Reverse direction should NOT exist
                std::string reverse = "operator+(" + rhs + " lhs, " + lhs +
                    " rhs)";
                RC_ASSERT(not contains(code, reverse));
            });
    }

    TEST_CASE("Property: Constexpr flag controls qualifier presence")
    {
        check("constexpr flag works correctly", [](bool is_constexpr) {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "+",
                .lhs_type = "TypeA",
                .rhs_type = "TypeB",
                .result_type = "TypeC",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = is_constexpr,
                .interaction_namespace = "",
                .value_access = "atlas::value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Find the operator line
            auto op_pos = code.find("TypeC\noperator+(TypeA lhs, TypeB rhs)");
            RC_ASSERT(op_pos != std::string::npos);

            // Get some context before the operator
            auto context_start = op_pos > 50 ? op_pos - 50 : 0;
            std::string context = code.substr(context_start, 100);

            if (is_constexpr) {
                RC_ASSERT(contains(context, "constexpr"));
            } else {
                // If not constexpr, the word shouldn't appear right before
                // operator
                auto last_constexpr = context.rfind("constexpr");
                if (last_constexpr != std::string::npos) {
                    // If constexpr appears, it should not be immediately before
                    // operator
                    RC_ASSERT(op_pos - context_start - last_constexpr > 20);
                }
            }
        });
    }

    TEST_CASE("atlas_value_for respects is_constexpr flag")
    {
        SUBCASE("Multiple interactions with same RHS type - any non-constexpr "
                "makes atlas_value_for non-constexpr")
        {
            InteractionFileDescription desc{
                .includes = {},
                .interactions =
                    {{.op_symbol = "+",
                      .lhs_type = "Type1",
                      .rhs_type = "external::Shared",
                      .result_type = "Type1",
                      .symmetric = false,
                      .lhs_is_template = false,
                      .rhs_is_template = false,
                      .is_constexpr = true,
                      .interaction_namespace = "test",
                      .lhs_value_access = "atlas::to_underlying",
                      .rhs_value_access = ".getValue()",
                      .value_access = ""},
                     {.op_symbol = "-",
                      .lhs_type = "Type2",
                      .rhs_type = "external::Shared",
                      .result_type = "Type2",
                      .symmetric = false,
                      .lhs_is_template = false,
                      .rhs_is_template = false,
                      .is_constexpr = false,
                      .interaction_namespace = "test",
                      .lhs_value_access = "atlas::to_underlying",
                      .rhs_value_access = ".getValue()",
                      .value_access = ""}},
                .guard_prefix = "",
                .guard_separator = "_",
                .upcase_guard = true};

            auto code = generate_interactions(desc);

            // Should have non-constexpr atlas_value_for because one interaction is
            // non-constexpr
            CHECK(contains(
                code,
                "inline auto\natlas_value_for(::external::Shared const& v, "
                "value_tag)"));
            CHECK_FALSE(contains(
                code,
                "inline constexpr auto\natlas_value_for(::external::Shared const& "
                "v, value_tag)"));
        }
    }

    TEST_CASE("Error Conditions")
    {
        SUBCASE("TypeConstraint with neither concept nor enable_if") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "T",
                .rhs_type = "T",
                .result_type = "T",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // Create constraint with no concept_expr or enable_if_expr
            desc.constraints["T"] = TypeConstraint{.name = "T"};

            CHECK_THROWS_WITH(
                generate_interactions(desc),
                "TypeConstraint has neither concept nor enable_if expression");
        }

        SUBCASE("Missing template constraint for lhs_type") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "T",
                .rhs_type = "int",
                .result_type = "T",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // No constraints defined at all
            CHECK_THROWS_WITH(
                generate_interactions(desc),
                "Template type 'T' used but no constraint defined");
        }

        SUBCASE("Missing template constraint for rhs_type") {
            InteractionFileDescription desc;

            TypeConstraint lhs_constraint{.name = "T"};
            lhs_constraint.concept_expr = "std::is_arithmetic_v<T>";
            desc.constraints["T"] = lhs_constraint;

            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "T",
                .rhs_type = "U",
                .result_type = "T",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            CHECK_THROWS_WITH(
                generate_interactions(desc),
                "Template type 'U' used but no constraint defined");
        }

        SUBCASE("Missing template constraint for result_type") {
            InteractionFileDescription desc;

            TypeConstraint lhs_constraint{.name = "T"};
            lhs_constraint.concept_expr = "std::is_arithmetic_v<T>";
            desc.constraints["T"] = lhs_constraint;

            TypeConstraint rhs_constraint{.name = "U"};
            rhs_constraint.concept_expr = "std::is_integral_v<U>";
            desc.constraints["U"] = rhs_constraint;

            // Note: result_type "R" will be checked because it's a template
            // type (matches constraint name pattern but constraint not defined)
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "T",
                .rhs_type = "U",
                .result_type = "R",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // This may or may not throw depending on if R is detected as
            // template Let's make sure we have all constraints properly defined
            CHECK_NOTHROW(generate_interactions(desc));
        }

        SUBCASE("Alternative value access - function call operator") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "Callable",
                .rhs_type = "Callable",
                .result_type = "Callable",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "()"});

            auto code = generate_interactions(desc);
            // Should use function call operator for value access
            CHECK(contains(code, "lhs() + rhs()"));
        }

        SUBCASE("All template types with all constraints defined") {
            InteractionFileDescription desc;

            TypeConstraint t_constraint{.name = "T"};
            t_constraint.concept_expr = "std::is_arithmetic_v<T>";
            desc.constraints["T"] = t_constraint;

            TypeConstraint u_constraint{.name = "U"};
            u_constraint.concept_expr = "std::is_integral_v<U>";
            desc.constraints["U"] = u_constraint;

            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "T",
                .rhs_type = "U",
                .result_type = "T",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // Should not throw - all constraints defined
            CHECK_NOTHROW(generate_interactions(desc));
        }

        SUBCASE("Mixed template and concrete with partial constraints") {
            InteractionFileDescription desc;

            TypeConstraint t_constraint{.name = "T"};
            t_constraint.concept_expr = "std::is_arithmetic_v<T>";
            desc.constraints["T"] = t_constraint;

            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "T",
                .rhs_type = "double",
                .result_type = "T",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // Should not throw - only template types need constraints
            CHECK_NOTHROW(generate_interactions(desc));
        }

        SUBCASE("Result type constraint name doesn't match template parameter")
        {
            InteractionFileDescription desc;

            TypeConstraint int_constraint{
                .name = "IntType",
                .concept_expr = "std::integral",
                .enable_if_expr = ""};
            desc.constraints["IntType"] = int_constraint;

            TypeConstraint float_constraint{
                .name = "FloatType",
                .concept_expr = "std::floating_point",
                .enable_if_expr = ""};
            desc.constraints["FloatType"] = float_constraint;

            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "/",
                .lhs_type = "IntType",
                .rhs_type = "IntType",
                .result_type = "FloatType",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // Should throw - FloatType is a constraint name but doesn't match
            // the template parameters (IntType)
            CHECK_THROWS_WITH(
                generate_interactions(desc),
                "Result type 'FloatType' is a template constraint name but "
                "doesn't match the template parameter(s). For template "
                "interactions, the result type must either be a concrete type "
                "name (e.g., 'double', 'MyType') or match the template "
                "parameter being used. LHS type: 'IntType', RHS type: "
                "'IntType'");
        }

        SUBCASE(
            "Result type constraint name matches template parameter - valid")
        {
            InteractionFileDescription desc;

            TypeConstraint int_constraint{
                .name = "IntType",
                .concept_expr = "std::integral",
                .enable_if_expr = ""};
            desc.constraints["IntType"] = int_constraint;

            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "IntType",
                .rhs_type = "IntType",
                .result_type = "IntType",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // Should not throw - result type matches template parameter
            CHECK_NOTHROW(generate_interactions(desc));
        }

        SUBCASE("Result type is concrete type with template parameters - valid")
        {
            InteractionFileDescription desc;

            TypeConstraint int_constraint{
                .name = "IntType",
                .concept_expr = "std::integral",
                .enable_if_expr = ""};
            desc.constraints["IntType"] = int_constraint;

            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "/",
                .lhs_type = "IntType",
                .rhs_type = "IntType",
                .result_type = "double",
                .symmetric = false,
                .lhs_is_template = true,
                .rhs_is_template = true,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .value_access = "atlas::value"});

            // Should not throw - result type is a concrete type (not a
            // constraint name)
            CHECK_NOTHROW(generate_interactions(desc));
        }
    }

    TEST_CASE("Version Information")
    {
        SUBCASE("generated interaction code includes version") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "TypeA",
                .rhs_type = "TypeB",
                .result_type = "TypeC",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .lhs_value_access = ".value",
                .rhs_value_access = ".value",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Should contain version information in header comment
            CHECK(
                code.find("Atlas Interaction Generator v") !=
                std::string::npos);
            CHECK(code.find(codegen::version_string) != std::string::npos);
        }

        SUBCASE("generated interaction code includes DO NOT EDIT warning") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "TypeA",
                .rhs_type = "TypeB",
                .result_type = "TypeC",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "test",
                .lhs_value_access = ".value",
                .rhs_value_access = ".value",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            CHECK(code.find("DO NOT EDIT") != std::string::npos);
        }
    }

    TEST_CASE("C++ Standard Specification - Edge Cases")
    {
        SUBCASE("static_assert position - immediately after header guard") {
            InteractionFileDescription desc;
            desc.cpp_standard = 20;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "A",
                .rhs_type = "B",
                .result_type = "C",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "",
                .value_access = "atlas::value"});

            auto code = generate_interactions(desc);

            // Find the #define line
            auto define_pos = code.find("#define");
            REQUIRE(define_pos != std::string::npos);
            auto define_eol = code.find('\n', define_pos);

            // Find the static_assert
            auto assert_pos = code.find("static_assert");
            REQUIRE(assert_pos != std::string::npos);

            // Find the NOTICE banner
            auto notice_pos = code.find("NOTICE");
            REQUIRE(notice_pos != std::string::npos);

            // static_assert should be after #define but before NOTICE
            CHECK(assert_pos > define_eol);
            CHECK(assert_pos < notice_pos);
        }

        SUBCASE("Default C++11 - no static_assert generated") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "A",
                .rhs_type = "B",
                .result_type = "C",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "",
                .value_access = "atlas::value"});

            auto code = generate_interactions(desc);

            // Should NOT contain static_assert for C++11 (default)
            CHECK_FALSE(contains(code, "static_assert(__cplusplus >="));
        }
    }
}
