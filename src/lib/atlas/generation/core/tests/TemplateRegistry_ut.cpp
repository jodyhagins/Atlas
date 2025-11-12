// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file TemplateRegistry_ut.cpp
 * Comprehensive unit tests for TemplateRegistry
 *
 * Tests cover:
 * - Singleton pattern verification
 * - Template registration (successful and duplicate detection)
 * - Template retrieval and existence checking
 * - visit_applicable() filtering based on should_apply()
 * - Empty registry behavior
 * - Multiple template registration and iteration order
 * - Clear functionality
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/core/TemplateRegistry.hpp"

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/ITemplate.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "tests/doctest.hpp"

namespace {

using namespace wjh::atlas;
using namespace wjh::atlas::generation;
using generation::ClassInfo;

// ============================================================================
// Mock Template Classes for Testing
// ============================================================================

/**
 * Simple mock template that always applies
 */
class AlwaysAppliesTemplate
: public ITemplate
{
public:
    explicit AlwaysAppliesTemplate(std::string id = "test.always_applies")
    : id_(std::move(id))
    { }

protected:
    std::string id_impl() const override { return id_; }

    std::string_view get_template_impl() const override
    {
        static constexpr char const tmpl[] = "// Always applies template\n";
        return tmpl;
    }

    bool should_apply_impl(ClassInfo const &) const override { return true; }

    boost::json::object prepare_variables_impl(ClassInfo const &) const override
    {
        return boost::json::object{};
    }

private:
    std::string id_;
};

/**
 * Mock template that never applies
 */
class NeverAppliesTemplate
: public ITemplate
{
public:
    explicit NeverAppliesTemplate(std::string id = "test.never_applies")
    : id_(std::move(id))
    { }

protected:
    std::string id_impl() const override { return id_; }

    std::string_view get_template_impl() const override
    {
        static constexpr char const tmpl[] = "// Never applies template\n";
        return tmpl;
    }

    bool should_apply_impl(ClassInfo const &) const override { return false; }

    boost::json::object prepare_variables_impl(ClassInfo const &) const override
    {
        return boost::json::object{};
    }

private:
    std::string id_;
};

/**
 * Mock template that applies only if type_name matches
 */
class ConditionalTemplate
: public ITemplate
{
public:
    explicit ConditionalTemplate(std::string id, std::string required_type_name)
    : id_(std::move(id))
    , required_type_name_(std::move(required_type_name))
    { }

protected:
    std::string id_impl() const override { return id_; }

    std::string_view get_template_impl() const override
    {
        static constexpr char const tmpl[] =
            "// Conditional template for {{{type_name}}}\n";
        return tmpl;
    }

    bool should_apply_impl(ClassInfo const & info) const override
    {
        return info.class_name == required_type_name_;
    }

    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override
    {
        boost::json::object vars;
        vars["type_name"] = info.class_name;
        return vars;
    }

private:
    std::string id_;
    std::string required_type_name_;
};

/**
 * Mock template with required includes
 */
class TemplateWithIncludes
: public ITemplate
{
protected:
    std::string id_impl() const override { return "test.with_includes"; }

    std::string_view get_template_impl() const override
    {
        static constexpr char const tmpl[] = "// Template with includes\n";
        return tmpl;
    }

    bool should_apply_impl(ClassInfo const &) const override { return true; }

    boost::json::object prepare_variables_impl(ClassInfo const &) const override
    {
        return boost::json::object{};
    }

    std::set<std::string> required_includes_impl() const override
    {
        return {"<functional>", "<utility>", "<type_traits>"};
    }
};

/**
 * Helper to create a basic StrongTypeDescription for testing
 */
StrongTypeDescription
create_test_description(
    std::string type_name = "TestType",
    std::string type_namespace = "test",
    std::string description = "int")
{
    StrongTypeDescription desc;
    desc.type_name = std::move(type_name);
    desc.type_namespace = std::move(type_namespace);
    desc.description = std::move(description);
    desc.kind = "struct";
    return desc;
}

} // anonymous namespace

// ============================================================================
// Test Suite
// ============================================================================

TEST_SUITE("TemplateRegistry Tests")
{
    // Setup/teardown: Clear registry before each test to ensure isolation
    struct RegistryFixture
    {
        RegistryFixture() { TemplateRegistry::instance().clear(); }

        ~RegistryFixture() { TemplateRegistry::instance().clear(); }
    };

    // ========================================================================
    // Singleton Pattern Tests
    // ========================================================================

    TEST_CASE("Singleton - instance() always returns same object")
    {
        RegistryFixture fixture;

        auto & instance1 = TemplateRegistry::instance();
        auto & instance2 = TemplateRegistry::instance();

        // Verify they are the same object (same address)
        CHECK(&instance1 == &instance2);
    }

    TEST_CASE("Singleton - modifications persist across instance() calls")
    {
        RegistryFixture fixture;

        auto & registry1 = TemplateRegistry::instance();
        registry1.register_template(std::make_unique<AlwaysAppliesTemplate>());

        auto & registry2 = TemplateRegistry::instance();
        CHECK(registry2.size() == 1);
        CHECK(registry2.has_template("test.always_applies"));
    }

    // ========================================================================
    // Template Registration Tests
    // ========================================================================

    TEST_CASE("Registration - can register single template")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK(registry.size() == 0);

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());

        CHECK(registry.size() == 1);
        CHECK(registry.has_template("test.always_applies"));
    }

    TEST_CASE("Registration - can register multiple templates")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());
        registry.register_template(std::make_unique<NeverAppliesTemplate>());
        registry.register_template(std::make_unique<TemplateWithIncludes>());

        CHECK(registry.size() == 3);
        CHECK(registry.has_template("test.always_applies"));
        CHECK(registry.has_template("test.never_applies"));
        CHECK(registry.has_template("test.with_includes"));
    }

    TEST_CASE("Registration - throws on null template pointer")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK_THROWS_AS(
            registry.register_template(nullptr),
            std::runtime_error);
    }

    TEST_CASE("Registration - throws on duplicate template ID")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("duplicate.id"));

        CHECK_THROWS_AS(
            registry.register_template(
                std::make_unique<AlwaysAppliesTemplate>("duplicate.id")),
            std::runtime_error);
    }

    TEST_CASE("Registration - exception message contains duplicate ID")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("my.unique.id"));

        try {
            registry.register_template(
                std::make_unique<AlwaysAppliesTemplate>("my.unique.id"));
            FAIL("Expected exception not thrown");
        } catch (std::runtime_error const & e) {
            std::string message = e.what();
            CHECK(message.find("my.unique.id") != std::string::npos);
            CHECK(message.find("duplicate") != std::string::npos);
        }
    }

    // ========================================================================
    // Template Retrieval Tests
    // ========================================================================

    TEST_CASE(
        "Retrieval - get_template returns valid pointer for existing template")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());

        auto const * tmpl = registry.get_template("test.always_applies");
        REQUIRE(tmpl != nullptr);
        CHECK(tmpl->id() == "test.always_applies");
    }

    TEST_CASE(
        "Retrieval - get_template returns nullptr for non-existent template")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        auto const * tmpl = registry.get_template("nonexistent.template");
        CHECK(tmpl == nullptr);
    }

    TEST_CASE("Retrieval - has_template returns true for existing template")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());

        CHECK(registry.has_template("test.always_applies"));
    }

    TEST_CASE(
        "Retrieval - has_template returns false for non-existent template")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK_FALSE(registry.has_template("nonexistent.template"));
    }

    // ========================================================================
    // Empty Registry Tests
    // ========================================================================

    TEST_CASE("Empty registry - size is zero")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK(registry.size() == 0);
    }

    TEST_CASE("Empty registry - visit_applicable does not invoke visitor")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        int visit_count = 0;
        registry.visit_applicable(info, [&](ITemplate const &) {
            ++visit_count;
        });

        CHECK(visit_count == 0);
    }

    TEST_CASE("Empty registry - has_template always returns false")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK_FALSE(registry.has_template("any.template"));
        CHECK_FALSE(registry.has_template(""));
    }

    TEST_CASE("Empty registry - get_template always returns nullptr")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK(registry.get_template("any.template") == nullptr);
        CHECK(registry.get_template("") == nullptr);
    }

    // ========================================================================
    // visit_applicable() Tests
    // ========================================================================

    TEST_CASE(
        "visit_applicable - visits only templates where should_apply() is true")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("always1"));
        registry.register_template(
            std::make_unique<NeverAppliesTemplate>("never1"));
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("always2"));
        registry.register_template(
            std::make_unique<NeverAppliesTemplate>("never2"));

        std::vector<std::string> visited_ids;
        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            visited_ids.push_back(tmpl.id());
        });

        // Should only visit the "always" templates
        REQUIRE(visited_ids.size() == 2);
        CHECK(
            std::find(visited_ids.begin(), visited_ids.end(), "always1") !=
            visited_ids.end());
        CHECK(
            std::find(visited_ids.begin(), visited_ids.end(), "always2") !=
            visited_ids.end());
    }

    TEST_CASE("visit_applicable - respects conditional template logic")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(
            std::make_unique<ConditionalTemplate>("cond1", "TypeA"));
        registry.register_template(
            std::make_unique<ConditionalTemplate>("cond2", "TypeB"));
        registry.register_template(
            std::make_unique<ConditionalTemplate>("cond3", "TypeA"));

        SUBCASE("Visits templates matching TypeA") {
            auto desc = create_test_description("TypeA");
            auto info = ClassInfo::parse(desc);

            std::vector<std::string> visited_ids;
            registry.visit_applicable(info, [&](ITemplate const & tmpl) {
                visited_ids.push_back(tmpl.id());
            });

            REQUIRE(visited_ids.size() == 2);
            CHECK(
                std::find(visited_ids.begin(), visited_ids.end(), "cond1") !=
                visited_ids.end());
            CHECK(
                std::find(visited_ids.begin(), visited_ids.end(), "cond3") !=
                visited_ids.end());
        }

        SUBCASE("Visits templates matching TypeB") {
            auto desc = create_test_description("TypeB");
            auto info = ClassInfo::parse(desc);

            std::vector<std::string> visited_ids;
            registry.visit_applicable(info, [&](ITemplate const & tmpl) {
                visited_ids.push_back(tmpl.id());
            });

            REQUIRE(visited_ids.size() == 1);
            CHECK(visited_ids[0] == "cond2");
        }

        SUBCASE("Visits no templates for TypeC") {
            auto desc = create_test_description("TypeC");
            auto info = ClassInfo::parse(desc);

            int visit_count = 0;
            registry.visit_applicable(info, [&](ITemplate const &) {
                ++visit_count;
            });

            CHECK(visit_count == 0);
        }
    }

    TEST_CASE("visit_applicable - visitor can access template properties")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        registry.register_template(std::make_unique<TemplateWithIncludes>());

        bool visited = false;
        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            visited = true;

            // Verify we can call various template member functions
            CHECK(tmpl.id() == "test.with_includes");
            CHECK(tmpl.should_apply(info) == true);

            auto includes = tmpl.required_includes();
            CHECK(includes.size() == 3);
            CHECK(includes.count("<functional>") == 1);
            CHECK(includes.count("<utility>") == 1);
            CHECK(includes.count("<type_traits>") == 1);

            auto rendered = tmpl.render(info);
            CHECK_FALSE(rendered.empty());
        });

        CHECK(visited);
    }

    TEST_CASE("visit_applicable - works with lambda capturing by reference")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("template1"));
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("template2"));

        int counter = 0;
        std::string concatenated_ids;

        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            ++counter;
            concatenated_ids += tmpl.id() + ";";
        });

        CHECK(counter == 2);
        CHECK(concatenated_ids == "template1;template2;");
    }

    TEST_CASE("visit_applicable - works with lambda capturing by value")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());

        std::string prefix = "PREFIX:";
        std::string result;

        registry.visit_applicable(
            info,
            [prefix, &result](ITemplate const & tmpl) {
                result = prefix + tmpl.id();
            });

        CHECK(result == "PREFIX:test.always_applies");
    }

    // ========================================================================
    // Iteration Order Tests
    // ========================================================================

    TEST_CASE("Iteration order - templates visited in deterministic order "
              "(sorted by ID)")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        // Register in non-alphabetical order
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("z_last"));
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("a_first"));
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("m_middle"));

        std::vector<std::string> visited_ids;
        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            visited_ids.push_back(tmpl.id());
        });

        // Should be visited in sorted order
        REQUIRE(visited_ids.size() == 3);
        CHECK(visited_ids[0] == "a_first");
        CHECK(visited_ids[1] == "m_middle");
        CHECK(visited_ids[2] == "z_last");
    }

    TEST_CASE("Iteration order - order is consistent across multiple visits")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("template3"));
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("template1"));
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("template2"));

        // First visit
        std::vector<std::string> first_visit;
        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            first_visit.push_back(tmpl.id());
        });

        // Second visit
        std::vector<std::string> second_visit;
        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            second_visit.push_back(tmpl.id());
        });

        // Should be identical
        CHECK(first_visit == second_visit);
    }

    // ========================================================================
    // Clear Tests
    // ========================================================================

    TEST_CASE("Clear - removes all templates")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());
        registry.register_template(std::make_unique<NeverAppliesTemplate>());
        registry.register_template(std::make_unique<TemplateWithIncludes>());

        REQUIRE(registry.size() == 3);

        registry.clear();

        CHECK(registry.size() == 0);
        CHECK_FALSE(registry.has_template("test.always_applies"));
        CHECK_FALSE(registry.has_template("test.never_applies"));
        CHECK_FALSE(registry.has_template("test.with_includes"));
    }

    TEST_CASE("Clear - allows re-registration after clear")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        // Register, clear, and re-register same ID
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("reusable.id"));
        registry.clear();

        // Should not throw - ID is available again
        CHECK_NOTHROW(registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("reusable.id")));

        CHECK(registry.size() == 1);
        CHECK(registry.has_template("reusable.id"));
    }

    TEST_CASE("Clear - clearing empty registry is safe")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        CHECK(registry.size() == 0);
        CHECK_NOTHROW(registry.clear());
        CHECK(registry.size() == 0);
    }

    // ========================================================================
    // Integration Tests
    // ========================================================================

    TEST_CASE("Integration - complete workflow with multiple template types")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();

        // Register diverse templates
        registry.register_template(
            std::make_unique<AlwaysAppliesTemplate>("always"));
        registry.register_template(
            std::make_unique<NeverAppliesTemplate>("never"));
        registry.register_template(
            std::make_unique<ConditionalTemplate>("cond", "MyType"));
        registry.register_template(std::make_unique<TemplateWithIncludes>());

        // Check registry state
        CHECK(registry.size() == 4);

        // Test with matching type
        auto matching_desc = create_test_description("MyType");
        auto matching_info = ClassInfo::parse(matching_desc);
        std::vector<std::string> matching_visits;

        registry.visit_applicable(matching_info, [&](ITemplate const & tmpl) {
            matching_visits.push_back(tmpl.id());
        });

        // Should visit: always, cond, test.with_includes (not never)
        CHECK(matching_visits.size() == 3);

        // Test with non-matching type
        auto non_matching_desc = create_test_description("OtherType");
        auto non_matching_info = ClassInfo::parse(non_matching_desc);
        std::vector<std::string> non_matching_visits;

        registry.visit_applicable(
            non_matching_info,
            [&](ITemplate const & tmpl) {
                non_matching_visits.push_back(tmpl.id());
            });

        // Should visit: always, test.with_includes (not never or cond)
        CHECK(non_matching_visits.size() == 2);
    }

    TEST_CASE(
        "Integration - visitor can accumulate data from multiple templates")
    {
        RegistryFixture fixture;
        auto & registry = TemplateRegistry::instance();
        auto desc = create_test_description();
        auto info = ClassInfo::parse(desc);

        registry.register_template(std::make_unique<AlwaysAppliesTemplate>());
        registry.register_template(std::make_unique<TemplateWithIncludes>());

        // Accumulate all required includes
        std::set<std::string> all_includes;
        registry.visit_applicable(info, [&](ITemplate const & tmpl) {
            auto includes = tmpl.required_includes();
            all_includes.insert(includes.begin(), includes.end());
        });

        // Should have includes from TemplateWithIncludes
        CHECK(all_includes.size() == 3);
        CHECK(all_includes.count("<functional>") == 1);
        CHECK(all_includes.count("<utility>") == 1);
        CHECK(all_includes.count("<type_traits>") == 1);
    }
}
