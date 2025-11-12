// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/features/ForwardedMemfnTemplate.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

namespace {

// Helper to create a StrongTypeDescription with forwarded member functions
StrongTypeDescription
create_test_description_with_forwarded_memfns()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "std::string";
    desc.forwarded_memfns.push_back("size");
    desc.forwarded_memfns.push_back("const,empty");
    return desc;
}

// Helper with aliased forwarding
StrongTypeDescription
create_test_description_with_aliased_forward()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "std::string";
    desc.forwarded_memfns.push_back("size:length");
    return desc;
}

} // anonymous namespace

TEST_CASE("ForwardedMemfnTemplate registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Forwarded memfn template is registered") {
        CHECK(registry.has_template("features.forwarded_memfn"));

        auto const * tmpl = registry.get_template("features.forwarded_memfn");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "features.forwarded_memfn");
    }
}

TEST_CASE("ForwardedMemfnTemplate should_apply logic")
{
    ForwardedMemfnTemplate tmpl;

    SUBCASE("Applies when forwarded member functions are defined") {
        auto desc = create_test_description_with_forwarded_memfns();
        auto info = ClassInfo::parse(desc);
        CHECK(tmpl.should_apply(info));
    }

    SUBCASE("Does not apply when no forwarded member functions are defined") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +";
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(tmpl.should_apply(info));
    }
}

TEST_CASE("ForwardedMemfnTemplate template content")
{
    ForwardedMemfnTemplate tmpl;

    SUBCASE("Template contains member function forwarding structure") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("{{memfn_name}}") != std::string_view::npos);
        CHECK(
            template_str.find("value.{{memfn_name}}") !=
            std::string_view::npos);
    }

    SUBCASE("Template supports C++23 deducing this") {
        auto template_str = tmpl.get_template();
        CHECK(
            template_str.find("__cpp_explicit_this_parameter") !=
            std::string_view::npos);
        CHECK(template_str.find("this Self&&") != std::string_view::npos);
    }

    SUBCASE("Template includes ref-qualified overloads") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("const &") != std::string_view::npos);
        CHECK(template_str.find("const &&") != std::string_view::npos);
        CHECK(template_str.find("&") != std::string_view::npos);
        CHECK(template_str.find("&&") != std::string_view::npos);
    }

    SUBCASE("Template supports aliasing") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("{{#alias_name}}") != std::string_view::npos);
        CHECK(template_str.find("{{alias_name}}") != std::string_view::npos);
    }

    SUBCASE("Template supports return type transformation") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("{{#return_type}}") != std::string_view::npos);
        CHECK(template_str.find("{{return_type}}") != std::string_view::npos);
    }

    SUBCASE("Template supports constraint checking") {
        auto template_str = tmpl.get_template();
        CHECK(
            template_str.find("{{#has_constraint}}") != std::string_view::npos);
        CHECK(template_str.find("constraint_guard") != std::string_view::npos);
    }

    SUBCASE("Template includes const-only support") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("{{#const_only}}") != std::string_view::npos);
        CHECK(template_str.find("{{^const_only}}") != std::string_view::npos);
    }
}

TEST_CASE("ForwardedMemfnTemplate variable preparation")
{
    ForwardedMemfnTemplate tmpl;

    SUBCASE("Variables include required fields") {
        auto desc = create_test_description_with_forwarded_memfns();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        CHECK(vars.contains("class_name"));
        CHECK(vars.contains("has_constraint"));
        CHECK(vars.contains("forwarded_memfns"));
    }

    SUBCASE("Forwarded memfns is an array") {
        auto desc = create_test_description_with_forwarded_memfns();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        REQUIRE(vars.at("forwarded_memfns").is_array());
        auto const & fwd_array = vars.at("forwarded_memfns").as_array();

        // Should have 2 forwarded functions
        CHECK(fwd_array.size() == 2);
    }

    SUBCASE("Each forwarded memfn has required fields") {
        auto desc = create_test_description_with_forwarded_memfns();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        auto const & fwd_array = vars.at("forwarded_memfns").as_array();
        for (auto const & fwd : fwd_array) {
            REQUIRE(fwd.is_object());
            auto const & obj = fwd.as_object();
            CHECK(obj.contains("memfn_name"));
            CHECK(obj.contains("alias_name"));
            CHECK(obj.contains("const_only"));
        }
    }

    SUBCASE("Has constraint is false when no constraint") {
        auto desc = create_test_description_with_forwarded_memfns();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        CHECK(vars.at("has_constraint").as_bool() == false);
    }
}

TEST_CASE("ForwardedMemfnTemplate rendering integration")
{
    ForwardedMemfnTemplate tmpl;

    SUBCASE("Can render forwarded member functions") {
        auto desc = create_test_description_with_forwarded_memfns();
        auto info = ClassInfo::parse(desc);
        auto result = tmpl.render(info);

        CHECK_FALSE(result.empty());
        // The template renders multiple times (once per forwarded function)
        // so we just verify it doesn't throw
    }

    SUBCASE("Can render with aliased forwarding") {
        auto desc = create_test_description_with_aliased_forward();
        auto info = ClassInfo::parse(desc);
        auto result = tmpl.render(info);

        CHECK_FALSE(result.empty());
    }
}
