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

    TEST_CASE("get_template is a pre-C++20 hash_base one-liner")
    {
        HashSpecialization spec;
        auto tmpl = spec.get_template();

        // Pre-C++20 guard
        CHECK(
            tmpl.find("#if __cplusplus < 202002L") !=
            std::string_view::npos);
        CHECK(tmpl.find("#endif") != std::string_view::npos);

        // Inherits from hash_base
        CHECK(tmpl.find("template <>") != std::string_view::npos);
        CHECK(tmpl.find("struct std::hash<") != std::string_view::npos);
        CHECK(
            tmpl.find("atlas::atlas_detail::hash_base<") !=
            std::string_view::npos);

        // Uses full_qualified_name mustache variable
        CHECK(
            tmpl.find("{{{full_qualified_name}}}") !=
            std::string_view::npos);
    }

    TEST_CASE("prepare_variables includes full_qualified_name")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        auto vars = spec.prepare_variables(info);

        CHECK(vars.contains("full_qualified_name"));
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

    TEST_CASE("render produces pre-C++20 hash_base inheritance")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; hash";

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        CHECK(
            rendered.find("std::hash<TestType>") != std::string::npos);
        CHECK(
            rendered.find("atlas::atlas_detail::hash_base<TestType>") !=
            std::string::npos);
        CHECK(
            rendered.find("#if __cplusplus < 202002L") !=
            std::string::npos);
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
        CHECK(
            rendered.find("hash_base<MyNamespace::TestType>") !=
            std::string::npos);
    }

    TEST_CASE("render with no-constexpr-hash still produces hash_base inheritance")
    {
        HashSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; no-constexpr-hash";

        auto info = ClassInfo::parse(desc);
        auto rendered = spec.render(info);

        // Per-type specialization is the same one-liner regardless of
        // constexpr setting (constexpr is controlled at preamble level)
        CHECK(
            rendered.find("hash_base<TestType>") != std::string::npos);
    }
}

TEST_SUITE("FormatterSpecialization")
{
    TEST_CASE("id returns correct identifier")
    {
        FormatterSpecialization spec;
        CHECK(spec.id() == "specializations.formatter");
    }

    TEST_CASE("should_apply returns false - formatter is preamble-only")
    {
        // Per-type formatter specialization is disabled; auto_format at
        // file level handles it via the preamble.
        FormatterSpecialization spec;
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.description = "int";
        desc.generate_formatter = true;

        auto info = ClassInfo::parse(desc);
        CHECK_FALSE(spec.should_apply(info));
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

    TEST_CASE("get_template uses drill-based formatting")
    {
        FormatterSpecialization spec;
        auto tmpl = spec.get_template();

        CHECK(
            tmpl.find("format_drilled_type_t") !=
            std::string_view::npos);
        CHECK(
            tmpl.find("format_value_drill") !=
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

    TEST_CASE("Hash applies but formatter does not for per-type tokens")
    {
        // Per-type 'hash' enables hash_specialization, but per-type
        // formatter is disabled (uses preamble-only approach).
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.cpp_standard = 20;
        desc.description = "int; hash";
        desc.generate_formatter = true;

        HashSpecialization hash_spec;
        FormatterSpecialization formatter_spec;

        auto info = ClassInfo::parse(desc);
        CHECK(hash_spec.should_apply(info));
        CHECK_FALSE(formatter_spec.should_apply(info));
    }

    TEST_CASE("Hash renders pre-C++20 specialization independently")
    {
        StrongTypeDescription desc;
        desc.type_name = "TestType";
        desc.type_namespace = "MyNamespace";
        desc.cpp_standard = 20;
        desc.description = "std::string; hash";

        HashSpecialization hash_spec;

        auto info = ClassInfo::parse(desc);
        auto hash_rendered = hash_spec.render(info);

        // Hash should contain std::hash but not std::formatter
        CHECK(hash_rendered.find("std::hash<") != std::string::npos);
        CHECK(hash_rendered.find("std::formatter<") == std::string::npos);
        CHECK(hash_rendered.find("hash_base<") != std::string::npos);
    }
}
