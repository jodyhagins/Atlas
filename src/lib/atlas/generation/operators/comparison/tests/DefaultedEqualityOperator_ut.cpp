// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/comparison/DefaultedEqualityOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with defaulted equality
StrongTypeDescription
create_test_description(bool has_spaceship = true)
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";

    if (has_spaceship) {
        // Spaceship operator typically enables defaulted equality
        desc.description = "int; <=>";
    } else {
        desc.description = "int";
    }

    return desc;
}

} // anonymous namespace

TEST_CASE("DefaultedEqualityOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Defaulted equality operator is registered") {
        CHECK(registry.has_template("operators.comparison.defaulted_equality"));

        auto const * tmpl = registry.get_template(
            "operators.comparison.defaulted_equality");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.comparison.defaulted_equality");
    }
}

TEST_CASE("DefaultedEqualityOperator should_apply logic")
{
    DefaultedEqualityOperator op;

    SUBCASE("Applies when defaulted equality operator is enabled") {
        auto desc = create_test_description(true);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when defaulted equality operator is disabled") {
        auto desc = create_test_description(false);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("DefaultedEqualityOperator template content")
{
    DefaultedEqualityOperator op;

    SUBCASE("Template contains C++20 defaulted equality operator") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator ==") != std::string::npos);
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

    SUBCASE("Template has C++17 fallback for equality") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator ==") != std::string::npos);
        CHECK(tmpl_str.find("lhs.value == rhs.value") != std::string::npos);
    }

    SUBCASE("Template has C++17 fallback for inequality") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator !=") != std::string::npos);
        CHECK(tmpl_str.find("lhs.value != rhs.value") != std::string::npos);
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

    SUBCASE("Template returns bool") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("bool operator ==") != std::string::npos);
        CHECK(tmpl_str.find("bool operator !=") != std::string::npos);
    }

    SUBCASE("Template has friend declarations") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("friend") != std::string::npos);
    }

    SUBCASE("Template takes const references") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("{{{class_name}}} const & lhs") != std::string::npos);
        CHECK(
            tmpl_str.find("{{{class_name}}} const & rhs") != std::string::npos);
    }
}

TEST_CASE("DefaultedEqualityOperator prepare_variables")
{
    DefaultedEqualityOperator op;

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

TEST_CASE("DefaultedEqualityOperator required_includes")
{
    DefaultedEqualityOperator op;

    SUBCASE("No special includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("DefaultedEqualityOperator required_preamble")
{
    DefaultedEqualityOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}

TEST_CASE("DefaultedEqualityOperator usage with SpaceshipOperator")
{
    DefaultedEqualityOperator eq_op;

    SUBCASE("Typical usage: spaceship enables defaulted equality") {
        auto desc = create_test_description(true);
        // Spaceship operator with no other comparison operators
        // should enable defaulted equality (this is handled by parser)
        // We just verify the template is registered
        CHECK(eq_op.id() == "operators.comparison.defaulted_equality");
    }
}
