// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/features/ConstantTemplate.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a StrongTypeDescription with constants
StrongTypeDescription
create_test_description_with_constants()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int";
    desc.constants["zero"] = "0";
    desc.constants["one"] = "1";
    desc.constants["max"] = "100";
    return desc;
}

} // anonymous namespace

TEST_CASE("ConstantTemplate registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Constant declarations template is registered") {
        CHECK(registry.has_template("features.constant_declarations"));

        auto const * tmpl = registry.get_template(
            "features.constant_declarations");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "features.constant_declarations");
    }

    SUBCASE("Constant definitions template is registered") {
        CHECK(registry.has_template("features.constant_definitions"));

        auto const * tmpl = registry.get_template(
            "features.constant_definitions");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "features.constant_definitions");
    }
}

TEST_CASE("ConstantDeclarationsTemplate should_apply logic")
{
    ConstantDeclarationsTemplate tmpl;

    SUBCASE("Applies when constants are defined") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        CHECK(tmpl.should_apply(info));
    }

    SUBCASE("Does not apply when no constants are defined") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int";
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(tmpl.should_apply(info));
    }
}

TEST_CASE("ConstantDefinitionsTemplate should_apply logic")
{
    ConstantDefinitionsTemplate tmpl;

    SUBCASE("Applies when constants are defined") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        CHECK(tmpl.should_apply(info));
    }

    SUBCASE("Does not apply when no constants are defined") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int";
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(tmpl.should_apply(info));
    }
}

TEST_CASE("ConstantDeclarationsTemplate template content")
{
    ConstantDeclarationsTemplate tmpl;

    SUBCASE("Template contains constant declaration structure") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("static const") != std::string_view::npos);
        CHECK(template_str.find("{{{class_name}}}") != std::string_view::npos);
        CHECK(template_str.find("{{{name}}}") != std::string_view::npos);
    }
}

TEST_CASE("ConstantDefinitionsTemplate template content")
{
    ConstantDefinitionsTemplate tmpl;

    SUBCASE("Template contains constant definition structure") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("inline") != std::string_view::npos);
        CHECK(
            template_str.find("{{{full_qualified_name}}}") !=
            std::string_view::npos);
        CHECK(template_str.find("{{{name}}}") != std::string_view::npos);
        CHECK(template_str.find("{{{value}}}") != std::string_view::npos);
    }
}

TEST_CASE("ConstantDeclarationsTemplate variable preparation")
{
    ConstantDeclarationsTemplate tmpl;

    SUBCASE("Variables include class_name and constants array") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        CHECK(vars.contains("class_name"));
        CHECK(vars.contains("constants"));

        // Check that constants is an array
        REQUIRE(vars.at("constants").is_array());
        auto const & constants_array = vars.at("constants").as_array();

        // Should have 3 constants
        CHECK(constants_array.size() == 3);

        // Each constant should have name
        for (auto const & constant : constants_array) {
            REQUIRE(constant.is_object());
            auto const & obj = constant.as_object();
            CHECK(obj.contains("name"));
        }
    }
}

TEST_CASE("ConstantDefinitionsTemplate variable preparation")
{
    ConstantDefinitionsTemplate tmpl;

    SUBCASE("Variables include full_qualified_name and constants array") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        CHECK(vars.contains("full_qualified_name"));
        CHECK(vars.contains("constants"));

        // Check that constants is an array
        REQUIRE(vars.at("constants").is_array());
        auto const & constants_array = vars.at("constants").as_array();

        // Should have 3 constants
        CHECK(constants_array.size() == 3);

        // Each constant should have name and value
        for (auto const & constant : constants_array) {
            REQUIRE(constant.is_object());
            auto const & obj = constant.as_object();
            CHECK(obj.contains("name"));
            CHECK(obj.contains("value"));
        }
    }

    SUBCASE("Full qualified name is properly constructed") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        auto fqn = vars.at("full_qualified_name").as_string();
        CHECK(fqn.c_str() == std::string("test::TestType"));
    }
}

TEST_CASE("ConstantDeclarationsTemplate rendering integration")
{
    ConstantDeclarationsTemplate tmpl;

    SUBCASE("Can render constant declarations for a type") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        auto result = tmpl.render(info);

        // The result should contain constant declarations
        CHECK_FALSE(result.empty());
        CHECK(result.find("static const") != std::string::npos);
    }
}

TEST_CASE("ConstantDefinitionsTemplate rendering integration")
{
    ConstantDefinitionsTemplate tmpl;

    SUBCASE("Can render constant definitions for a type") {
        auto desc = create_test_description_with_constants();
        auto info = ClassInfo::parse(desc);
        auto result = tmpl.render(info);

        // The result should contain constant definitions
        CHECK_FALSE(result.empty());
        CHECK(result.find("inline") != std::string::npos);
    }
}
