// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/access/ArrowOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with arrow operator
StrongTypeDescription
create_test_description_with_arrow()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int*; ->";
    return desc;
}

} // anonymous namespace

TEST_CASE("ArrowOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Arrow operator is registered") {
        CHECK(registry.has_template("operators.access.arrow"));

        auto const * tmpl = registry.get_template("operators.access.arrow");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.access.arrow");
    }
}

TEST_CASE("ArrowOperator should_apply logic")
{
    ArrowOperator op;

    SUBCASE("Applies when arrow operator is present") {
        auto desc = create_test_description_with_arrow();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when arrow operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("ArrowOperator template content")
{
    ArrowOperator op;

    SUBCASE("Template contains operator definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator -> ()") != std::string::npos);
    }

    SUBCASE("Template uses arrow_impl helper") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("atlas::atlas_detail::arrow_impl") !=
            std::string::npos);
    }

    SUBCASE("Template has constexpr support") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
    }

    SUBCASE("Template has const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("const_>") != std::string::npos);
    }

    SUBCASE("Template has non-const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("mutable_>") != std::string::npos);
    }

    SUBCASE("Template uses PriorityTag") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("PriorityTag<1>") != std::string::npos);
    }

    SUBCASE("Template uses decltype for return type deduction") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("-> decltype(atlas::atlas_detail::arrow_impl") !=
            std::string::npos);
    }
}

TEST_CASE("ArrowOperator prepare_variables")
{
    ArrowOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_arrow();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // const_expr should be present (may be empty or "constexpr " based on
        // parsing)
    }
}

TEST_CASE("ArrowOperator required_includes")
{
    ArrowOperator op;

    SUBCASE("No special includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("ArrowOperator required_preamble")
{
    ArrowOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
