// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/operators/io/IStreamOperator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "tests/doctest.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

namespace {

// Helper to create a basic StrongTypeDescription with istream operator
StrongTypeDescription
create_test_description_with_istream()
{
    StrongTypeDescription desc;
    desc.kind = "struct";
    desc.type_name = "TestType";
    desc.type_namespace = "test";
    desc.description = "int; in";
    return desc;
}

} // anonymous namespace

TEST_CASE("IStreamOperator template registration")
{
    auto const & registry = TemplateRegistry::instance();

    SUBCASE("IStream operator is registered") {
        CHECK(registry.has_template("operators.io.istream"));

        auto const * tmpl = registry.get_template("operators.io.istream");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "operators.io.istream");
    }
}

TEST_CASE("IStreamOperator should_apply logic")
{
    IStreamOperator op;

    SUBCASE("Applies when istream operator is present") {
        auto desc = create_test_description_with_istream();
        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    SUBCASE("Does not apply when istream operator is absent") {
        StrongTypeDescription desc;
        desc.kind = "struct";
        desc.type_name = "TestType";
        desc.type_namespace = "test";
        desc.description = "int; +"; // Only arithmetic operator
        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }
}

TEST_CASE("IStreamOperator template content")
{
    IStreamOperator op;

    SUBCASE("Template contains operator definition") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("operator >>") != std::string::npos);
    }

    SUBCASE("Template has friend declaration") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("friend") != std::string::npos);
    }

    SUBCASE("Template returns std::istream reference") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("std::istream &") != std::string::npos);
    }

    SUBCASE("Template takes std::istream reference parameter") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("std::istream & strm") != std::string::npos);
    }

    SUBCASE("Template takes non-const reference to strong type") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        // Should be "&" not "const &" since we're modifying
        CHECK(tmpl_str.find("{{{class_name}}} &") != std::string::npos);
    }

    SUBCASE("Template uses class_name variable") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("{{{class_name}}}") != std::string::npos);
    }

    SUBCASE("Template extracts from stream into t.value") {
        auto tmpl = op.get_template();
        std::string tmpl_str(tmpl);
        CHECK(tmpl_str.find("strm >> t.value") != std::string::npos);
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

TEST_CASE("IStreamOperator prepare_variables")
{
    IStreamOperator op;

    SUBCASE("Sets class_name variable") {
        auto desc = create_test_description_with_istream();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("class_name"));
        CHECK(vars.at("class_name").as_string() == "TestType");
    }

    SUBCASE("Sets underlying_type variable") {
        auto desc = create_test_description_with_istream();
        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        CHECK(vars.contains("underlying_type"));
        CHECK(vars.at("underlying_type").as_string() == "int");
    }
}

TEST_CASE("IStreamOperator required_includes")
{
    IStreamOperator op;

    SUBCASE("Requires <istream> header") {
        auto includes = op.required_includes();
        CHECK(includes.size() == 1);
        CHECK(includes.count("<istream>") == 1);
    }
}

TEST_CASE("IStreamOperator required_preamble")
{
    IStreamOperator op;

    SUBCASE("No preamble required") {
        auto preamble = op.required_preamble();
        CHECK(preamble.empty());
    }
}
