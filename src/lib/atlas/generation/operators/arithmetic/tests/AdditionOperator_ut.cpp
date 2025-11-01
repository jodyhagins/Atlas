// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"
#include "atlas/generation/operators/arithmeticAdditionOperator.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with addition operator
StrongTypeDescription
create_test_description(ArithmeticMode mode = ArithmeticMode::Default)
{
    StrongTypeDescription desc;
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    // Type defined in description
    desc.description = "int; +";
    // Arithmetic mode will be parsed from description
    return desc;
}

} // anonymous namespace

TEST_CASE("DefaultAdditionOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Default addition operator is registered") {
        CHECK(registry.has_template("operators.arithmetic.addition.default"));

        auto const * tmpl = registry.get_template(
            "operators.arithmetic.addition.default");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.arithmetic.addition.default");
    }
}

TEST_CASE("DefaultAdditionOperator should_apply logic")
{
    DefaultAdditionOperator op;

    SUBCASE("Applies when addition operator is present and mode is Default") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when mode is Checked") {
        auto desc = create_test_description(ArithmeticMode::Checked);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    SUBCASE("Does not apply when mode is Saturating") {
        auto desc = create_test_description(ArithmeticMode::Saturating);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    SUBCASE("Does not apply when mode is Wrapping") {
        auto desc = create_test_description(ArithmeticMode::Wrapping);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    SUBCASE("Does not apply when addition operator is absent") {
        auto desc = create_test_description();
        desc.description = "int; -"; // Only subtraction
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("DefaultAdditionOperator template content")
{
    DefaultAdditionOperator op;

    SUBCASE("Template contains operator+= definition") {
        auto tmpl = op.get_template();
        CHECK(
            std::string(tmpl).find("operator {{{op}}}=") != std::string::npos);
        CHECK(
            std::string(tmpl).find("lhs.value {{{op}}}= rhs.value") !=
            std::string::npos);
    }

    SUBCASE("Template contains operator+ definition") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("operator {{{op}}}") != std::string::npos);
        CHECK(std::string(tmpl).find("lhs {{{op}}}= rhs") != std::string::npos);
    }

    SUBCASE("Template has noexcept specifications") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("noexcept") != std::string::npos);
    }

    SUBCASE("Template has constraint checking") {
        auto tmpl = op.get_template();
        CHECK(
            std::string(tmpl).find("{{#has_constraint}}") != std::string::npos);
        CHECK(
            std::string(tmpl).find("atlas_constraint::check") !=
            std::string::npos);
    }
}

TEST_CASE("DefaultAdditionOperator rendering")
{
    DefaultAdditionOperator op;

    SUBCASE("Renders with correct operator symbol") {
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);

        CHECK(rendered.find("operator +=") != std::string::npos);
        CHECK(rendered.find("operator +") != std::string::npos);
        CHECK(rendered.find("lhs.value += rhs.value") != std::string::npos);
    }

    SUBCASE("Renders with correct type name") {
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);

        CHECK(rendered.find("TestType") != std::string::npos);
    }
}

TEST_CASE("CheckedAdditionOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Checked addition operator is registered") {
        CHECK(registry.has_template("operators.arithmetic.addition.checked"));

        auto const * tmpl = registry.get_template(
            "operators.arithmetic.addition.checked");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.arithmetic.addition.checked");
    }
}

TEST_CASE("CheckedAdditionOperator should_apply logic")
{
    CheckedAdditionOperator op;

    SUBCASE("Applies when addition operator is present and mode is Checked") {
        auto desc = create_test_description(ArithmeticMode::Checked);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when mode is Default") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("CheckedAdditionOperator template content")
{
    CheckedAdditionOperator op;

    SUBCASE("Template uses checked_add function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("checked_add") != std::string::npos);
    }

    SUBCASE("Template mentions overflow exceptions") {
        auto tmpl = op.get_template();
        CHECK(
            std::string(tmpl).find("CheckedOverflowError") !=
            std::string::npos);
        CHECK(
            std::string(tmpl).find("CheckedUnderflowError") !=
            std::string::npos);
    }

    SUBCASE("Template has overflow/underflow messages") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("addition overflow") != std::string::npos);
        CHECK(
            std::string(tmpl).find("addition underflow") != std::string::npos);
    }
}

TEST_CASE("SaturatingAdditionOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Saturating addition operator is registered") {
        CHECK(
            registry.has_template("operators.arithmetic.addition.saturating"));
    }
}

TEST_CASE("SaturatingAdditionOperator should_apply logic")
{
    SaturatingAdditionOperator op;

    SUBCASE("Applies when addition operator is present and mode is Saturating")
    {
        auto desc = create_test_description(ArithmeticMode::Saturating);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when mode is Default") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("SaturatingAdditionOperator template content")
{
    SaturatingAdditionOperator op;

    SUBCASE("Template uses saturating_add function") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("saturating_add") != std::string::npos);
    }

    SUBCASE("Template mentions it clamps to limits") {
        auto tmpl = op.get_template();
        CHECK(
            std::string(tmpl).find("clamps to type limits") !=
            std::string::npos);
    }

    SUBCASE("Template is marked noexcept") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("noexcept") != std::string::npos);
    }
}

TEST_CASE("WrappingAdditionOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Wrapping addition operator is registered") {
        CHECK(registry.has_template("operators.arithmetic.addition.wrapping"));
    }
}

TEST_CASE("WrappingAdditionOperator should_apply logic")
{
    WrappingAdditionOperator op;

    SUBCASE("Applies when addition operator is present and mode is Wrapping") {
        auto desc = create_test_description(ArithmeticMode::Wrapping);
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when mode is Default") {
        auto desc = create_test_description(ArithmeticMode::Default);
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("WrappingAdditionOperator template content")
{
    WrappingAdditionOperator op;

    SUBCASE("Template uses unsigned arithmetic") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("make_unsigned") != std::string::npos);
        CHECK(std::string(tmpl).find("unsigned_type") != std::string::npos);
    }

    SUBCASE("Template has static_assert for integral types") {
        auto tmpl = op.get_template();
        CHECK(std::string(tmpl).find("static_assert") != std::string::npos);
        CHECK(std::string(tmpl).find("is_integral") != std::string::npos);
    }

    SUBCASE("Template mentions well-defined overflow") {
        auto tmpl = op.get_template();
        CHECK(
            std::string(tmpl).find("well-defined overflow") !=
            std::string::npos);
    }
}

TEST_CASE("Addition operator mode selection")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Only default addition applies in Default mode") {
        auto desc = create_test_description(ArithmeticMode::Default);

        int applicable_count = 0;
        registry.visit_applicable(desc, [&](ITemplate const & tmpl) {
            auto id = tmpl.id();
            if (id.find("addition") != std::string::npos) {
                ++applicable_count;
                CHECK(id == "operators.arithmetic.addition.default");
            }
        });

        CHECK(applicable_count == 1);
    }

    SUBCASE("Only checked addition applies in Checked mode") {
        auto desc = create_test_description(ArithmeticMode::Checked);

        int applicable_count = 0;
        registry.visit_applicable(desc, [&](ITemplate const & tmpl) {
            auto id = tmpl.id();
            if (id.find("addition") != std::string::npos) {
                ++applicable_count;
                CHECK(id == "operators.arithmetic.addition.checked");
            }
        });

        CHECK(applicable_count == 1);
    }

    SUBCASE("Only saturating addition applies in Saturating mode") {
        auto desc = create_test_description(ArithmeticMode::Saturating);

        int applicable_count = 0;
        registry.visit_applicable(desc, [&](ITemplate const & tmpl) {
            auto id = tmpl.id();
            if (id.find("addition") != std::string::npos) {
                ++applicable_count;
                CHECK(id == "operators.arithmetic.addition.saturating");
            }
        });

        CHECK(applicable_count == 1);
    }

    SUBCASE("Only wrapping addition applies in Wrapping mode") {
        auto desc = create_test_description(ArithmeticMode::Wrapping);

        int applicable_count = 0;
        registry.visit_applicable(desc, [&](ITemplate const & tmpl) {
            auto id = tmpl.id();
            if (id.find("addition") != std::string::npos) {
                ++applicable_count;
                CHECK(id == "operators.arithmetic.addition.wrapping");
            }
        });

        CHECK(applicable_count == 1);
    }
}
