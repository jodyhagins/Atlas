#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "atlas/generation/operators/arithmetic/IncrementOperators.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"


using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

StrongTypeDescription
create_test_description(std::string const & ops)
{
    StrongTypeDescription desc;
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    // Type defined in description
    desc.increment_operators = ops;
    return desc;
}

} // anonymous namespace

TEST_CASE("IncrementOperatorsTemplate is registered")
{
    auto const & registry = TemplateRegistry::instance();
    CHECK(registry.has_template("operators.arithmetic.increment"));
}

TEST_CASE("IncrementOperatorsTemplate behavior")
{
    IncrementOperatorsTemplate tmpl;

    SUBCASE("Applies when increment operators are present") {
        auto desc = create_test_description("++");
        auto info = ClassInfo::parse(desc);
        CHECK(tmpl.should_apply(info));
    }

    SUBCASE("Does not apply when no increment operators") {
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        // Type defined in description
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(tmpl.should_apply(info));
    }

    SUBCASE("Template has prefix and postfix forms") {
        auto template_str = tmpl.get_template();
        CHECK(std::string(template_str).find("prefix") != std::string::npos);
        CHECK(std::string(template_str).find("postfix") != std::string::npos);
        CHECK(
            std::string(template_str).find("{{{op}}}t.value") !=
            std::string::npos);
        // Postfix form has ", int" parameter
        CHECK(std::string(template_str).find(", int)") != std::string::npos);
    }

    SUBCASE("Renders for increment operator") {
        auto desc = create_test_description("++");
        auto info = ClassInfo::parse(desc);
        auto rendered = tmpl.render(info);
        CHECK(rendered.find("operator ++") != std::string::npos);
        CHECK(rendered.find("prefix") != std::string::npos);
        CHECK(rendered.find("postfix") != std::string::npos);
    }

    SUBCASE("Renders for decrement operator") {
        auto desc = create_test_description("--");
        auto info = ClassInfo::parse(desc);
        auto rendered = tmpl.render(info);
        CHECK(rendered.find("operator --") != std::string::npos);
    }
}
