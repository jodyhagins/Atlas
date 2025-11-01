// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/access/IndirectionOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with indirection operator
StrongTypeDescription
create_test_description_with_indirection()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int*; @";
    return desc;
}

} // anonymous namespace

TEST_CASE("IndirectionOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Indirection operator is registered") {
        CHECK(registry.has_template("operators.access.indirection"));

        auto const * tmpl = registry.get_template(
            "operators.access.indirection");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.access.indirection");
    }
}

TEST_CASE("IndirectionOperator should_apply logic")
{
    IndirectionOperator op;

    SUBCASE("Applies when indirection operator is present") {
        auto desc = create_test_description_with_indirection();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when indirection operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("IndirectionOperator template content")
{
    IndirectionOperator op;

    SUBCASE("Template contains operator definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator * ()") != std::string::npos);
    }

    SUBCASE("Template uses star_impl helper") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("atlas::atlas_detail::star_impl") !=
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

    SUBCASE("Template uses PriorityTag with different values") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("PriorityTag<1>") != std::string::npos);
        CHECK(tmpl_str.find("PriorityTag<10>") != std::string::npos);
    }

    SUBCASE("Template uses decltype for return type deduction") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("-> decltype(atlas::atlas_detail::star_impl") !=
            std::string::npos);
    }
}

TEST_CASE("IndirectionOperator prepare_variables")
{
    IndirectionOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_indirection();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // const_expr should be present (may be empty or "constexpr " based on
        // parsing)
    }
}

TEST_CASE("IndirectionOperator required_includes")
{
    IndirectionOperator op;

    SUBCASE("No special includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("IndirectionOperator required_preamble")
{
    IndirectionOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
