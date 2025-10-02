// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "InteractionGenerator.hpp"

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
        CHECK(contains(code, "template<std::floating_point T>"));

        // Check C++11 SFINAE version
        CHECK(contains(
            code,
            "typename std::enable_if<std::is_floating_point_v<T>, bool>::type = true"));

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
        CHECK(contains(code, "template<std::integral T>"));

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
            "typename std::enable_if<std::is_integral_v<T>, bool>::type = true"));

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

        // Should have two template parameters TL and TR
        CHECK(contains(code, "template<std::integral TL>"));
        CHECK(contains(code, "template<std::floating_point TR>"));
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
}
