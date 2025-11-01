// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/functional/AddressOfOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with address-of operator
StrongTypeDescription
create_test_description_with_addressof()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int; &of";
    return desc;
}

} // anonymous namespace

TEST_CASE("AddressOfOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Address-of operator is registered") {
        CHECK(registry.has_template("operators.functional.addressof"));

        auto const * tmpl = registry.get_template(
            "operators.functional.addressof");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.functional.addressof");
    }
}

TEST_CASE("AddressOfOperator should_apply logic")
{
    AddressOfOperator op;

    SUBCASE("Applies when address-of operator is present") {
        auto desc = create_test_description_with_addressof();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when address-of operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("AddressOfOperator template content")
{
    AddressOfOperator op;

    SUBCASE("Template contains operator& definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator {{{op}}}") != std::string::npos);
    }

    SUBCASE("Template has const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator {{{op}}} () const") != std::string::npos);
    }

    SUBCASE("Template has non-const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        // Look for pattern with operator without const
        CHECK(
            tmpl_str.find("* operator {{{op}}} ()\n    noexcept") !=
            std::string::npos);
    }

    SUBCASE("Template returns pointer to underlying type") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("{{{underlying_type}}} const *") !=
            std::string::npos);
        CHECK(tmpl_str.find("{{{underlying_type}}} *") != std::string::npos);
    }

    SUBCASE("Template uses const_expr variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
    }

    SUBCASE("Template uses std::addressof") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("std::addressof(value)") != std::string::npos);
    }

    SUBCASE("Template is noexcept") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("noexcept") != std::string::npos);
    }

    SUBCASE("Template has documentation comment") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("/**") != std::string::npos);
        CHECK(
            tmpl_str.find("pointer to the wrapped object") !=
            std::string::npos);
    }
}

TEST_CASE("AddressOfOperator prepare_variables")
{
    AddressOfOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_addressof();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // Default should be "constexpr " (with space)
        CHECK(vars.at("const_expr").as_string() == "constexpr ");
    }

    SUBCASE("Sets underlying_type variable") {
        auto desc = create_test_description_with_addressof();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("underlying_type"));
        CHECK(vars.at("underlying_type").as_string() == "int");
    }

    SUBCASE("Sets op variable to &") {
        auto desc = create_test_description_with_addressof();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("op"));
        CHECK(vars.at("op").as_string() == "&");
    }

    SUBCASE("Handles no-constexpr option") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; &of; no-constexpr";

        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);
        CHECK(vars.contains("const_expr"));
        CHECK(vars.at("const_expr").as_string().empty());
    }
}

TEST_CASE("AddressOfOperator required_includes")
{
    AddressOfOperator op;

    SUBCASE("Requires <memory> header") {
        auto includes = op.required_includes();
        CHECK(includes.size() == 1);
        CHECK(includes.count("<memory>") == 1);
    }
}

TEST_CASE("AddressOfOperator required_preamble")
{
    AddressOfOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
