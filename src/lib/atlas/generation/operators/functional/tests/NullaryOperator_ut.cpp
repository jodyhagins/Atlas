// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/functional/NullaryOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with nullary operator
StrongTypeDescription
create_test_description_with_nullary()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int; ()";
    return desc;
}

} // anonymous namespace

TEST_CASE("NullaryOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Nullary operator is registered") {
        CHECK(registry.has_template("operators.functional.nullary"));

        auto const * tmpl = registry.get_template(
            "operators.functional.nullary");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.functional.nullary");
    }
}

TEST_CASE("NullaryOperator should_apply logic")
{
    NullaryOperator op;

    SUBCASE("Applies when nullary operator is present") {
        auto desc = create_test_description_with_nullary();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when nullary operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("NullaryOperator template content")
{
    NullaryOperator op;

    SUBCASE("Template contains operator() definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator () ()") != std::string::npos);
    }

    SUBCASE("Template has const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator () () const") != std::string::npos);
    }

    SUBCASE("Template has non-const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        // Check for non-const version by looking for the pattern without const
        // after ()
        CHECK(
            tmpl_str.find("& operator () ()\n    noexcept") !=
            std::string::npos);
    }

    SUBCASE("Template returns reference to underlying type") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("{{{underlying_type}}} const & operator") !=
            std::string::npos);
        CHECK(
            tmpl_str.find("{{{underlying_type}}} & operator") !=
            std::string::npos);
    }

    SUBCASE("Template uses const_expr variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
    }

    SUBCASE("Template returns value") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("return value;") != std::string::npos);
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
        CHECK(tmpl_str.find("nullary call operator") != std::string::npos);
    }
}

TEST_CASE("NullaryOperator prepare_variables")
{
    NullaryOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_nullary();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // Default should be "constexpr " (with space)
        CHECK(vars.at("const_expr").as_string() == "constexpr ");
    }

    SUBCASE("Sets underlying_type variable") {
        auto desc = create_test_description_with_nullary();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("underlying_type"));
        CHECK(vars.at("underlying_type").as_string() == "int");
    }

    SUBCASE("Handles no-constexpr option") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; (); no-constexpr";

        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);
        CHECK(vars.contains("const_expr"));
        CHECK(vars.at("const_expr").as_string().empty());
    }
}

TEST_CASE("NullaryOperator required_includes")
{
    NullaryOperator op;

    SUBCASE("No includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("NullaryOperator required_preamble")
{
    NullaryOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
