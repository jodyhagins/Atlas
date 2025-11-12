// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file MainTemplate_ut.cpp
 * Comprehensive unit tests for MainTemplate
 *
 * Tests cover:
 * - Template registration with TemplateRegistry
 * - ID verification
 * - Template content retrieval
 * - should_apply() behavior (always returns true)
 * - required_includes() (returns empty set)
 * - required_preamble() (returns empty set)
 * - prepare_variables() with valid JSON output
 * - render() producing valid C++ code
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/core/MainTemplate.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "tests/doctest.hpp"

namespace {

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Create a StrongTypeDescription with common defaults for testing
 */
StrongTypeDescription
make_description(
    std::string type_namespace = "test",
    std::string type_name = "TestType",
    std::string description = "int")
{
    StrongTypeDescription desc;
    desc.type_namespace = std::move(type_namespace);
    desc.type_name = std::move(type_name);
    desc.description = std::move(description);
    desc.kind = "struct";
    desc.cpp_standard = 20;
    return desc;
}

/**
 * Check if a string contains a given substring
 */
bool
contains(std::string_view text, std::string_view substring)
{
    return text.find(substring) != std::string_view::npos;
}

/**
 * Check if string contains all substrings
 */
bool
contains_all(
    std::string_view text,
    std::initializer_list<std::string_view> substrings)
{
    return std::all_of(
        substrings.begin(),
        substrings.end(),
        [&](std::string_view substr) { return contains(text, substr); });
}

// ============================================================================
// Test Suite
// ============================================================================

TEST_SUITE("MainTemplate Tests")
{
    // ========================================================================
    // Template Registration Tests
    // ========================================================================

    TEST_CASE(
        "Registration - MainTemplate registers itself with TemplateRegistry")
    {
        // Note: MainTemplate self-registers via static TemplateRegistrar
        // during static initialization. We verify it's in the registry.
        auto & registry = TemplateRegistry::instance();

        SUBCASE("Template is registered with expected ID") {
            CHECK(registry.has_template("core.main_structure"));
        }

        SUBCASE("Can retrieve template from registry") {
            auto const * tmpl = registry.get_template("core.main_structure");
            REQUIRE(tmpl != nullptr);
            CHECK(tmpl->id() == "core.main_structure");
        }

        SUBCASE("Retrieved template is MainTemplate instance") {
            auto const * tmpl = registry.get_template("core.main_structure");
            REQUIRE(tmpl != nullptr);

            // Verify it has the expected template content
            auto template_str = tmpl->get_template();
            CHECK_FALSE(template_str.empty());

            // Should contain characteristic Mustache sections
            CHECK(contains(template_str, "{{#namespace_open}}"));
            CHECK(contains(template_str, "{{#namespace_close}}"));
        }
    }

    // ========================================================================
    // ID Tests
    // ========================================================================

    TEST_CASE("ID - returns correct hierarchical identifier")
    {
        MainTemplate tmpl;

        SUBCASE("ID is exactly 'core.main_structure'") {
            CHECK(tmpl.id() == "core.main_structure");
        }

        SUBCASE("ID follows hierarchical naming convention") {
            std::string id = tmpl.id();

            // Should have 'core' prefix (fundamental template)
            CHECK(id.find("core") == 0);

            // Should contain separator
            CHECK(id.find('.') != std::string::npos);

            // Should describe purpose
            CHECK(contains(id, "main_structure"));
        }

        SUBCASE("ID is consistent across multiple calls") {
            std::string id1 = tmpl.id();
            std::string id2 = tmpl.id();
            CHECK(id1 == id2);
        }
    }

    // ========================================================================
    // Template Content Tests
    // ========================================================================

    TEST_CASE(
        "Template Content - get_template() returns valid Mustache template")
    {
        MainTemplate tmpl;
        auto template_str = tmpl.get_template();

        SUBCASE("Returns non-empty template string") {
            CHECK_FALSE(template_str.empty());
        }

        SUBCASE("Contains namespace opening section") {
            CHECK(contains(template_str, "{{#namespace_open}}"));
            CHECK(contains(template_str, "{{/namespace_open}}"));
        }

        SUBCASE("Contains namespace closing section") {
            CHECK(contains(template_str, "{{#namespace_close}}"));
            CHECK(contains(template_str, "{{/namespace_close}}"));
        }

        SUBCASE("Contains class structure elements") {
            // Class declaration
            CHECK(contains(template_str, "atlas::strong_type_tag"));

            // Member variable
            CHECK(contains(template_str, "value"));

            // Type alias
            CHECK(contains(template_str, "atlas_value_type"));
        }

        SUBCASE("Contains constructor sections") {
            // Default constructor
            CHECK(contains(template_str, "{{#delete_default_constructor}}"));

            // Variadic forwarding constructor
            CHECK(contains(template_str, "typename... ArgTs"));
            CHECK(contains(template_str, "std::forward"));
        }

        SUBCASE("Contains cast operator sections") {
            // Explicit cast operators
            CHECK(contains(template_str, "explicit operator"));

            // Additional cast operator partials
            CHECK(contains(template_str, "{{#explicit_cast_operators}}"));
            CHECK(contains(template_str, "{{#implicit_cast_operators}}"));
        }

        SUBCASE("Contains operator partial placeholders") {
            // Arithmetic operators
            CHECK(contains(template_str, "{{#arithmetic_binary_operators}}"));

            // Relational operators
            CHECK(contains(template_str, "{{#relational_operators}}"));

            // Logical operators
            CHECK(contains(template_str, "{{#logical_operators}}"));

            // Increment operators
            CHECK(contains(template_str, "{{#increment_operators}}"));
        }

        SUBCASE("Contains constraint support") {
            CHECK(contains(template_str, "{{#has_constraint}}"));
            CHECK(contains(template_str, "{{#is_bounded}}"));
            CHECK(contains(template_str, "atlas_constraint"));
        }

        SUBCASE("Contains specialization sections") {
            // Hash specialization
            CHECK(contains(template_str, "{{#hash_specialization}}"));

            // Formatter specialization
            CHECK(contains(template_str, "{{#formatter_specialization}}"));
        }

        SUBCASE("Contains documentation comment") {
            CHECK(contains(template_str, "@brief"));
            CHECK(contains(template_str, "Strong type wrapper"));
            CHECK(contains(template_str, "Generated by Atlas"));
        }

        SUBCASE("Template is consistent across multiple calls") {
            auto tmpl1 = tmpl.get_template();
            auto tmpl2 = tmpl.get_template();
            CHECK(tmpl1 == tmpl2);
        }

        SUBCASE("Template string has expected length") {
            // The template should be substantial (several hundred characters)
            // but not enormous (less than 10KB)
            CHECK(template_str.length() > 500);
            CHECK(template_str.length() < 10000);
        }
    }

    // ========================================================================
    // should_apply() Tests
    // ========================================================================

    TEST_CASE("should_apply() - always returns true")
    {
        MainTemplate tmpl;

        SUBCASE("Returns true for basic description") {
            auto desc = make_description("test", "Type");
            auto info = ClassInfo::parse(desc);
            CHECK(tmpl.should_apply(info) == true);
        }

        SUBCASE("Returns true for minimal description") {
            auto desc = make_description("", "");
            auto info = ClassInfo::parse(desc);
            CHECK(tmpl.should_apply(info) == true);
        }

        SUBCASE("Returns true for complex nested namespace") {
            auto desc = make_description(
                "company::project::module",
                "ComplexType");
            auto info = ClassInfo::parse(desc);
            CHECK(tmpl.should_apply(info) == true);
        }

        SUBCASE("Returns true regardless of type name") {
            auto desc1 = make_description("ns", "TypeA");
            auto info1 = ClassInfo::parse(desc1);
            auto desc2 = make_description("ns", "TypeB");
            auto info2 = ClassInfo::parse(desc2);
            auto desc3 = make_description("ns", "TypeC");
            auto info3 = ClassInfo::parse(desc3);

            CHECK(tmpl.should_apply(info1) == true);
            CHECK(tmpl.should_apply(info2) == true);
            CHECK(tmpl.should_apply(info3) == true);
        }

        SUBCASE("Returns true for different kinds") {
            auto struct_desc = make_description();
            struct_desc.kind = "struct";
            auto struct_info = ClassInfo::parse(struct_desc);

            auto class_desc = make_description();
            class_desc.kind = "class";
            auto class_info = ClassInfo::parse(class_desc);

            CHECK(tmpl.should_apply(struct_info) == true);
            CHECK(tmpl.should_apply(class_info) == true);
        }

        SUBCASE("Consistency - always returns same value") {
            auto desc = make_description();

            auto info1 = ClassInfo::parse(desc);
            bool result1 = tmpl.should_apply(info1);
            auto info2 = ClassInfo::parse(desc);
            bool result2 = tmpl.should_apply(info2);
            auto info3 = ClassInfo::parse(desc);
            bool result3 = tmpl.should_apply(info3);

            CHECK(result1 == true);
            CHECK(result2 == true);
            CHECK(result3 == true);
        }

        SUBCASE("Rationale - main template is the foundation") {
            // The main template ALWAYS applies because every strong type
            // needs the basic class structure. This is documented in the
            // class implementation.
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            CHECK(tmpl.should_apply(info) == true);
        }
    }

    // ========================================================================
    // required_includes() Tests
    // ========================================================================

    TEST_CASE("required_includes() - returns empty set")
    {
        MainTemplate tmpl;

        SUBCASE("Returns empty set") {
            auto includes = tmpl.required_includes();
            CHECK(includes.empty());
            CHECK(includes.size() == 0);
        }

        SUBCASE("Consistency across calls") {
            auto includes1 = tmpl.required_includes();
            auto includes2 = tmpl.required_includes();

            CHECK(includes1 == includes2);
            CHECK(includes1.empty());
        }

        SUBCASE("Rationale - no additional includes needed") {
            // The main template doesn't require any additional headers
            // beyond what's in the preamble. Feature-specific templates
            // (hash, formatter, etc.) will add their own includes.
            auto includes = tmpl.required_includes();
            CHECK(includes.empty());
        }
    }

    // ========================================================================
    // required_preamble() Tests
    // ========================================================================

    TEST_CASE("required_preamble() - returns empty set")
    {
        MainTemplate tmpl;

        SUBCASE("Returns empty set") {
            auto preamble = tmpl.required_preamble();
            CHECK(preamble.empty());
            CHECK(preamble.size() == 0);
        }

        SUBCASE("Consistency across calls") {
            auto preamble1 = tmpl.required_preamble();
            auto preamble2 = tmpl.required_preamble();

            CHECK(preamble1 == preamble2);
            CHECK(preamble1.empty());
        }

        SUBCASE("Rationale - preamble handled at higher level") {
            // The main template relies on the general preamble that's
            // added by the generator infrastructure. No template-specific
            // preamble sections are needed.
            auto preamble = tmpl.required_preamble();
            CHECK(preamble.empty());
        }
    }

    // ========================================================================
    // prepare_variables() Tests
    // ========================================================================

    TEST_CASE("prepare_variables() - returns valid JSON object")
    {
        MainTemplate tmpl;

        SUBCASE("Returns non-empty JSON object") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            CHECK_FALSE(vars.empty());
        }

        SUBCASE("JSON contains expected top-level keys") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            // Should contain ClassInfo fields
            CHECK(vars.contains("class_name"));
            CHECK(vars.contains("class_namespace"));
            CHECK(vars.contains("underlying_type"));
            CHECK(vars.contains("full_class_name"));
        }

        SUBCASE("class_name is extracted correctly") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            REQUIRE(vars.contains("class_name"));
            CHECK(vars["class_name"].is_string());
            CHECK(vars["class_name"].as_string() == "MyType");
        }

        SUBCASE("class_namespace is extracted correctly") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            REQUIRE(vars.contains("class_namespace"));
            CHECK(vars["class_namespace"].is_string());
            CHECK(vars["class_namespace"].as_string() == "myns");
        }

        SUBCASE("underlying_type is extracted correctly") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            REQUIRE(vars.contains("underlying_type"));
            CHECK(vars["underlying_type"].is_string());
            CHECK(vars["underlying_type"].as_string() == "int");
        }

        SUBCASE("Different type names produce different class_name") {
            auto desc1 = make_description("ns", "TypeA");
            auto info1 = ClassInfo::parse(desc1);
            auto desc2 = make_description("ns", "TypeB");
            auto info2 = ClassInfo::parse(desc2);

            auto vars1 = tmpl.prepare_variables(info1);
            auto vars2 = tmpl.prepare_variables(info2);

            CHECK(vars1["class_name"] != vars2["class_name"]);
        }

        SUBCASE("Different namespaces produce different class_namespace") {
            auto desc1 = make_description("ns1", "Type");
            auto info1 = ClassInfo::parse(desc1);
            auto desc2 = make_description("ns2", "Type");
            auto info2 = ClassInfo::parse(desc2);

            auto vars1 = tmpl.prepare_variables(info1);
            auto vars2 = tmpl.prepare_variables(info2);

            CHECK(vars1["class_namespace"] != vars2["class_namespace"]);
        }

        SUBCASE("Contains original description metadata") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            REQUIRE(vars.contains("desc"));
            CHECK(vars["desc"].is_object());

            auto const & desc_obj = vars["desc"].as_object();
            CHECK(desc_obj.contains("type_name"));
            CHECK(desc_obj.contains("type_namespace"));
            CHECK(desc_obj.contains("description"));
        }

        SUBCASE("Variables are consistent for same description") {
            auto desc = make_description("myns", "MyType", "int");

            auto info1 = ClassInfo::parse(desc);
            auto vars1 = tmpl.prepare_variables(info1);
            auto info2 = ClassInfo::parse(desc);
            auto vars2 = tmpl.prepare_variables(info2);

            // Compare key fields
            CHECK(vars1["class_name"] == vars2["class_name"]);
            CHECK(vars1["class_namespace"] == vars2["class_namespace"]);
            CHECK(vars1["underlying_type"] == vars2["underlying_type"]);
        }

        SUBCASE("Complex type with operators") {
            auto desc = make_description("test", "MyType", "int; +, -, *");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            CHECK_FALSE(vars.empty());
            CHECK(vars.contains("class_name"));
        }

        SUBCASE("Type with std::string") {
            auto desc = make_description("test", "MyType", "std::string");
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);

            REQUIRE(vars.contains("underlying_type"));
            CHECK(vars["underlying_type"].as_string() == "std::string");
        }
    }

    // ========================================================================
    // render() Tests
    // ========================================================================

    TEST_CASE("render() - produces valid C++ code")
    {
        MainTemplate tmpl;

        SUBCASE("Renders non-empty code") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK_FALSE(code.empty());
        }

        SUBCASE("Contains type name in rendered code") {
            auto desc = make_description("myapp", "UserId", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "UserId"));
        }

        SUBCASE("Contains namespace in rendered code") {
            auto desc = make_description("myapp", "UserId", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "namespace myapp"));
        }

        SUBCASE("Contains underlying type in rendered code") {
            auto desc = make_description("test", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "int value"));
        }

        SUBCASE("Contains strong_type_tag inheritance") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "atlas::strong_type_tag"));
        }

        SUBCASE("Contains type alias") {
            auto desc = make_description("test", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "using atlas_value_type = int"));
        }

        SUBCASE("Contains constructor") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "template <"));
            CHECK(contains(code, "typename... ArgTs"));
        }

        SUBCASE("Contains explicit cast operators") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "explicit operator"));
        }

        SUBCASE("Struct kind generates struct keyword") {
            auto desc = make_description("test", "MyType", "int");
            desc.kind = "struct";
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "struct MyType"));
        }

        SUBCASE("Class kind generates class keyword") {
            auto desc = make_description("test", "MyType", "int");
            desc.kind = "class";
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "class MyType"));
        }

        SUBCASE("Renders documentation comment") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "@brief"));
            CHECK(contains(code, "Strong type wrapper"));
        }

        SUBCASE("Different types produce different code") {
            auto desc1 = make_description("ns", "TypeA", "int");
            auto info1 = ClassInfo::parse(desc1);
            auto desc2 = make_description("ns", "TypeB", "int");
            auto info2 = ClassInfo::parse(desc2);

            auto code1 = tmpl.render(info1);
            auto code2 = tmpl.render(info2);

            CHECK(code1 != code2);
            CHECK(contains(code1, "TypeA"));
            CHECK(contains(code2, "TypeB"));
        }

        SUBCASE("Nested namespace renders correctly") {
            auto desc = make_description("a::b::c", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            // Should have nested namespace declarations
            CHECK(contains(code, "namespace a"));
            CHECK(contains(code, "namespace b"));
            CHECK(contains(code, "namespace c"));
        }

        SUBCASE("Code contains closing namespace comments") {
            auto desc = make_description("myapp", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            // Should have closing namespace with comment
            CHECK(contains(code, "} // namespace"));
        }

        SUBCASE("Same description produces same code") {
            auto desc = make_description("test", "MyType", "int");

            auto info1 = ClassInfo::parse(desc);
            auto code1 = tmpl.render(info1);
            auto info2 = ClassInfo::parse(desc);
            auto code2 = tmpl.render(info2);

            CHECK(code1 == code2);
        }

        SUBCASE("Renders with std::string type") {
            auto desc = make_description("test", "MyType", "std::string");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "std::string value"));
        }

        SUBCASE("Renders with template type") {
            auto desc = make_description("test", "MyType", "std::vector<int>");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK(contains(code, "std::vector<int>"));
        }
    }

    // ========================================================================
    // Integration Tests
    // ========================================================================

    TEST_CASE("Integration - MainTemplate works with TemplateRegistry")
    {
        auto & registry = TemplateRegistry::instance();
        auto desc = make_description();
        auto info = ClassInfo::parse(desc);

        SUBCASE("MainTemplate is visited by visit_applicable") {
            bool visited = false;
            std::string visited_id;

            registry.visit_applicable(info, [&](ITemplate const & tmpl) {
                if (tmpl.id() == "core.main_structure") {
                    visited = true;
                    visited_id = tmpl.id();
                }
            });

            CHECK(visited);
            CHECK(visited_id == "core.main_structure");
        }

        SUBCASE("MainTemplate applies to all descriptions") {
            // Create diverse descriptions
            std::vector<StrongTypeDescription> descriptions = {
                make_description("ns1", "Type1"),
                make_description("ns2::sub", "Type2"),
                make_description("", "Type3"),
                make_description("very::deep::nested::ns", "Type4")};

            for (auto const & desc : descriptions) {
                auto info = ClassInfo::parse(desc);
                bool main_template_applied = false;

                registry.visit_applicable(info, [&](ITemplate const & tmpl) {
                    if (tmpl.id() == "core.main_structure") {
                        main_template_applied = true;
                    }
                });

                CHECK(main_template_applied);
            }
        }

        SUBCASE("Can render via registry") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto const * tmpl = registry.get_template("core.main_structure");

            REQUIRE(tmpl != nullptr);

            auto code = tmpl->render(info);
            CHECK(contains(code, "MyType"));
            CHECK(contains(code, "myns"));
        }

        SUBCASE("Can prepare_variables via registry") {
            auto desc = make_description("myns", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto const * tmpl = registry.get_template("core.main_structure");

            REQUIRE(tmpl != nullptr);

            auto vars = tmpl->prepare_variables(info);
            CHECK(vars.contains("class_name"));
            CHECK(vars["class_name"].as_string() == "MyType");
        }
    }

    // ========================================================================
    // Template Structure Validation Tests
    // ========================================================================

    TEST_CASE("Template Structure - validates Mustache syntax")
    {
        MainTemplate tmpl;
        auto template_str = tmpl.get_template();

        SUBCASE("All opening sections have matching closing sections") {
            // Check a few key sections
            std::vector<std::string> sections = {
                "namespace_open",
                "namespace_close",
                "has_default_value",
                "public_specifier",
                "has_constraint",
                "is_bounded",
                "delete_default_constructor",
                "constants",
                "hash_specialization",
                "formatter_specialization"};

            for (auto const & section : sections) {
                std::string opening = "{{#" + section + "}}";
                std::string closing = "{{/" + section + "}}";

                bool has_opening = contains(template_str, opening);
                bool has_closing = contains(template_str, closing);

                // If section exists, both opening and closing must exist
                if (has_opening || has_closing) {
                    CHECK(has_opening);
                    CHECK(has_closing);
                }
            }
        }

        SUBCASE("Contains expected variable substitutions") {
            // Triple-brace for unescaped HTML (used for code generation)
            CHECK(contains(template_str, "{{{underlying_type}}}"));
            CHECK(contains(template_str, "{{{full_class_name}}}"));
            CHECK(contains(template_str, "{{{class_name}}}"));
            CHECK(contains(template_str, "{{{const_expr}}}"));
        }

        SUBCASE("Contains expected partial references") {
            // Partials are included with {{>partial_name}}
            std::vector<std::string> partials = {
                "constant_declarations",
                "template_assignment_operator",
                "explicit_cast_operator",
                "implicit_cast_operator",
                "arithmetic_binary_operators",
                "relational_operator",
                "hash_specialization",
                "formatter_specialization"};

            for (auto const & partial : partials) {
                std::string partial_ref = "{{>" + partial + "}}";
                CHECK(contains(template_str, partial_ref));
            }
        }
    }

    // ========================================================================
    // Comparison with Other Templates Tests
    // ========================================================================

    TEST_CASE("Comparison - MainTemplate vs other templates")
    {
        MainTemplate main_tmpl;
        auto & registry = TemplateRegistry::instance();

        SUBCASE("MainTemplate has unique ID") {
            // Ensure no other template has the same ID
            std::vector<std::string> all_ids;

            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            registry.visit_applicable(info, [&](ITemplate const & tmpl) {
                all_ids.push_back(tmpl.id());
            });

            // Count how many times core.main_structure appears
            int count = std::count(
                all_ids.begin(),
                all_ids.end(),
                "core.main_structure");

            // Should appear exactly once
            CHECK(count == 1);
        }

        SUBCASE("MainTemplate is fundamental (core namespace)") {
            // ID should start with "core" prefix
            std::string id = main_tmpl.id();
            CHECK(id.find("core") == 0);
        }
    }

    // ========================================================================
    // Error Handling Tests
    // ========================================================================

    TEST_CASE("Error Handling - graceful behavior with invalid input")
    {
        MainTemplate tmpl;

        SUBCASE("should_apply handles empty description") {
            StrongTypeDescription empty_desc;
            auto info = ClassInfo::parse(empty_desc);
            // Should not crash - always returns true
            auto result = tmpl.should_apply(info);
            CHECK(result == true);
        }

        SUBCASE("get_template is safe to call multiple times") {
            auto t1 = tmpl.get_template();
            auto t2 = tmpl.get_template();
            auto t3 = tmpl.get_template();
            CHECK_FALSE(t1.empty());
            CHECK(t1 == t2);
            CHECK(t2 == t3);
        }

        SUBCASE("required_includes never throws") {
            auto inc = tmpl.required_includes();
            CHECK(inc.empty());
        }

        SUBCASE("required_preamble never throws") {
            auto pre = tmpl.required_preamble();
            CHECK(pre.empty());
        }

        SUBCASE("prepare_variables handles valid description") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto vars = tmpl.prepare_variables(info);
            CHECK_FALSE(vars.empty());
        }

        SUBCASE("render handles valid description") {
            auto desc = make_description();
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);
            CHECK_FALSE(code.empty());
        }
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("Edge Cases - unusual but valid inputs")
    {
        MainTemplate tmpl;

        SUBCASE("Empty namespace") {
            auto desc = make_description("", "MyType", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK_FALSE(code.empty());
            CHECK(contains(code, "MyType"));
        }

        SUBCASE("Single character type name") {
            auto desc = make_description("ns", "T", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK_FALSE(code.empty());
            CHECK(contains(code, "struct T"));
        }

        SUBCASE("Long nested namespace") {
            auto desc = make_description("a::b::c::d::e::f", "Type", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK_FALSE(code.empty());
        }

        SUBCASE("Type name with underscores") {
            auto desc = make_description("test", "my_special_type", "int");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK_FALSE(code.empty());
            CHECK(contains(code, "my_special_type"));
        }

        SUBCASE("Complex template type") {
            auto desc = make_description(
                "test",
                "MyType",
                "std::map<std::string, int>");
            auto info = ClassInfo::parse(desc);
            auto code = tmpl.render(info);

            CHECK_FALSE(code.empty());
            CHECK(contains(code, "std::map<std::string, int>"));
        }
    }
}

} // anonymous namespace
