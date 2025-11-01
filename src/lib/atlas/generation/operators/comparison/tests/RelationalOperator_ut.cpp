// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/comparison/RelationalOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with relational operators
StrongTypeDescription
create_test_description_with_operators(std::vector<std::string> const & ops)
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";

    // Build description with operators
    std::string description = "int";
    for (auto const & op : ops) {
        description += "; " + op;
    }
    desc.description = description;

    return desc;
}

} // anonymous namespace

TEST_CASE("RelationalOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Relational operator is registered") {
        CHECK(registry.has_template("operators.comparison.relational"));

        auto const * tmpl = registry.get_template(
            "operators.comparison.relational");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.comparison.relational");
    }
}

TEST_CASE("RelationalOperator should_apply logic")
{
    RelationalOperator op;

    SUBCASE("Applies when less-than operator is present") {
        auto desc = create_test_description_with_operators({"<"});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies when greater-than operator is present") {
        auto desc = create_test_description_with_operators({">"});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies when less-than-or-equal operator is present") {
        auto desc = create_test_description_with_operators({"<="});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies when greater-than-or-equal operator is present") {
        auto desc = create_test_description_with_operators({">="});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies when equality operator is present") {
        auto desc = create_test_description_with_operators({"=="});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies when inequality operator is present") {
        auto desc = create_test_description_with_operators({"!="});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies when multiple relational operators are present") {
        auto desc = create_test_description_with_operators(
            {"<", ">", "<=", ">="});
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when no relational operators are present") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("RelationalOperator template content")
{
    RelationalOperator op;

    SUBCASE("Template contains operator definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator {{{op}}}") != std::string::npos);
        CHECK(
            tmpl_str.find("lhs.value {{{op}}} rhs.value") != std::string::npos);
    }

    SUBCASE("Template has noexcept specification") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("noexcept(noexcept(") != std::string::npos);
    }

    SUBCASE("Template has constexpr support") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
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

    SUBCASE("Template takes const references") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("{{{class_name}}} const & lhs") != std::string::npos);
        CHECK(
            tmpl_str.find("{{{class_name}}} const & rhs") != std::string::npos);
    }
}

TEST_CASE("RelationalOperator prepare_variables")
{
    RelationalOperator op;

    SUBCASE("Sets class_name variable") {
        auto desc = create_test_description_with_operators({"<"});
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("class_name"));
        CHECK(
            boost::json::value_to<std::string>(vars.at("class_name")) ==
            "TestType");
    }

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_operators({"<"});
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // const_expr should be present (may be empty or "constexpr " based on
        // parsing)
    }
}

TEST_CASE("RelationalOperator required_includes")
{
    RelationalOperator op;

    SUBCASE("No special includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("RelationalOperator required_preamble")
{
    RelationalOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
