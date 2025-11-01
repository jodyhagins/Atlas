#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "atlas/generation/operators/arithmetic/UnaryOperators.hpp"

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
    desc.unary_operators = ops;
    return desc;
}

} // anonymous namespace

TEST_CASE("UnaryOperatorsTemplate is registered")
{
    auto const & registry = TemplateRegistry::instance();
    CHECK(registry.has_template("operators.arithmetic.unary"));
}

TEST_CASE("UnaryOperatorsTemplate behavior")
{
    UnaryOperatorsTemplate tmpl;

    SUBCASE("Applies when unary operators are present") {
        auto desc = create_test_description("+");
        auto info = ClassInfo::parse(desc);
        CHECK(tmpl.should_apply(info));
    }

    SUBCASE("Does not apply when no unary operators") {
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        // Type defined in description
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(tmpl.should_apply(info));
    }

    SUBCASE("Template has proper structure") {
        auto template_str = tmpl.get_template();
        CHECK(
            std::string(template_str).find("operator {{{op}}}") !=
            std::string::npos);
        CHECK(
            std::string(template_str).find("result.value = {{{op}}} t.value") !=
            std::string::npos);
        CHECK(std::string(template_str).find("noexcept") != std::string::npos);
    }

    SUBCASE("Renders for unary plus") {
        auto desc = create_test_description("+");
        auto info = ClassInfo::parse(desc);
        auto rendered = tmpl.render(info);
        CHECK(rendered.find("operator +") != std::string::npos);
    }

    SUBCASE("Renders for unary minus") {
        auto desc = create_test_description("-");
        auto info = ClassInfo::parse(desc);
        auto rendered = tmpl.render(info);
        CHECK(rendered.find("operator -") != std::string::npos);
    }

    SUBCASE("Renders for bitwise NOT") {
        auto desc = create_test_description("~");
        auto info = ClassInfo::parse(desc);
        auto rendered = tmpl.render(info);
        CHECK(rendered.find("operator ~") != std::string::npos);
    }
}
