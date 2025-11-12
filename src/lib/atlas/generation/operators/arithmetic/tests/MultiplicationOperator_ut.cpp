#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "atlas/generation/operators/arithmetic/MultiplicationOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"


using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

namespace {

StrongTypeDescription
create_test_description(ArithmeticMode mode = ArithmeticMode::Default)
{
    StrongTypeDescription desc;
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    // Type defined in description
    desc.description = "int; *";
    // Arithmetic mode will be parsed from description
    return desc;
}

} // anonymous namespace

TEST_CASE("Multiplication operator templates are registered")
{
    auto const & registry = TemplateRegistry::instance();

    CHECK(registry.has_template("operators.arithmetic.multiplication.default"));
    CHECK(registry.has_template("operators.arithmetic.multiplication.checked"));
    CHECK(registry.has_template(
        "operators.arithmetic.multiplication.saturating"));
    CHECK(
        registry.has_template("operators.arithmetic.multiplication.wrapping"));
}

TEST_CASE("DefaultMultiplicationOperator")
{
    DefaultMultiplicationOperator op;

    SUBCASE("Applies in Default mode") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Renders multiplication operator") {
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);
        CHECK(rendered.find("operator *=") != std::string::npos);
    }
}

TEST_CASE("CheckedMultiplicationOperator")
{
    CheckedMultiplicationOperator op;

    SUBCASE("Applies in Checked mode") {
        auto desc = create_test_description(ArithmeticMode::Checked);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses checked_mul function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("checked_mul") != std::string::npos);
        CHECK(
            std::string(tmpl).find("multiplication overflow") !=
            std::string::npos);
    }
}

TEST_CASE("SaturatingMultiplicationOperator")
{
    SaturatingMultiplicationOperator op;

    SUBCASE("Applies in Saturating mode") {
        auto desc = create_test_description(ArithmeticMode::Saturating);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses saturating_mul function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("saturating_mul") != std::string::npos);
    }
}

TEST_CASE("WrappingMultiplicationOperator")
{
    WrappingMultiplicationOperator op;

    SUBCASE("Applies in Wrapping mode") {
        auto desc = create_test_description(ArithmeticMode::Wrapping);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }
}
