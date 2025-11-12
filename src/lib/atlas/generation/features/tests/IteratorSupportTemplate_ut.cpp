// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/features/IteratorSupportTemplate.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a StrongTypeDescription with iterator support
StrongTypeDescription
create_test_description_with_iterator_support()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "std::vector<int>; iterable";
    return desc;
}

} // anonymous namespace

TEST_CASE("IteratorSupportTemplate registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Iterator support template is registered") {
        CHECK(registry.has_template("features.iterator_support"));

        auto const * tmpl = registry.get_template("features.iterator_support");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "features.iterator_support");
    }
}

TEST_CASE("IteratorSupportTemplate should_apply logic")
{
    IteratorSupportTemplate tmpl;

    SUBCASE("Applies when iterator support is enabled") {
        auto desc = create_test_description_with_iterator_support();
        auto info = ClassInfo::parse(desc);
        CHECK(tmpl.should_apply(info));
    }

    SUBCASE("Does not apply when iterator support is not enabled") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +";
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(tmpl.should_apply(info));
    }
}

TEST_CASE("IteratorSupportTemplate template content")
{
    IteratorSupportTemplate tmpl;

    SUBCASE("Template contains iterator type aliases") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("using iterator") != std::string_view::npos);
        CHECK(
            template_str.find("using const_iterator") !=
            std::string_view::npos);
        CHECK(template_str.find("using value_type") != std::string_view::npos);
    }

    SUBCASE("Template contains begin() and end()") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("begin()") != std::string_view::npos);
        CHECK(template_str.find("end()") != std::string_view::npos);
    }

    SUBCASE("Template uses ADL-enabled helpers") {
        auto template_str = tmpl.get_template();
        CHECK(
            template_str.find("atlas::atlas_detail::begin_") !=
            std::string_view::npos);
        CHECK(
            template_str.find("atlas::atlas_detail::end_") !=
            std::string_view::npos);
    }

    SUBCASE("Template includes const overloads") {
        auto template_str = tmpl.get_template();
        // Look for both non-const and const versions
        CHECK(template_str.find("begin()") != std::string_view::npos);
        CHECK(template_str.find("begin() const") != std::string_view::npos);
        CHECK(template_str.find("end()") != std::string_view::npos);
        CHECK(template_str.find("end() const") != std::string_view::npos);
    }

    SUBCASE("Template includes noexcept specifications") {
        auto template_str = tmpl.get_template();
        CHECK(template_str.find("noexcept") != std::string_view::npos);
    }
}

TEST_CASE("IteratorSupportTemplate variable preparation")
{
    IteratorSupportTemplate tmpl;

    SUBCASE("Variables include required fields") {
        auto desc = create_test_description_with_iterator_support();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        CHECK(vars.contains("underlying_type"));
    }

    SUBCASE("Underlying type is correctly extracted") {
        auto desc = create_test_description_with_iterator_support();
        auto info = ClassInfo::parse(desc);
        auto vars = tmpl.prepare_variables(info);

        auto underlying_type = vars.at("underlying_type").as_string();
        CHECK(underlying_type.c_str() == std::string("std::vector<int>"));
    }
}

TEST_CASE("IteratorSupportTemplate rendering integration")
{
    IteratorSupportTemplate tmpl;

    SUBCASE("Can render iterator support members") {
        auto desc = create_test_description_with_iterator_support();
        auto info = ClassInfo::parse(desc);
        auto result = tmpl.render(info);

        CHECK_FALSE(result.empty());
        CHECK(result.find("using iterator") != std::string::npos);
        CHECK(result.find("begin()") != std::string::npos);
        CHECK(result.find("end()") != std::string::npos);
        CHECK(result.find("std::vector<int>") != std::string::npos);
    }
}
