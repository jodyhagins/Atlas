// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/features/TemplateAssignmentOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a StrongTypeDescription with template assignment
StrongTypeDescription
create_test_description_with_template_assignment()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "std::string; assign";
    return desc;
}

} // anonymous namespace

TEST_CASE("TemplateAssignmentOperator registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Template assignment operator is registered") {
        CHECK(registry.has_template("features.template_assignment"));

        auto const * tmpl = registry.get_template(
            "features.template_assignment");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "features.template_assignment");
    }
}

TEST_CASE("TemplateAssignmentOperator should_apply logic")
{
    TemplateAssignmentOperator op;

    SUBCASE("Applies when template assignment is enabled") {
        auto desc = create_test_description_with_template_assignment();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when template assignment is not enabled") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +";
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("TemplateAssignmentOperator template content")
{
    TemplateAssignmentOperator op;

    SUBCASE("Template contains assignment operator definition") {
        auto tmpl = op.get_template();
        CHECK(tmpl.find("operator=") != std::string_view::npos);
        CHECK(tmpl.find("template") != std::string_view::npos);
        CHECK(tmpl.find("std::forward") != std::string_view::npos);
    }

    SUBCASE("Template includes C++20 concepts support") {
        auto tmpl = op.get_template();
        CHECK(tmpl.find("__cpp_concepts") != std::string_view::npos);
        CHECK(tmpl.find("std::assignable_from") != std::string_view::npos);
        CHECK(tmpl.find("std::same_as") != std::string_view::npos);
    }

    SUBCASE("Template includes C++11-17 SFINAE fallback") {
        auto tmpl = op.get_template();
        CHECK(tmpl.find("std::enable_if") != std::string_view::npos);
        CHECK(tmpl.find("std::is_assignable") != std::string_view::npos);
        CHECK(tmpl.find("std::is_same") != std::string_view::npos);
    }

    SUBCASE("Template includes noexcept specification") {
        auto tmpl = op.get_template();
        CHECK(tmpl.find("noexcept") != std::string_view::npos);
    }
}

TEST_CASE("TemplateAssignmentOperator variable preparation")
{
    TemplateAssignmentOperator op;

    SUBCASE("Variables include required fields") {
        auto desc = create_test_description_with_template_assignment();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        CHECK(vars.contains("class_name"));
        CHECK(vars.contains("underlying_type"));
    }

    SUBCASE("Class name is correctly extracted") {
        auto desc = create_test_description_with_template_assignment();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        auto class_name = vars.at("class_name").as_string();
        CHECK(class_name.c_str() == std::string("TestType"));
    }

    SUBCASE("Underlying type is correctly extracted") {
        auto desc = create_test_description_with_template_assignment();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        auto underlying_type = vars.at("underlying_type").as_string();
        CHECK(underlying_type.c_str() == std::string("std::string"));
    }
}

TEST_CASE("TemplateAssignmentOperator rendering integration")
{
    TemplateAssignmentOperator op;

    SUBCASE("Can render template assignment operator") {
        auto desc = create_test_description_with_template_assignment();
        auto info = ClassInfo::parse(desc);
        auto result = op.render(info);

        CHECK_FALSE(result.empty());
        CHECK(result.find("operator=") != std::string::npos);
        CHECK(result.find("TestType") != std::string::npos);
        CHECK(result.find("std::string") != std::string::npos);
    }
}
