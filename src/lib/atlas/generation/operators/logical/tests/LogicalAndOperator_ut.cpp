// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/logical/LogicalAndOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with logical AND operator
StrongTypeDescription
create_test_description_with_logical_and()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "bool; &&";
    return desc;
}

} // anonymous namespace

TEST_CASE("LogicalAndOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Logical AND operator is registered") {
        CHECK(registry.has_template("operators.logical.and"));

        auto const * tmpl = registry.get_template("operators.logical.and");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.logical.and");
    }
}

TEST_CASE("LogicalAndOperator should_apply logic")
{
    LogicalAndOperator op;

    SUBCASE("Applies when logical AND operator is present") {
        auto desc = create_test_description_with_logical_and();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies with 'and' keyword form") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "bool; and";
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when logical AND operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    SUBCASE("Does not apply when only OR operator present") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "bool; ||";
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("LogicalAndOperator template content")
{
    LogicalAndOperator op;

    SUBCASE("Template contains operator definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator {{{op}}}") != std::string::npos);
    }

    SUBCASE("Template returns bool") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("bool operator") != std::string::npos);
    }

    SUBCASE("Template has friend declaration") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("friend") != std::string::npos);
    }

    SUBCASE("Template has constexpr support") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
    }

    SUBCASE("Template has noexcept specification") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("noexcept") != std::string::npos);
    }

    SUBCASE("Template uses class_name variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{class_name}}}") != std::string::npos);
    }

    SUBCASE("Template uses underlying_type variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{underlying_type}}}") != std::string::npos);
    }

    SUBCASE("Template has short-circuit warning") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("short-circuit") != std::string::npos);
    }

    SUBCASE("Template applies operator to both values") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("lhs.value {{{op}}} rhs.value") != std::string::npos);
    }
}

TEST_CASE("LogicalAndOperator prepare_variables")
{
    LogicalAndOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_logical_and();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
    }

    SUBCASE("Sets class_name variable") {
        auto desc = create_test_description_with_logical_and();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("class_name"));
        CHECK(vars.at("class_name").as_string() == "TestType");
    }

    SUBCASE("Sets underlying_type variable") {
        auto desc = create_test_description_with_logical_and();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("underlying_type"));
        CHECK(vars.at("underlying_type").as_string() == "bool");
    }

    SUBCASE("Sets op variable to 'and'") {
        auto desc = create_test_description_with_logical_and();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("op"));
        CHECK(vars.at("op").as_string() == "and");
    }
}

TEST_CASE("LogicalAndOperator required_includes")
{
    LogicalAndOperator op;

    SUBCASE("No special includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("LogicalAndOperator required_preamble")
{
    LogicalAndOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
