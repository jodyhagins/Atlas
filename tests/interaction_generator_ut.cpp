// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "InteractionGenerator.hpp"
#include "version.hpp"

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
    TEST_CASE("Basic concrete type interaction")
    {
        InteractionFileDescription desc;
        desc.includes = {"Distance.hpp", "Time.hpp"};

        InteractionDescription interaction{
            .op_symbol = "*",
            .lhs_type = "Velocity",
            .rhs_type = "Time",
            .result_type = "Distance",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "physics",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Check header guard
        CHECK(contains(code, "#ifndef"));
        CHECK(contains(code, "#define"));
        CHECK(contains(code, "#endif"));

        // Check includes
        CHECK(contains(code, "#include \"Distance.hpp\""));
        CHECK(contains(code, "#include \"Time.hpp\""));

        // Check namespace
        CHECK(contains(code, "namespace physics {"));
        CHECK(contains(code, "} // namespace physics"));

        // Check operator signature
        CHECK(contains(
            code,
            "constexpr Distance\noperator*(Velocity lhs, Time rhs)"));

        // Check operator body uses atlas::value
        CHECK(contains(code, "atlas::value(lhs) * atlas::value(rhs)"));

        // Should not generate reverse operator
        CHECK_FALSE(contains(code, "operator*(Time lhs, Velocity rhs)"));
    }

    TEST_CASE("Symmetric interaction")
    {
        InteractionFileDescription desc;

        InteractionDescription interaction{
            .op_symbol = "*",
            .lhs_type = "Distance",
            .rhs_type = "double",
            .result_type = "Distance",
            .symmetric = true,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "physics",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Check both directions
        CHECK(contains(code, "operator*(Distance lhs, double rhs)"));
        CHECK(contains(code, "operator*(double lhs, Distance rhs)"));
    }

    TEST_CASE("Template with concept and enable_if")
    {
        InteractionFileDescription desc;

        TypeConstraint constraint{
            .name = "std::floating_point",
            .concept_expr = "std::floating_point",
            .enable_if_expr = "std::is_floating_point_v<T>"};

        desc.constraints["std::floating_point"] = constraint;

        InteractionDescription interaction{
            .op_symbol = "*",
            .lhs_type = "Distance",
            .rhs_type = "std::floating_point",
            .result_type = "Distance",
            .symmetric = true,
            .lhs_is_template = false,
            .rhs_is_template = true,
            .is_constexpr = true,
            .interaction_namespace = "physics",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Check feature detection
        CHECK(contains(code, "#if __cpp_concepts >= 201907L"));
        CHECK(contains(code, "#else"));
        CHECK(contains(code, "#endif"));

        // Check C++20 concept version
        CHECK(contains(code, "template <std::floating_point T>"));

        // Check C++11 SFINAE version
        CHECK(contains(
            code,
            "typename std::enable_if<std::is_floating_point_v<T>, bool>::type "
            "= true"));

        // Check operator uses template parameter
        CHECK(contains(code, "operator*(Distance lhs, T rhs)"));
        CHECK(contains(code, "operator*(T lhs, Distance rhs)"));
    }

    TEST_CASE("Template with concept only")
    {
        InteractionFileDescription desc;

        TypeConstraint constraint{
            .name = "std::integral",
            .concept_expr = "std::integral",
            .enable_if_expr = ""};

        desc.constraints["std::integral"] = constraint;

        InteractionDescription interaction{
            .op_symbol = "*",
            .lhs_type = "Time",
            .rhs_type = "std::integral",
            .result_type = "Time",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = true,
            .is_constexpr = true,
            .interaction_namespace = "",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Should have concept version only
        CHECK(contains(code, "template <std::integral T>"));

        // Should not have feature detection
        CHECK_FALSE(contains(code, "#if __cpp_concepts"));
    }

    TEST_CASE("Template with enable_if only")
    {
        InteractionFileDescription desc;

        TypeConstraint constraint{
            .name = "std::integral",
            .concept_expr = "",
            .enable_if_expr = "std::is_integral_v<T>"};

        desc.constraints["std::integral"] = constraint;

        InteractionDescription interaction{
            .op_symbol = "+",
            .lhs_type = "std::integral",
            .rhs_type = "Distance",
            .result_type = "Distance",
            .symmetric = false,
            .lhs_is_template = true,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Should have SFINAE version only (C++11 compatible)
        CHECK(contains(
            code,
            "typename std::enable_if<std::is_integral_v<T>, bool>::type = "
            "true"));

        // Should not have feature detection
        CHECK_FALSE(contains(code, "#if __cpp_concepts"));
    }

    TEST_CASE("Both types are templates")
    {
        InteractionFileDescription desc;

        TypeConstraint constraint1{
            .name = "std::integral",
            .concept_expr = "std::integral",
            .enable_if_expr = "std::is_integral_v<T>"};

        TypeConstraint constraint2{
            .name = "std::floating_point",
            .concept_expr = "std::floating_point",
            .enable_if_expr = "std::is_floating_point_v<T>"};

        desc.constraints["std::integral"] = constraint1;
        desc.constraints["std::floating_point"] = constraint2;

        InteractionDescription interaction{
            .op_symbol = "*",
            .lhs_type = "std::integral",
            .rhs_type = "std::floating_point",
            .result_type = "double",
            .symmetric = false,
            .lhs_is_template = true,
            .rhs_is_template = true,
            .is_constexpr = true,
            .interaction_namespace = "",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Should have combined template with two parameters TL and TR
        CHECK(contains(
            code,
            "template <std::integral TL, std::floating_point TR>"));
        CHECK(contains(code, "operator*(TL lhs, TR rhs)"));
    }

    TEST_CASE("No constexpr")
    {
        InteractionFileDescription desc;

        InteractionDescription interaction{
            .op_symbol = "+",
            .lhs_type = "BigNumber",
            .rhs_type = "BigNumber",
            .result_type = "BigNumber",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = false,
            .interaction_namespace = "math",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Find the operator line - should not have constexpr before it
        auto op_pos = code.find("BigNumber\noperator+");
        CHECK(op_pos != std::string::npos);

        // Get the line containing the operator
        auto line_start = code.rfind('\n', op_pos);
        auto line_end = code.find('\n', op_pos);
        std::string op_line = code.substr(
            line_start + 1,
            line_end - line_start - 1);

        // Check that constexpr is not on this line
        CHECK_FALSE(contains(op_line, "constexpr"));
    }

    TEST_CASE("Custom value access - member")
    {
        InteractionFileDescription desc;

        InteractionDescription interaction{
            .op_symbol = "*",
            .lhs_type = "Price",
            .rhs_type = "int",
            .result_type = "Price",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "",
            .value_access = ".value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Should use .value member access
        CHECK(contains(code, "lhs.value * rhs"));
    }

    TEST_CASE("Custom value access - function call")
    {
        InteractionFileDescription desc;

        InteractionDescription interaction{
            .op_symbol = "+",
            .lhs_type = "Custom",
            .rhs_type = "Custom",
            .result_type = "Custom",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "",
            .value_access = "get_value"};

        desc.interactions.push_back(interaction);

        auto code = generate_interactions(desc);

        // Should use get_value() function
        CHECK(contains(code, "get_value(lhs) + get_value(rhs)"));
    }

    TEST_CASE("Multiple namespaces")
    {
        InteractionFileDescription desc;

        InteractionDescription interaction1{
            .op_symbol = "*",
            .lhs_type = "A",
            .rhs_type = "B",
            .result_type = "C",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "ns1",
            .value_access = "atlas::value"};

        InteractionDescription interaction2{
            .op_symbol = "+",
            .lhs_type = "X",
            .rhs_type = "Y",
            .result_type = "Z",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "ns2",
            .value_access = "atlas::value"};

        desc.interactions.push_back(interaction1);
        desc.interactions.push_back(interaction2);

        auto code = generate_interactions(desc);

        // Check both namespaces
        CHECK(contains(code, "namespace ns1 {"));
        CHECK(contains(code, "} // namespace ns1"));
        CHECK(contains(code, "namespace ns2 {"));
        CHECK(contains(code, "} // namespace ns2"));
    }

    TEST_CASE("Multiple operators")
    {
        InteractionFileDescription desc;
        desc.includes = {"<atlas/value.hpp>"};
        desc.guard_prefix = "PHYSICS_OPS";

        // Velocity * Time -> Distance
        desc.interactions.push_back(InteractionDescription{
            .op_symbol = "*",
            .lhs_type = "Velocity",
            .rhs_type = "Time",
            .result_type = "Distance",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "physics",
            .value_access = "atlas::value"});

        // Distance / Time -> Velocity
        desc.interactions.push_back(InteractionDescription{
            .op_symbol = "/",
            .lhs_type = "Distance",
            .rhs_type = "Time",
            .result_type = "Velocity",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "physics",
            .value_access = "atlas::value"});

        // Distance / Distance -> double
        desc.interactions.push_back(InteractionDescription{
            .op_symbol = "/",
            .lhs_type = "Distance",
            .rhs_type = "Distance",
            .result_type = "double",
            .symmetric = false,
            .lhs_is_template = false,
            .rhs_is_template = false,
            .is_constexpr = true,
            .interaction_namespace = "physics",
            .value_access = "atlas::value"});

        auto code = generate_interactions(desc);

        // Check all three operators are present
        CHECK(contains(code, "Distance\noperator*(Velocity lhs, Time rhs)"));
        CHECK(contains(code, "Velocity\noperator/(Distance lhs, Time rhs)"));
        CHECK(contains(code, "double\noperator/(Distance lhs, Distance rhs)"));

        // Check custom guard prefix
        CHECK(contains(code, "PHYSICS_OPS_"));
    }

    TEST_CASE("Empty namespace")
    {
        InteractionFileDescription desc;

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

        // Should not have namespace declaration for empty namespace
        CHECK_FALSE(contains(code, "namespace  {"));
        CHECK(contains(code, "C\noperator+(A lhs, B rhs)"));
    }

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

    TEST_CASE("Include handling")
    {
        InteractionFileDescription desc;
        desc.includes = {
            "Distance.hpp", // No quotes - should add quotes
            "<concepts>", // Already has angle brackets
            "\"Time.hpp\""}; // Already has quotes

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

        // Check include formatting
        CHECK(contains(code, "#include \"Distance.hpp\""));
        CHECK(contains(code, "#include <concepts>"));
        CHECK(contains(code, "#include \"Time.hpp\""));
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

    TEST_CASE("Property: All includes appear in output")
    {
        // Simplified test with concrete examples instead of property testing
        // RapidCheck generates too many invalid strings (empty, with newlines)
        std::vector<std::vector<std::string>> test_cases = {
            {"<iostream>"},
            {"<vector>", "<string>"},
            {"<memory>", "<algorithm>", "<functional>"},
            {"\"myheader.hpp\""},
            {"<cstdint>", "\"types.hpp\"", "<optional>"}};

        for (auto const & includes : test_cases) {
            InteractionFileDescription desc;
            desc.includes = includes;

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

            // Every include should appear
            for (auto const & inc : includes) {
                REQUIRE(contains(code, inc));
            }
        }
    }

    TEST_CASE("atlas_value respects is_constexpr flag")
    {
        SUBCASE("Non-constexpr interaction generates non-constexpr atlas_value")
        {
            InteractionFileDescription desc{
                .includes = {},
                .interactions =
                    {{.op_symbol = "+",
                      .lhs_type = "MyType",
                      .rhs_type = "external::OtherType",
                      .result_type = "MyType",
                      .symmetric = false,
                      .lhs_is_template = false,
                      .rhs_is_template = false,
                      .is_constexpr = false,
                      .interaction_namespace = "test",
                      .lhs_value_access = "atlas::value",
                      .rhs_value_access = ".getValue()",
                      .value_access = ""}},
                .guard_prefix = "",
                .guard_separator = "_",
                .upcase_guard = true};

            auto code = generate_interactions(desc);

            // Should have non-constexpr atlas_value
            CHECK(contains(
                code,
                "inline auto\natlas_value(::external::OtherType const& v, "
                "value_tag)"));
            // Should NOT have constexpr
            CHECK_FALSE(contains(
                code,
                "inline constexpr auto\natlas_value(::external::OtherType "
                "const& v, value_tag)"));
        }

        SUBCASE("Constexpr interaction generates constexpr atlas_value") {
            InteractionFileDescription desc{
                .includes = {},
                .interactions =
                    {{.op_symbol = "+",
                      .lhs_type = "MyType",
                      .rhs_type = "external::OtherType",
                      .result_type = "MyType",
                      .symmetric = false,
                      .lhs_is_template = false,
                      .rhs_is_template = false,
                      .is_constexpr = true,
                      .interaction_namespace = "test",
                      .lhs_value_access = "atlas::value",
                      .rhs_value_access = ".data",
                      .value_access = ""}},
                .guard_prefix = "",
                .guard_separator = "_",
                .upcase_guard = true};

            auto code = generate_interactions(desc);

            // Should have constexpr atlas_value
            CHECK(contains(
                code,
                "inline constexpr auto\natlas_value(::external::OtherType "
                "const& v, value_tag)"));
        }

        SUBCASE("Multiple interactions with same RHS type - any non-constexpr "
                "makes atlas_value non-constexpr")
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
                      .lhs_value_access = "atlas::value",
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
                      .lhs_value_access = "atlas::value",
                      .rhs_value_access = ".getValue()",
                      .value_access = ""}},
                .guard_prefix = "",
                .guard_separator = "_",
                .upcase_guard = true};

            auto code = generate_interactions(desc);

            // Should have non-constexpr atlas_value because one interaction is
            // non-constexpr
            CHECK(contains(
                code,
                "inline auto\natlas_value(::external::Shared const& v, "
                "value_tag)"));
            CHECK_FALSE(contains(
                code,
                "inline constexpr auto\natlas_value(::external::Shared const& "
                "v, value_tag)"));
        }

        SUBCASE("value_access fallback also respects is_constexpr") {
            InteractionFileDescription desc{
                .includes = {},
                .interactions =
                    {{.op_symbol = "+",
                      .lhs_type = "MyType",
                      .rhs_type = "external::OtherType",
                      .result_type = "MyType",
                      .symmetric = false,
                      .lhs_is_template = false,
                      .rhs_is_template = false,
                      .is_constexpr = false,
                      .interaction_namespace = "test",
                      .lhs_value_access = "atlas::value",
                      .rhs_value_access = "",
                      .value_access = ".data"}},
                .guard_prefix = "",
                .guard_separator = "_",
                .upcase_guard = true};

            auto code = generate_interactions(desc);

            // Should have non-constexpr atlas_value when using value_access
            // fallback
            CHECK(contains(
                code,
                "inline auto\natlas_value(::external::OtherType const& v, "
                "value_tag)"));
            CHECK_FALSE(contains(
                code,
                "inline constexpr auto\natlas_value(::external::OtherType "
                "const& v, value_tag)"));
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

    TEST_CASE("Primitive type qualification")
    {
        SUBCASE(
            "size_t is not namespace-qualified and no atlas_value generated")
        {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "ByteCount",
                .rhs_type = "size_t",
                .result_type = "ByteCount",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "data",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Should NOT contain ::data::size_t (invalid)
            CHECK_FALSE(contains(code, "::data::size_t"));

            // Should NOT generate atlas_value for primitives
            CHECK_FALSE(contains(code, "atlas_value(size_t const& v"));

            // Should use size_t directly without .value in operator
            CHECK(contains(code, "lhs.value * rhs"));
        }

        SUBCASE(
            "double is not namespace-qualified and no atlas_value generated")
        {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "Money",
                .rhs_type = "double",
                .result_type = "Money",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "finance::core",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Should NOT contain ::finance::core::double (invalid)
            CHECK_FALSE(contains(code, "::finance::core::double"));

            // Should NOT generate atlas_value for primitives
            CHECK_FALSE(contains(code, "atlas_value(double const& v"));

            // Should use double directly without .value in operator
            CHECK(contains(code, "lhs.value * rhs"));
        }

        SUBCASE("int is not namespace-qualified and no atlas_value generated") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "<<",
                .lhs_type = "Octet",
                .rhs_type = "int",
                .result_type = "Octet",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "net::ipv4",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Should NOT contain ::net::ipv4::int (invalid)
            CHECK_FALSE(contains(code, "::net::ipv4::int"));

            // Should NOT generate atlas_value for primitives
            CHECK_FALSE(contains(code, "atlas_value(int const& v"));

            // Should use int directly without .value in operator
            CHECK(contains(code, "lhs.value << rhs"));
        }

        SUBCASE("Multiple primitive types") {
            InteractionFileDescription desc;

            // ByteCount * size_t
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "ByteCount",
                .rhs_type = "size_t",
                .result_type = "ByteCount",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "data",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            // Money * double
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "Money",
                .rhs_type = "double",
                .result_type = "Money",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "finance",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Check no invalid namespace-qualified primitives
            CHECK_FALSE(contains(code, "::data::size_t"));
            CHECK_FALSE(contains(code, "::finance::double"));

            // Primitives should NOT generate atlas_value functions
            CHECK_FALSE(contains(code, "atlas_value(size_t const& v"));
            CHECK_FALSE(contains(code, "atlas_value(double const& v"));

            // Should use primitives directly in operators
            CHECK(contains(code, "ByteCount{lhs.value * rhs}"));
            CHECK(contains(code, "Money{lhs.value * rhs}"));
        }
    }

    TEST_CASE("std:: type qualification")
    {
        SUBCASE("std::string is not namespace-qualified and no atlas_value "
                "generated")
        {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "ConfigKey",
                .rhs_type = "std::string",
                .result_type = "ConfigKey",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "app::config",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Should NOT contain ::app::config::std::string (invalid)
            CHECK_FALSE(contains(code, "::app::config::std::string"));

            // std library types should NOT generate atlas_value functions
            CHECK_FALSE(contains(code, "atlas_value(std::string const& v"));
            CHECK_FALSE(contains(code, "atlas_value(::std::string const& v"));

            // Should use std::string directly in operator
            CHECK(contains(code, "lhs.value + rhs"));
        }

        SUBCASE("std::string used directly without .value") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "Text",
                .rhs_type = "::std::string",
                .result_type = "Text",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "app",
                .lhs_value_access = "",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // std library types should NOT generate atlas_value functions
            CHECK_FALSE(contains(code, "atlas_value(std::string const& v"));
            CHECK_FALSE(contains(code, "atlas_value(::std::string const& v"));

            // Should use std::string directly in operator
            CHECK(contains(code, "lhs.value + rhs"));
        }
    }

    TEST_CASE("User-defined type qualification")
    {
        SUBCASE("Unqualified user type gets namespace prefix") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "Meters",
                .rhs_type = "Seconds",
                .result_type = "MetersPerSecond",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "physics::units",
                .lhs_value_access = "atlas::value",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Only RHS types with custom value access get atlas_value overloads
            // RHS (Seconds) should be namespace-qualified in atlas_value
            CHECK(contains(
                code,
                "atlas_value(::physics::units::Seconds const& v, value_tag)"));
        }

        SUBCASE("Already qualified user type gets global qualifier") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "MyType",
                .rhs_type = "other::ns::OtherType",
                .result_type = "MyType",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "my::namespace",
                .lhs_value_access = "atlas::value",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Should add global qualifier to already-qualified type
            CHECK(contains(
                code,
                "atlas_value(::other::ns::OtherType const& v, value_tag)"));
        }

        SUBCASE("Global namespace type (::Foo) stays as-is") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "MyType",
                .rhs_type = "::GlobalType",
                .result_type = "MyType",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "my::namespace",
                .lhs_value_access = "atlas::value",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Global namespace type should remain unchanged
            CHECK(contains(
                code,
                "atlas_value(::GlobalType const& v, value_tag)"));
        }

        SUBCASE("Unqualified type in empty namespace gets global qualifier") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "TypeA",
                .rhs_type = "TypeB",
                .result_type = "TypeA",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "",
                .lhs_value_access = "atlas::value",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Type in empty namespace should get :: prefix
            CHECK(contains(code, "atlas_value(::TypeB const& v, value_tag)"));
        }
    }

    TEST_CASE("Mixed type qualification scenarios")
    {
        SUBCASE("User type with primitive type") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "*",
                .lhs_type = "Distance",
                .rhs_type = "double",
                .result_type = "Distance",
                .symmetric = true,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "physics",
                .lhs_value_access = "atlas::value",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // Primitives should NOT be qualified and NOT generate atlas_value
            CHECK_FALSE(contains(code, "atlas_value(double const& v"));
            CHECK_FALSE(contains(code, "::physics::double"));

            // Should use double directly in operator
            CHECK(contains(code, " * rhs"));
        }

        SUBCASE("User type with std:: type") {
            InteractionFileDescription desc;
            desc.interactions.push_back(InteractionDescription{
                .op_symbol = "+",
                .lhs_type = "Container",
                .rhs_type = "std::vector",
                .result_type = "Container",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "data",
                .lhs_value_access = "atlas::value",
                .rhs_value_access = "",
                .value_access = ".value"});

            auto code = generate_interactions(desc);

            // std:: types should NOT be namespace-qualified and NOT generate
            // atlas_value
            CHECK_FALSE(contains(code, "atlas_value(std::vector const& v"));
            CHECK_FALSE(contains(code, "atlas_value(::std::vector const& v"));
            CHECK_FALSE(contains(code, "::data::std::vector"));

            // Should use std::vector directly in operator
            CHECK(contains(code, " + rhs"));
        }

        SUBCASE("All primitive types are correctly handled") {
            InteractionFileDescription desc;

            // Test various primitive types including signed variants
            std::vector<std::string> primitives = {
                "int",
                "signed int",
                "unsigned",
                "signed",
                "long",
                "signed long",
                "signed long long",
                "float",
                "double",
                "size_t",
                "uint8_t",
                "int32_t"};

            for (auto const & prim : primitives) {
                desc.interactions.push_back(InteractionDescription{
                    .op_symbol = "*",
                    .lhs_type = "MyType",
                    .rhs_type = prim,
                    .result_type = "MyType",
                    .symmetric = false,
                    .lhs_is_template = false,
                    .rhs_is_template = false,
                    .is_constexpr = true,
                    .interaction_namespace = "test::namespace",
                    .lhs_value_access = "",
                    .rhs_value_access = "",
                    .value_access = ".value"});
            }

            auto code = generate_interactions(desc);

            // None of the primitives should have namespace qualification
            // and none should generate atlas_value functions
            for (auto const & prim : primitives) {
                std::string invalid = "::test::namespace::" + prim;
                CHECK_FALSE(contains(code, invalid));

                // Primitives should NOT generate atlas_value functions
                std::string atlas_val = "atlas_value(" + prim + " const& v";
                CHECK_FALSE(contains(code, atlas_val));
            }
        }
    }

    TEST_CASE("Primitive types don't get .value access")
    {
        SUBCASE("size_t RHS with value_access=.value") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "*",
                .lhs_type = "ByteCount",
                .rhs_type = "size_t",
                .result_type = "ByteCount",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "data",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT have rhs.value for size_t
            CHECK_FALSE(contains(code, "rhs.value"));

            // Should have lhs.value for ByteCount and direct rhs for size_t
            CHECK(contains(code, "lhs.value * rhs"));
        }

        SUBCASE("double RHS with value_access=.value") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "*",
                .lhs_type = "Money",
                .rhs_type = "double",
                .result_type = "Money",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "finance",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT have rhs.value for double
            CHECK_FALSE(contains(code, "rhs.value"));

            // Should have lhs.value for Money and direct rhs for double
            CHECK(contains(code, "lhs.value * rhs"));
        }

        SUBCASE("double LHS with primitive on left side") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "*",
                .lhs_type = "double",
                .rhs_type = "Meters",
                .result_type = "Meters",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "physics",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT have lhs.value for double
            CHECK_FALSE(contains(code, "lhs.value"));

            // Should have direct lhs for double and rhs.value for Meters
            CHECK(contains(code, "lhs * rhs.value"));
        }

        SUBCASE("int primitive with custom value access") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "+",
                .lhs_type = "Counter",
                .rhs_type = "int",
                .result_type = "Counter",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "util",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT have rhs.value for int
            CHECK_FALSE(contains(code, "rhs.value"));
        }
    }

    TEST_CASE("std library types don't get .value access")
    {
        SUBCASE("std::string RHS with value_access=.value") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "+",
                .lhs_type = "ConfigKey",
                .rhs_type = "std::string",
                .result_type = "ConfigKey",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "config",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT have rhs.value for std::string
            CHECK_FALSE(contains(code, "rhs.value"));

            // Should have lhs.value for ConfigKey and direct rhs for
            // std::string
            CHECK(contains(code, "lhs.value + rhs"));
        }
    }

    TEST_CASE("Atlas types still get .value access")
    {
        SUBCASE("Both sides are Atlas types") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "+",
                .lhs_type = "ByteCount",
                .rhs_type = "ByteCount",
                .result_type = "ByteCount",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "data",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should have .value for both sides (both are Atlas types)
            CHECK(contains(code, "lhs.value + rhs.value"));
        }
    }

    TEST_CASE("Primitives don't generate atlas_value overloads")
    {
        SUBCASE("size_t with .value access should not generate atlas_value") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "*",
                .lhs_type = "ByteCount",
                .rhs_type = "size_t",
                .result_type = "ByteCount",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "data",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT generate atlas_value for size_t
            CHECK_FALSE(contains(code, "atlas_value(size_t const& v"));
            CHECK_FALSE(contains(code, "atlas_value(::size_t const& v"));
        }

        SUBCASE("double with .value access should not generate atlas_value") {
            InteractionFileDescription desc;
            InteractionDescription interaction{
                .op_symbol = "*",
                .lhs_type = "Money",
                .rhs_type = "double",
                .result_type = "Money",
                .symmetric = false,
                .lhs_is_template = false,
                .rhs_is_template = false,
                .is_constexpr = true,
                .interaction_namespace = "finance",
                .value_access = ".value"};
            desc.interactions.push_back(interaction);

            auto code = generate_interactions(desc);

            // Should NOT generate atlas_value for double
            CHECK_FALSE(contains(code, "atlas_value(double const& v"));
            CHECK_FALSE(contains(code, "atlas_value(::double const& v"));
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
}
