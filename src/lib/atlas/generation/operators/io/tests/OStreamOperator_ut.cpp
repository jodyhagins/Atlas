// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/io/OStreamOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::v1::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with ostream operator
StrongTypeDescription
create_test_description_with_ostream()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int; out";
    return desc;
}

} // anonymous namespace

TEST_CASE("OStreamOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("OStream operator is registered") {
        CHECK(registry.has_template("operators.io.ostream"));

        auto const * tmpl = registry.get_template("operators.io.ostream");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.io.ostream");
    }
}

TEST_CASE("OStreamOperator should_apply logic")
{
    OStreamOperator op;

    SUBCASE("Applies when ostream operator is present") {
        auto desc = create_test_description_with_ostream();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when ostream operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("OStreamOperator template content")
{
    OStreamOperator op;

    SUBCASE("Template contains operator definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator <<") != std::string::npos);
    }

    SUBCASE("Template has friend declaration") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("friend") != std::string::npos);
    }

    SUBCASE("Template returns std::ostream reference") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("std::ostream &") != std::string::npos);
    }

    SUBCASE("Template takes std::ostream reference parameter") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("std::ostream & strm") != std::string::npos);
    }

    SUBCASE("Template uses class_name variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{class_name}}}") != std::string::npos);
    }

    SUBCASE("Template inserts t.value into stream") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("strm << t.value") != std::string::npos);
    }

    SUBCASE("Template returns stream for chaining") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("return strm") != std::string::npos);
    }

    SUBCASE("Template has noexcept specification") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("noexcept") != std::string::npos);
    }

    SUBCASE("Template uses underlying_type variable in noexcept") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{underlying_type}}}") != std::string::npos);
    }
}

TEST_CASE("OStreamOperator prepare_variables")
{
    OStreamOperator op;

    SUBCASE("Sets class_name variable") {
        auto desc = create_test_description_with_ostream();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("class_name"));
        CHECK(vars.at("class_name").as_string() == "TestType");
    }

    SUBCASE("Sets underlying_type variable") {
        auto desc = create_test_description_with_ostream();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("underlying_type"));
        CHECK(vars.at("underlying_type").as_string() == "int");
    }
}

TEST_CASE("OStreamOperator required_includes")
{
    OStreamOperator op;

    SUBCASE("Requires <ostream> header") {
        auto includes = op.required_includes();
        CHECK(includes.size() == 1);
        CHECK(includes.count("<ostream>") == 1);
    }
}

TEST_CASE("OStreamOperator required_preamble")
{
    OStreamOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
