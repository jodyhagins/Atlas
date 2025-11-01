#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "atlas/generation/operators/arithmetic/SubtractionOperator.hpp"

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
    desc.description = "int; -";
    // Arithmetic mode will be parsed from description
    return desc;
}

} // anonymous namespace

TEST_CASE("Subtraction operator templates are registered")
{
    auto const & registry = TemplateRegistry::instance();

    CHECK(registry.has_template("operators.arithmetic.subtraction.default"));
    CHECK(registry.has_template("operators.arithmetic.subtraction.checked"));
    CHECK(registry.has_template("operators.arithmetic.subtraction.saturating"));
    CHECK(registry.has_template("operators.arithmetic.subtraction.wrapping"));
}

TEST_CASE("DefaultSubtractionOperator behavior")
{
    DefaultSubtractionOperator op;

    SUBCASE("Applies in Default mode") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply in Checked mode") {
        auto desc = create_test_description(ArithmeticMode::Checked);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    SUBCASE("Renders subtraction operator") {
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);

        CHECK(rendered.find("operator -=") != std::string::npos);
        CHECK(rendered.find("operator -") != std::string::npos);
        CHECK(rendered.find("lhs.value -= rhs.value") != std::string::npos);
    }
}

TEST_CASE("CheckedSubtractionOperator behavior")
{
    CheckedSubtractionOperator op;

    SUBCASE("Applies in Checked mode") {
        auto desc = create_test_description(ArithmeticMode::Checked);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses checked_sub function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("checked_sub") != std::string::npos);
        CHECK(
            std::string(tmpl).find("subtraction overflow") !=
            std::string::npos);
        CHECK(
            std::string(tmpl).find("subtraction underflow") !=
            std::string::npos);
    }
}

TEST_CASE("SaturatingSubtractionOperator behavior")
{
    SaturatingSubtractionOperator op;

    SUBCASE("Applies in Saturating mode") {
        auto desc = create_test_description(ArithmeticMode::Saturating);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses saturating_sub function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("saturating_sub") != std::string::npos);
        CHECK(std::string(tmpl).find("noexcept") != std::string::npos);
    }
}

TEST_CASE("WrappingSubtractionOperator behavior")
{
    WrappingSubtractionOperator op;

    SUBCASE("Applies in Wrapping mode") {
        auto desc = create_test_description(ArithmeticMode::Wrapping);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Uses unsigned arithmetic") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("make_unsigned") != std::string::npos);
        CHECK(std::string(tmpl).find("static_assert") != std::string::npos);
    }
}

TEST_CASE("Subtraction operator mode selection")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Only appropriate mode applies") {
        auto desc = create_test_description(ArithmeticMode::Checked);

        int applicable_count = 0;
        registry.visit_applicable(desc, [&](ITemplate const & tmpl) {
            auto id = tmpl.id();
            if (id.find("subtraction") != std::string::npos) {
                ++applicable_count;
                CHECK(id == "operators.arithmetic.subtraction.checked");
            }
        });

        CHECK(applicable_count == 1);
    }
}
