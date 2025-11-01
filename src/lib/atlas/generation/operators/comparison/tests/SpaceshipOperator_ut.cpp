// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/comparison/SpaceshipOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with spaceship operator
StrongTypeDescription
create_test_description(bool has_spaceship = true)
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";

    if (has_spaceship) {
        desc.description = "int; <=>";
    } else {
        desc.description = "int";
    }

    return desc;
}

} // anonymous namespace

TEST_CASE("SpaceshipOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Spaceship operator is registered") {
        CHECK(registry.has_template("operators.comparison.spaceship"));

        auto const * tmpl = registry.get_template(
            "operators.comparison.spaceship");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.comparison.spaceship");
    }
}

TEST_CASE("SpaceshipOperator should_apply logic")
{
    SpaceshipOperator op;

    SUBCASE("Applies when spaceship operator is enabled") {
        auto desc = create_test_description(true);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when spaceship operator is disabled") {
        auto desc = create_test_description(false);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("SpaceshipOperator template content")
{
    SpaceshipOperator op;

    SUBCASE("Template contains C++20 spaceship operator") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator <=>") != std::string::npos);
        CHECK(tmpl_str.find("= default") != std::string::npos);
    }

    SUBCASE("Template has C++20 feature detection") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("__cpp_impl_three_way_comparison") !=
            std::string::npos);
        CHECK(tmpl_str.find(">= 201907L") != std::string::npos);
    }

    SUBCASE("Template has C++17 fallback for less-than") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator <") != std::string::npos);
        CHECK(tmpl_str.find("lhs.value < rhs.value") != std::string::npos);
    }

    SUBCASE("Template has C++17 fallback for less-than-or-equal") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator <=") != std::string::npos);
        CHECK(tmpl_str.find("lhs.value <= rhs.value") != std::string::npos);
    }

    SUBCASE("Template has C++17 fallback for greater-than") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator >") != std::string::npos);
        CHECK(tmpl_str.find("lhs.value > rhs.value") != std::string::npos);
    }

    SUBCASE("Template has C++17 fallback for greater-than-or-equal") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator >=") != std::string::npos);
        CHECK(tmpl_str.find("lhs.value >= rhs.value") != std::string::npos);
    }

    SUBCASE("Template has noexcept specifications in C++17 fallback") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        // Should have multiple noexcept specifications
        auto pos = tmpl_str.find("noexcept(noexcept(");
        CHECK(pos != std::string::npos);
        // Find second occurrence
        pos = tmpl_str.find("noexcept(noexcept(", pos + 1);
        CHECK(pos != std::string::npos);
    }

    SUBCASE("Template has constexpr support") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
    }

    SUBCASE("Template has friend declarations") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("friend") != std::string::npos);
    }
}

TEST_CASE("SpaceshipOperator prepare_variables")
{
    SpaceshipOperator op;

    SUBCASE("Sets class_name variable") {
        auto desc = create_test_description(true);
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("class_name"));
        CHECK(
            boost::json::value_to<std::string>(vars.at("class_name")) ==
            "TestType");
    }

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description(true);
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // const_expr should be present (may be empty or "constexpr " based on
        // parsing)
    }
}

TEST_CASE("SpaceshipOperator required_includes")
{
    SpaceshipOperator op;

    SUBCASE("Requires <compare> header") {
        auto includes = op.required_includes();
        CHECK(includes.size() == 1);
        CHECK(includes.count("<compare>") == 1);
    }
}

TEST_CASE("SpaceshipOperator required_preamble")
{
    SpaceshipOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
