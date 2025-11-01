// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/functional/SubscriptOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with subscript operator
StrongTypeDescription
create_test_description_with_subscript()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int; []";
    return desc;
}

} // anonymous namespace

TEST_CASE("SubscriptOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Subscript operator is registered") {
        CHECK(registry.has_template("operators.functional.subscript"));

        auto const * tmpl = registry.get_template(
            "operators.functional.subscript");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.functional.subscript");
    }
}

TEST_CASE("SubscriptOperator should_apply logic")
{
    SubscriptOperator op;

    SUBCASE("Applies when subscript operator is present") {
        auto desc = create_test_description_with_subscript();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when subscript operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("SubscriptOperator template content")
{
    SubscriptOperator op;

    SUBCASE("Template contains operator[] definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator []") != std::string::npos);
    }

    SUBCASE("Template has const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator [] (ArgT && arg") != std::string::npos);
        CHECK(tmpl_str.find(") const") != std::string::npos);
    }

    SUBCASE("Template has non-const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        // Look for pattern with operator[] without const
        std::size_t count = 0;
        std::size_t pos = 0;
        while ((pos = tmpl_str.find("operator []", pos)) != std::string::npos) {
            count++;
            pos++;
        }
        CHECK(count >= 4); // 2 for C++23 + 2 for fallback
    }

    SUBCASE("Template supports multidimensional subscript in C++23") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("__cpp_multidimensional_subscript") !=
            std::string::npos);
        CHECK(tmpl_str.find("ArgTs && ... args") != std::string::npos);
    }

    SUBCASE("Template has fallback for earlier standards") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("#else") != std::string::npos);
        CHECK(tmpl_str.find("template <typename ArgT>") != std::string::npos);
    }

    SUBCASE("Template uses const_expr variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{const_expr}}}") != std::string::npos);
    }

    SUBCASE("Template is conditionally noexcept") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("noexcept(noexcept(") != std::string::npos);
    }

    SUBCASE("Template uses perfect forwarding") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("std::forward<ArgT>") != std::string::npos);
    }

    SUBCASE("Template returns decltype(auto) for C++23") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("decltype(auto) operator []") != std::string::npos);
    }

    SUBCASE("Template returns auto with trailing return type for fallback") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("auto operator []") != std::string::npos);
        CHECK(tmpl_str.find("-> decltype(value[") != std::string::npos);
    }

    SUBCASE("Template forwards to value[]") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("return value[") != std::string::npos);
    }

    SUBCASE("Template has documentation comment") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("/**") != std::string::npos);
        CHECK(tmpl_str.find("Subscript operator") != std::string::npos);
    }
}

TEST_CASE("SubscriptOperator prepare_variables")
{
    SubscriptOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_subscript();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("const_expr"));
        // Default should be "constexpr " (with space)
        CHECK(vars.at("const_expr").as_string() == "constexpr ");
    }

    SUBCASE("Handles no-constexpr option") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; []; no-constexpr";

        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);
        CHECK(vars.contains("const_expr"));
        CHECK(vars.at("const_expr").as_string().empty());
    }
}

TEST_CASE("SubscriptOperator required_includes")
{
    SubscriptOperator op;

    SUBCASE("No includes required") {
        auto includes = op.required_includes();
        CHECK(includes.empty());
    }
}

TEST_CASE("SubscriptOperator required_preamble")
{
    SubscriptOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
