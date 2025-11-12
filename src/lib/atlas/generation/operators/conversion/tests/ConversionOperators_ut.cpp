#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../BoolOperator.hpp"
#include "../ExplicitCastOperator.hpp"
#include "../ImplicitCastOperator.hpp"
#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

TEST_SUITE("BoolOperator")
{
    TEST_CASE("id returns correct identifier")
    {
        BoolOperator op;
        CHECK(op.id() == "operators.conversion.bool");
    }

    TEST_CASE("should_apply returns true when bool operator is set")
    {
        BoolOperator op;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int; bool";

        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    TEST_CASE("should_apply returns false when bool is not set")
    {
        BoolOperator op;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";

        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    TEST_CASE("get_template contains explicit operator bool")
    {
        BoolOperator op;
        auto tmpl = op.get_template();

        CHECK(tmpl.find("explicit operator bool") != std::string_view::npos);
        CHECK(tmpl.find("static_cast<bool>(value)") != std::string_view::npos);
        CHECK(tmpl.find("noexcept") != std::string_view::npos);
    }

    TEST_CASE("prepare_variables returns valid JSON")
    {
        BoolOperator op;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "std::string; bool";

        auto info = ClassInfo::parse(desc);
        auto vars = op.prepare_variables(info);

        // Just verify it returns a valid JSON object with expected keys
        CHECK(vars.contains("const_expr"));
        CHECK(vars.contains("underlying_type"));
    }

    TEST_CASE("render produces code with bool operator")
    {
        BoolOperator op;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; bool";

        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);

        CHECK(rendered.find("explicit operator bool") != std::string::npos);
        CHECK(rendered.find("static_cast<bool>(value)") != std::string::npos);
    }
}

TEST_SUITE("ExplicitCastOperator")
{
    TEST_CASE("id returns correct identifier")
    {
        ExplicitCastOperator op;
        CHECK(op.id() == "operators.conversion.explicit");
    }

    TEST_CASE("should_apply returns true when explicit casts are present")
    {
        ExplicitCastOperator op;

        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.explicit_casts = {"double", "float"};

        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    TEST_CASE("should_apply returns false when no explicit casts")
    {
        ExplicitCastOperator op;

        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.explicit_casts = {};

        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    TEST_CASE("get_template contains explicit operator")
    {
        ExplicitCastOperator op;

        auto tmpl = op.get_template();

        CHECK(
            tmpl.find("explicit operator {{{cast_type}}}()") !=
            std::string_view::npos);
        CHECK(
            tmpl.find("static_cast<{{{cast_type}}}>(value)") !=
            std::string_view::npos);
    }

    TEST_CASE("render produces explicit cast operator code for all casts")
    {
        ExplicitCastOperator op;

        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "double";
        desc.explicit_casts = {"int", "long"};

        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);

        // Should contain both cast operators
        CHECK(rendered.find("explicit operator int()") != std::string::npos);
        CHECK(rendered.find("explicit operator long()") != std::string::npos);
        CHECK(rendered.find("static_cast<int>(value)") != std::string::npos);
        CHECK(rendered.find("static_cast<long>(value)") != std::string::npos);
    }
}

TEST_SUITE("ImplicitCastOperator")
{
    TEST_CASE("id returns correct identifier")
    {
        ImplicitCastOperator op;
        CHECK(op.id() == "operators.conversion.implicit");
    }

    TEST_CASE("should_apply returns true when implicit casts are present")
    {
        ImplicitCastOperator op;

        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.implicit_casts = {"double", "float"};

        auto info = ClassInfo::parse(desc);
        CHECK(op.should_apply(info));
    }

    TEST_CASE("should_apply returns false when no implicit casts")
    {
        ImplicitCastOperator op;

        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.implicit_casts = {};

        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(op.should_apply(info));
    }

    TEST_CASE(
        "get_template contains implicit operator without explicit keyword")
    {
        ImplicitCastOperator op;

        auto tmpl = op.get_template();

        // Should have operator but NOT explicit keyword before it
        CHECK(
            tmpl.find("operator {{{cast_type}}}()") != std::string_view::npos);
        CHECK(tmpl.find("explicit operator") == std::string_view::npos);
        CHECK(
            tmpl.find("static_cast<{{{cast_type}}}>(value)") !=
            std::string_view::npos);
    }

    TEST_CASE("render produces implicit cast operator code for all casts")
    {
        ImplicitCastOperator op;

        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "std::string";
        desc.implicit_casts = {"std::string_view", "const char*"};

        auto info = ClassInfo::parse(desc);
        auto rendered = op.render(info);

        // Should contain both cast operators
        CHECK(
            rendered.find("operator std::string_view()") != std::string::npos);
        CHECK(rendered.find("operator const char*()") != std::string::npos);
        // Should NOT have explicit keyword
        CHECK(rendered.find("explicit operator") == std::string::npos);
    }

    TEST_CASE("implicit and explicit operators use different templates")
    {
        ImplicitCastOperator implicit_op;
        ExplicitCastOperator explicit_op;

        auto implicit_tmpl = implicit_op.get_template();
        auto explicit_tmpl = explicit_op.get_template();

        // Verify they're different
        CHECK(
            implicit_tmpl.find("explicit operator") == std::string_view::npos);
        CHECK(
            explicit_tmpl.find("explicit operator") != std::string_view::npos);
    }
}

TEST_SUITE("Cast Operators Integration")
{
    TEST_CASE("Different operators have unique IDs")
    {
        BoolOperator bool_op;
        ExplicitCastOperator explicit_op;
        ImplicitCastOperator implicit_op;

        // Each should have unique ID
        CHECK(bool_op.id() == "operators.conversion.bool");
        CHECK(explicit_op.id() == "operators.conversion.explicit");
        CHECK(implicit_op.id() == "operators.conversion.implicit");
    }

    TEST_CASE("Multiple cast operators with same description")
    {
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; bool";
        desc.explicit_casts = {"long", "double"};
        desc.implicit_casts = {"float"};

        auto info = ClassInfo::parse(desc);

        BoolOperator bool_op;
        ExplicitCastOperator explicit_op;
        ImplicitCastOperator implicit_op;

        // All should apply
        CHECK(bool_op.should_apply(info));
        CHECK(explicit_op.should_apply(info));
        CHECK(implicit_op.should_apply(info));
    }
}
