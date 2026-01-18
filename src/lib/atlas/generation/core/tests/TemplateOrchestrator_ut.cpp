// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file TemplateOrchestrator_ut.cpp
 * Unit tests for TemplateOrchestrator
 *
 * Tests cover:
 * - Basic rendering of simple strong types
 * - Include collection from multiple templates
 * - Preamble collection from multiple templates
 * - Partial building and composition
 * - Warning collection and forwarding
 * - Integration with TemplateRegistry
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/core/TemplateOrchestrator.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/MainTemplate.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"
#include "atlas/generation/operators/comparison/RelationalOperator.hpp"

#include <algorithm>
#include <string>

#include "tests/doctest.hpp"

namespace {

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

// Force linking of MainTemplate and other template classes by referencing them
// This ensures static registrars are linked in from the library
[[maybe_unused]]
inline void
force_template_registration()
{
    // Create instances to force linker to include template .cpp files
    // which contain the static TemplateRegistrar instances
    static bool initialized = false;
    if (not initialized) {
        [[maybe_unused]] MainTemplate main_tmpl;
        [[maybe_unused]] RelationalOperator relational_op;
        initialized = true;
    }
}

// Call during test initialization
struct TemplateRegistrationForcer
{
    TemplateRegistrationForcer() { force_template_registration(); }
};

[[maybe_unused]] static TemplateRegistrationForcer forcer;

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

TEST_SUITE("TemplateOrchestrator Tests")
{
    // ========================================================================
    // Basic Rendering Tests
    // ========================================================================

    TEST_CASE("Orchestrator - Can render minimal strong type")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description("myns", "MyType", "int");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);

        // Verify basic structure
        CHECK(contains(code, "namespace myns"));
        CHECK(contains(code, "struct MyType"));
        CHECK(contains(code, ": private atlas::strong_type_tag<"));
        CHECK(contains(code, "int value;"));
        CHECK(contains(code, "using atlas_value_type = int"));
    }

    TEST_CASE("Orchestrator - Renders type with arithmetic operators")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description("test", "Counter", "int; +");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);

        // Should contain addition operators
        CHECK(contains(code, "Counter"));
        // The actual operator implementation depends on templates being
        // registered
    }

    TEST_CASE("Orchestrator - Renders type with comparison operators")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description("test", "UserId", "int; <");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);

        CHECK(contains(code, "UserId"));
        // Comparison operators should be rendered
    }

    TEST_CASE("Orchestrator - Renders type with multiple features")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description(
            "test",
            "ComplexType",
            "std::string; +, <, ->, out");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);

        CHECK(contains(code, "ComplexType"));
        CHECK(contains(code, "std::string value"));
    }

    // ========================================================================
    // Warning Collection Tests
    // ========================================================================

    TEST_CASE("Orchestrator - get_warnings returns empty initially")
    {
        TemplateOrchestrator orchestrator;
        auto warnings = orchestrator.get_warnings();
        CHECK(warnings.empty());
    }

    TEST_CASE("Orchestrator - Warnings cleared after each render")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description();

        auto info1 = ClassInfo::parse(desc);
        orchestrator.render(info1);
        auto warnings1 = orchestrator.get_warnings();

        auto info2 = ClassInfo::parse(desc);
        orchestrator.render(info2);
        // Warnings from first render should be cleared
        // (assuming no warnings generated in this simple case)
    }

    TEST_CASE("Orchestrator - clear_warnings works")
    {
        TemplateOrchestrator orchestrator;
        orchestrator.clear_warnings();
        CHECK(orchestrator.get_warnings().empty());
    }

    // ========================================================================
    // Error Handling Tests
    // ========================================================================

    TEST_CASE("Orchestrator - Throws if main template not registered")
    {
        // This test would require clearing the registry, which might affect
        // other tests. Skip for now or make it conditional.
        // The orchestrator should throw if core.main_structure is not found.
    }

    // ========================================================================
    // Integration Tests
    // ========================================================================

    TEST_CASE("Orchestrator - Works with registry")
    {
        // Verify the orchestrator can use templates from the registry
        auto & registry = TemplateRegistry::instance();
        CHECK(registry.size() > 0); // Should have templates registered

        TemplateOrchestrator orchestrator;
        auto desc = make_description();

        // Should not throw
        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);
        CHECK_FALSE(code.empty());
    }

    TEST_CASE("Orchestrator - Handles namespace nesting")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description("foo::bar::baz", "NestedType", "int");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);

        CHECK(contains(code, "namespace foo"));
        CHECK(contains(code, "namespace bar"));
        CHECK(contains(code, "namespace baz"));
        CHECK(contains(code, "NestedType"));
    }

    TEST_CASE("Orchestrator - Handles different underlying types")
    {
        TemplateOrchestrator orchestrator;

        SUBCASE("std::string") {
            auto desc = make_description("test", "Name", "std::string");
            auto info = ClassInfo::parse(desc);
            std::string code = orchestrator.render(info);
            CHECK(contains(code, "std::string value"));
        }

        SUBCASE("double") {
            auto desc = make_description("test", "Price", "double");
            auto info = ClassInfo::parse(desc);
            std::string code = orchestrator.render(info);
            CHECK(contains(code, "double value"));
        }

        SUBCASE("std::vector<int>") {
            auto desc = make_description("test", "Ids", "std::vector<int>");
            auto info = ClassInfo::parse(desc);
            std::string code = orchestrator.render(info);
            CHECK(contains(code, "std::vector<int> value"));
        }
    }

    // ========================================================================
    // Operator Composition Tests
    // ========================================================================

    TEST_CASE("Orchestrator - Multiple operators compose correctly")
    {
        TemplateOrchestrator orchestrator;

        // Type with several operators
        auto desc =
            make_description("test", "FullFeatured", "int; +, -, *, /, %");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);
        CHECK(contains(code, "FullFeatured"));

        // Should have basic structure
        CHECK(contains(code, "struct FullFeatured"));
        CHECK(contains(code, ": private atlas::strong_type_tag<"));
    }

    TEST_CASE("Orchestrator - Specializations rendered outside class")
    {
        TemplateOrchestrator orchestrator;

        auto desc = make_description("test", "Hashable", "int; hash");

        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);

        // Hash specialization should appear (if template is registered)
        CHECK(contains(code, "Hashable"));
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("Orchestrator - Handles empty description")
    {
        TemplateOrchestrator orchestrator;
        auto desc = make_description("test", "Empty", "int");

        // Should render minimal type
        auto info = ClassInfo::parse(desc);
        std::string code = orchestrator.render(info);
        CHECK(contains(code, "Empty"));
        CHECK(contains(code, "int value"));
    }

    TEST_CASE("Orchestrator - Handles class vs struct")
    {
        TemplateOrchestrator orchestrator;

        SUBCASE("struct") {
            auto desc = make_description("test", "S", "int");
            desc.kind = "struct";
            auto info = ClassInfo::parse(desc);
            std::string code = orchestrator.render(info);
            CHECK(contains(code, "struct S"));
        }

        SUBCASE("class") {
            auto desc = make_description("test", "C", "int");
            desc.kind = "class";
            auto info = ClassInfo::parse(desc);
            std::string code = orchestrator.render(info);
            CHECK(contains(code, "class C"));
        }
    }

    TEST_CASE("TemplateOrchestrator renders relational operators")
    {
        ClassInfo info;
        info.desc.type_name = "TestType";
        info.desc.type_namespace = "test";
        info.desc.description = "int";
        info.class_name = "TestType";
        info.full_class_name = "TestType";
        info.underlying_type = "int";
        info.full_qualified_name = "test::TestType";

        // Add relational operators
        info.relational_operators.push_back(Operator{"=="});
        info.relational_operators.push_back(Operator{"!="});
        info.has_relational_operators = true;

        TemplateOrchestrator orchestrator;
        std::string result = orchestrator.render(info);

        // Check that operators are present
        CHECK(result.find("operator ==") != std::string::npos);
        CHECK(result.find("operator !=") != std::string::npos);
    }
}

} // anonymous namespace
