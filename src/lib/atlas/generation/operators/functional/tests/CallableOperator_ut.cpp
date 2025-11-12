// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/functional/CallableOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with callable operator
StrongTypeDescription
create_test_description_with_callable()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int; (&)";
    return desc;
}

} // anonymous namespace

TEST_CASE("CallableOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("Callable operator is registered") {
        CHECK(registry.has_template("operators.functional.callable"));

        auto const * tmpl = registry.get_template(
            "operators.functional.callable");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.functional.callable");
    }
}

TEST_CASE("CallableOperator should_apply logic")
{
    CallableOperator op;

    SUBCASE("Applies when callable operator is present") {
        auto desc = create_test_description_with_callable();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when callable operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("CallableOperator template content")
{
    CallableOperator op;

    SUBCASE("Template contains operator() with InvocableT") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("operator () (InvocableT && inv)") !=
            std::string::npos);
    }

    SUBCASE("Template has const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(
            tmpl_str.find("operator () (InvocableT && inv) const") !=
            std::string::npos);
    }

    SUBCASE("Template has non-const overload") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        // Look for pattern with template and operator without const
        std::size_t count = 0;
        std::size_t pos = 0;
        while ((pos = tmpl_str.find("operator () (InvocableT && inv)", pos)) !=
               std::string::npos)
        {
            count++;
            pos++;
        }
        CHECK(
            count >=
            4); // 2 const + 2 non-const (std::invoke and fallback versions)
    }

    SUBCASE("Template uses std::invoke when available") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("__cpp_lib_invoke") != std::string::npos);
        CHECK(tmpl_str.find("std::invoke") != std::string::npos);
    }

    SUBCASE("Template has fallback for older compilers") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("#else") != std::string::npos);
        CHECK(
            tmpl_str.find("std::forward<InvocableT>(inv)(value)") !=
            std::string::npos);
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
        CHECK(tmpl_str.find("std::forward<InvocableT>") != std::string::npos);
    }

    SUBCASE("Template has documentation comment") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("/**") != std::string::npos);
        CHECK(tmpl_str.find("invocable") != std::string::npos);
    }

    SUBCASE("Template returns auto with trailing return type") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("auto operator") != std::string::npos);
        CHECK(tmpl_str.find("-> decltype(") != std::string::npos);
    }
}

TEST_CASE("CallableOperator prepare_variables")
{
    CallableOperator op;

    SUBCASE("Sets const_expr variable") {
        auto desc = create_test_description_with_callable();
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
        desc.description = "int; (&); no-constexpr";

        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);
        CHECK(vars.contains("const_expr"));
        CHECK(vars.at("const_expr").as_string().empty());
    }
}

TEST_CASE("CallableOperator required_includes")
{
    CallableOperator op;

    SUBCASE("Requires <utility> and <functional> headers") {
        auto includes = op.required_includes();
        CHECK(includes.size() == 2);
        CHECK(includes.count("<utility>") == 1);
        CHECK(includes.count("<functional>") == 1);
    }
}

TEST_CASE("CallableOperator required_preamble")
{
    CallableOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
