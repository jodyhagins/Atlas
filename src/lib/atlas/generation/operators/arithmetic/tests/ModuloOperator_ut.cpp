#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "atlas/generation/operators/arithmetic/ModuloOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"


using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

StrongTypeDescription
create_test_description(ArithmeticMode mode = ArithmeticMode::Default)
{
    StrongTypeDescription desc;
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    // Type defined in description
    desc.description = "int; %";
    // Arithmetic mode will be parsed from description
    return desc;
}

} // anonymous namespace

TEST_CASE("Modulo operator templates are registered")
{
    auto const & registry = TemplateRegistry::instance();

    CHECK(registry.has_template("operators.arithmetic.modulo.default"));
    CHECK(registry.has_template("operators.arithmetic.modulo.checked"));
    CHECK(registry.has_template("operators.arithmetic.modulo.saturating"));
    // Note: No wrapping modulo - falls back to default
}

TEST_CASE("DefaultModuloOperator")
{
    DefaultModuloOperator op;

    SUBCASE("Applies in Default mode") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Applies in Wrapping mode (fallback)") {
        auto desc = create_test_description(ArithmeticMode::Wrapping);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Renders modulo operator") {
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);
        CHECK(rendered.find("operator %=") != std::string::npos);
    }
}

TEST_CASE("CheckedModuloOperator")
{
    CheckedModuloOperator op;

    SUBCASE("Applies in Checked mode") {
        auto desc = create_test_description(ArithmeticMode::Checked);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses checked_mod function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("checked_mod") != std::string::npos);
        CHECK(std::string(tmpl).find("modulo by zero") != std::string::npos);
    }
}

TEST_CASE("SaturatingModuloOperator")
{
    SaturatingModuloOperator op;

    SUBCASE("Applies in Saturating mode") {
        auto desc = create_test_description(ArithmeticMode::Saturating);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses saturating_rem function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("saturating_rem") != std::string::npos);
        CHECK(
            std::string(tmpl).find("returns 0 for undefined operations") !=
            std::string::npos);
    }
}
