#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../FormatterSpecialization.hpp"
#include "../HashSpecialization.hpp"
#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

TEST_SUITE("HashSpecialization")
{
    TEST_CASE("id returns correct identifier")
    {
        HashSpecialization spec;
        CHECK(spec.id() == "specializations.hash");
    }

    TEST_CASE("should_apply returns true when hash is enabled")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        CHECK(spec.should_apply(info));
    }

    TEST_CASE("should_apply returns false when hash is not enabled")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";

        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(spec.should_apply(info));
    }

    TEST_CASE("get_template contains std::hash specialization")
    {
        HashSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(tmpl.find("template <>") != std::string_view::npos);
        CHECK(tmpl.find("struct std::hash<") != std::string_view::npos);
        CHECK(tmpl.find("ATLAS_NODISCARD") != std::string_view::npos);
        CHECK(tmpl.find("std::size_t operator ()") != std::string_view::npos);
        CHECK(tmpl.find("noexcept") != std::string_view::npos);
    }

    TEST_CASE("get_template delegates to underlying type's hash")
    {
        HashSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(
            tmpl.find("std::hash<{{{underlying_type}}}>{}") !=
            std::string_view::npos);
        CHECK(
            tmpl.find("static_cast<{{{underlying_type}}} const &>(t)") !=
            std::string_view::npos);
    }

    TEST_CASE("get_template uses hash_const_expr for constexpr control")
    {
        HashSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(
            tmpl.find("{{{hash_const_expr}}}std::size_t") !=
            std::string_view::npos);
    }

    TEST_CASE("prepare_variables returns valid JSON with required fields")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        auto vars = spec.prepare_variables(info);

        CHECK(vars.contains("full_qualified_name"));
        CHECK(vars.contains("underlying_type"));
        CHECK(vars.contains("hash_const_expr"));
    }

    TEST_CASE("prepare_variables includes namespace in full_qualified_name")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.type_namespace = "MyNamespace";
        desc.cpp_standard = 20;
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        auto vars = spec.prepare_variables(info);

        CHECK(vars.contains("full_qualified_name"));
        auto fqn = vars.at("full_qualified_name").as_string();
        CHECK(fqn.c_str() == std::string("MyNamespace::TestType"));
    }

    TEST_CASE("render produces code with std::hash specialization")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        CHECK(rendered.find("std::hash<TestType>") != std::string::npos);
        CHECK(rendered.find("std::hash<int>") != std::string::npos);
    }

    TEST_CASE("render with namespace uses fully qualified name")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.type_namespace = "MyNamespace";
        desc.cpp_standard = 20;
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        CHECK(
            rendered.find("std::hash<MyNamespace::TestType>") !=
            std::string::npos);
    }

    TEST_CASE("render with no-constexpr-hash omits constexpr")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; no-constexpr-hash";

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        // The hash_const_expr should be empty, so constexpr should not appear
        // before std::size_t We can check that "constexpr std::size_t" doesn't
        // appear
        CHECK(rendered.find("constexpr std::size_t") == std::string::npos);
        CHECK(rendered.find("std::size_t operator") != std::string::npos);
    }
}

TEST_SUITE("FormatterSpecialization")
{
    TEST_CASE("id returns correct identifier")
    {
        FormatterSpecialization spec;
        CHECK(spec.id() == "specializations.formatter");
    }

    TEST_CASE("should_apply returns true when formatter is enabled")
    {
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.generate_formatter = true;

        auto info = ClassInfo::parse(desc);
        CHECK(spec.should_apply(info));
    }

    TEST_CASE("should_apply returns false when formatter is not enabled")
    {
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.generate_formatter = false;

        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(spec.should_apply(info));
    }

    TEST_CASE("get_template contains std::formatter specialization")
    {
        FormatterSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(tmpl.find("template <>") != std::string_view::npos);
        CHECK(tmpl.find("struct std::formatter<") != std::string_view::npos);
        CHECK(tmpl.find("auto format(") != std::string_view::npos);
        CHECK(tmpl.find("std::format_context") != std::string_view::npos);
    }

    TEST_CASE("get_template inherits from underlying formatter")
    {
        FormatterSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(
            tmpl.find(": std::formatter<{{{underlying_type}}}>") !=
            std::string_view::npos);
    }

    TEST_CASE("get_template is wrapped in feature test macro")
    {
        FormatterSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(
            tmpl.find("#if defined(__cpp_lib_format)") !=
            std::string_view::npos);
        CHECK(
            tmpl.find("__cpp_lib_format >= 202110L") != std::string_view::npos);
        CHECK(tmpl.find("#endif") != std::string_view::npos);
    }

    TEST_CASE("get_template delegates to underlying formatter")
    {
        FormatterSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(
            tmpl.find("std::formatter<{{{underlying_type}}}>::format") !=
            std::string_view::npos);
        CHECK(
            tmpl.find("static_cast<{{{underlying_type}}} const &>(t)") !=
            std::string_view::npos);
    }

    TEST_CASE("prepare_variables returns valid JSON with required fields")
    {
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "std::string";
        desc.generate_formatter = true;

        auto info = ClassInfo::parse(desc);
        auto vars = spec.prepare_variables(info);

        CHECK(vars.contains("full_qualified_name"));
        CHECK(vars.contains("underlying_type"));
    }

    TEST_CASE("prepare_variables includes namespace in full_qualified_name")
    {
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.type_namespace = "MyNamespace";
        desc.cpp_standard = 20;
        desc.description = "std::string";
        desc.generate_formatter = true;

        auto info = ClassInfo::parse(desc);
        auto vars = spec.prepare_variables(info);

        CHECK(vars.contains("full_qualified_name"));
        auto fqn = vars.at("full_qualified_name").as_string();
        CHECK(fqn.c_str() == std::string("MyNamespace::TestType"));
    }

    TEST_CASE("render produces code with std::formatter specialization")
    {
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "std::string";
        desc.generate_formatter = true;

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        CHECK(
            rendered.find("#if defined(__cpp_lib_format)") !=
            std::string::npos);
        CHECK(rendered.find("std::formatter<TestType>") != std::string::npos);
        CHECK(
            rendered.find("std::formatter<std::string>") != std::string::npos);
        CHECK(rendered.find("#endif") != std::string::npos);
    }

    TEST_CASE("render with namespace uses fully qualified name")
    {
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.type_namespace = "MyNamespace";
        desc.cpp_standard = 20;
        desc.description = "int";
        desc.generate_formatter = true;

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        CHECK(
            rendered.find("std::formatter<MyNamespace::TestType>") !=
            std::string::npos);
    }
}

TEST_SUITE("Specializations Integration")
{
    TEST_CASE("Different specializations have unique IDs")
    {
        HashSpecialization hash_spec;
        FormatterSpecialization formatter_spec;

        CHECK(hash_spec.id() == "specializations.hash");
        CHECK(formatter_spec.id() == "specializations.formatter");
    }

    TEST_CASE("Both specializations can be enabled together")
    {
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; hash";
        desc.generate_formatter = true;

        HashSpecialization hash_spec;
        FormatterSpecialization formatter_spec;

        auto info = ClassInfo::parse(desc);
        CHECK(hash_spec.should_apply(info));
        CHECK(formatter_spec.should_apply(info));
    }

    TEST_CASE("Each specialization renders independently")
    {
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.type_namespace = "MyNamespace";
        desc.cpp_standard = 20;
        desc.description = "std::string; hash";
        desc.generate_formatter = true;

        HashSpecialization hash_spec;
        FormatterSpecialization formatter_spec;

        auto info = ClassInfo::parse(desc);
        auto hash_rendered = hash_spec.render(info);
        auto formatter_rendered = formatter_spec.render(info);

        // Hash should contain std::hash but not std::formatter
        CHECK(hash_rendered.find("std::hash<") != std::string::npos);
        CHECK(hash_rendered.find("std::formatter<") == std::string::npos);

        // Formatter should contain std::formatter but not std::hash
        CHECK(formatter_rendered.find("std::formatter<") != std::string::npos);
        CHECK(formatter_rendered.find("std::hash<") == std::string::npos);
    }
}
