// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file GuardGenerator_ut.cpp
 * Comprehensive unit tests for GuardGenerator
 *
 * Tests cover:
 * - Notice banner generation and formatting
 * - Header guard generation with various namespace/type combinations
 * - Guard prefix customization
 * - Guard separator customization
 * - Case sensitivity (upcase_guard)
 * - Content-addressable guards (SHA1 hash changes)
 * - Namespace colon stripping
 * - Valid C++ identifier generation
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/generation/core/GuardGenerator.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "tests/doctest.hpp"

namespace {

using namespace wjh::atlas;
using namespace wjh::atlas::generation;

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
    std::string guard_prefix = "",
    std::string guard_separator = "_",
    bool upcase_guard = true)
{
    StrongTypeDescription desc;
    desc.type_namespace = std::move(type_namespace);
    desc.type_name = std::move(type_name);
    desc.guard_prefix = std::move(guard_prefix);
    desc.guard_separator = std::move(guard_separator);
    desc.upcase_guard = upcase_guard;
    return desc;
}

/**
 * Check if a string is a valid C++ identifier (no colons, valid chars)
 */
bool
is_valid_cpp_identifier(std::string const & s)
{
    if (s.empty()) {
        return false;
    }

    // First character must be letter or underscore
    if (not std::isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_') {
        return false;
    }

    // Remaining characters must be alphanumeric or underscore
    // Also check for no colons (would make invalid macro)
    for (char c : s) {
        if (not std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }

    return true;
}

/**
 * Check if string contains any colons (invalid for C++ macros)
 */
bool
contains_colons(std::string const & s)
{
    return s.find(':') != std::string::npos;
}

/**
 * Check if string is all uppercase
 */
bool
is_uppercase(std::string const & s)
{
    return std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return not std::isalpha(c) || std::isupper(c);
    });
}

// ============================================================================
// Test Cases
// ============================================================================

TEST_SUITE("GuardGenerator")
{
    // ========================================================================
    // make_notice_banner() Tests
    // ========================================================================

    TEST_CASE("make_notice_banner() - Basic Properties")
    {
        auto banner = GuardGenerator::make_notice_banner();

        SUBCASE("Returns non-empty string") {
            CHECK_FALSE(banner.empty());
        }

        SUBCASE("Contains multiple lines") {
            // Should have multiple newlines for multi-line banner
            auto newline_count = std::count(banner.begin(), banner.end(), '\n');
            CHECK(newline_count > 5);
        }

        SUBCASE("Contains key warning phrases") {
            CHECK(banner.find("DO NOT EDIT") != std::string::npos);
            CHECK(banner.find("NOTICE") != std::string::npos);
            CHECK(
                banner.find("Atlas Strong Type Generator") !=
                std::string::npos);
        }

        SUBCASE("Contains version string") {
            // Should include the version from codegen::version_string
            CHECK(banner.find("0.1.0") != std::string::npos);
        }

        SUBCASE("Contains project URL") {
            CHECK(
                banner.find("https://github.com/jodyhagins/Atlas") !=
                std::string::npos);
        }

        SUBCASE("Has proper comment formatting") {
            // Each line should start with "//"
            std::istringstream iss(banner);
            std::string line;
            while (std::getline(iss, line)) {
                if (not line.empty()) {
                    CHECK(line.substr(0, 2) == "//");
                }
            }
        }

        SUBCASE("Contains visual separators") {
            // Should have ====== and ------ lines
            CHECK(banner.find("======") != std::string::npos);
            CHECK(banner.find("------") != std::string::npos);
        }

        SUBCASE("Banner is deterministic") {
            // Multiple calls should produce identical results
            auto banner1 = GuardGenerator::make_notice_banner();
            auto banner2 = GuardGenerator::make_notice_banner();
            CHECK(banner1 == banner2);
        }
    }

    // ========================================================================
    // make_guard() Tests - Basic Functionality
    // ========================================================================

    TEST_CASE("make_guard() - Basic Namespace and Type Name")
    {
        auto desc = make_description("myns", "MyType");
        std::string code = "// some code";
        auto guard = GuardGenerator::make_guard(desc, code);

        SUBCASE("Returns non-empty string") {
            CHECK_FALSE(guard.empty());
        }

        SUBCASE("Contains namespace") {
            // Should contain "MYNS" (uppercase) in the guard
            CHECK(guard.find("MYNS") != std::string::npos);
        }

        SUBCASE("Contains type name") {
            // Should contain "MYTYPE" (uppercase) in the guard
            CHECK(guard.find("MYTYPE") != std::string::npos);
        }

        SUBCASE("Contains no colons") {
            // Colons should be stripped/replaced
            CHECK_FALSE(contains_colons(guard));
        }

        SUBCASE("Is valid C++ identifier") {
            // Guard should be valid for use as a macro
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Is uppercase by default") {
            // Default upcase_guard = true
            CHECK(is_uppercase(guard));
        }
    }

    TEST_CASE("make_guard() - Nested Namespaces")
    {
        SUBCASE("Two-level namespace") {
            auto desc = make_description("acme::util", "UserId");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("ACME") != std::string::npos);
            CHECK(guard.find("UTIL") != std::string::npos);
            CHECK(guard.find("USERID") != std::string::npos);
            CHECK_FALSE(contains_colons(guard));
        }

        SUBCASE("Three-level namespace") {
            auto desc = make_description(
                "company::project::module",
                "DataType");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("COMPANY") != std::string::npos);
            CHECK(guard.find("PROJECT") != std::string::npos);
            CHECK(guard.find("MODULE") != std::string::npos);
            CHECK(guard.find("DATATYPE") != std::string::npos);
            CHECK_FALSE(contains_colons(guard));
        }

        SUBCASE("Deep nesting") {
            auto desc = make_description("a::b::c::d::e", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
        }
    }

    TEST_CASE("make_guard() - Leading and Trailing Colons")
    {
        SUBCASE("Leading colons stripped") {
            auto desc = make_description("::myns", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
            // Should not start with separator
            CHECK(guard[0] != '_');
        }

        SUBCASE("Trailing colons stripped") {
            auto desc = make_description("myns::", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Both leading and trailing colons") {
            auto desc = make_description("::myns::", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Multiple leading colons") {
            auto desc = make_description("::::myns", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
        }
    }

    TEST_CASE("make_guard() - Empty Namespace")
    {
        auto desc = make_description("", "Type");
        auto guard = GuardGenerator::make_guard(desc, "code");

        SUBCASE("Still generates valid guard") {
            CHECK_FALSE(guard.empty());
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Contains type name") {
            CHECK(guard.find("TYPE") != std::string::npos);
        }

        SUBCASE("Contains no colons") {
            CHECK_FALSE(contains_colons(guard));
        }
    }

    // ========================================================================
    // make_guard() Tests - Custom Guard Prefix
    // ========================================================================

    TEST_CASE("make_guard() - Custom Guard Prefix")
    {
        SUBCASE("Uses prefix instead of namespace/type") {
            auto desc =
                make_description("myns", "MyType", "CUSTOM_PREFIX", "_", true);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Should start with custom prefix
            CHECK(guard.find("CUSTOM_PREFIX") == 0);

            // Should NOT contain namespace or type name
            CHECK(guard.find("MYNS") == std::string::npos);
            CHECK(guard.find("MYTYPE") == std::string::npos);
        }

        SUBCASE("Prefix with different case") {
            auto desc =
                make_description("ns", "Type", "MyCustomPrefix", "_", true);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Should be uppercased due to upcase_guard=true
            CHECK(guard.find("MYCUSTOMPREFIX") == 0);
        }

        SUBCASE("Prefix is not uppercased when upcase_guard=false") {
            auto desc =
                make_description("ns", "Type", "MyCustomPrefix", "_", false);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Should maintain original case
            CHECK(guard.find("MyCustomPrefix") == 0);
        }
    }

    // ========================================================================
    // make_guard() Tests - Guard Separator
    // ========================================================================

    TEST_CASE("make_guard() - Custom Guard Separator")
    {
        SUBCASE("Default underscore separator") {
            auto desc = make_description("myns", "Type", "", "_", true);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Should use underscore to separate parts
            CHECK(guard.find("_") != std::string::npos);
        }

        SUBCASE("Double underscore separator") {
            auto desc = make_description("my::ns", "Type", "", "__", true);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Should use double underscore
            CHECK(guard.find("__") != std::string::npos);
        }

        SUBCASE("Custom separator with dash") {
            auto desc = make_description("myns", "Type", "", "-", true);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Note: dash makes invalid C++ identifier, but we allow it
            CHECK(guard.find("-") != std::string::npos);
        }

        SUBCASE("Empty separator") {
            auto desc = make_description("myns", "Type", "", "", true);
            auto guard = GuardGenerator::make_guard(desc, "code");

            // Should concatenate without separator
            CHECK_FALSE(guard.empty());
            // Should not have underscore from default
            auto underscores = std::count(guard.begin(), guard.end(), '_');
            CHECK(underscores == 0);
        }
    }

    // ========================================================================
    // make_guard() Tests - Case Sensitivity
    // ========================================================================

    TEST_CASE("make_guard() - Upcase Guard Toggle")
    {
        std::string code = "// test code";

        SUBCASE("upcase_guard = true produces uppercase") {
            auto desc = make_description("myns", "MyType", "", "_", true);
            auto guard = GuardGenerator::make_guard(desc, code);

            CHECK(is_uppercase(guard));
            CHECK(guard.find("MYNS") != std::string::npos);
            CHECK(guard.find("MYTYPE") != std::string::npos);
        }

        SUBCASE("upcase_guard = false preserves original case") {
            // With lowercase namespace and mixed-case type
            auto desc = make_description("myns", "MyType", "", "_", false);
            auto guard = GuardGenerator::make_guard(desc, code);

            // Should preserve the original case from namespace and type
            CHECK(guard.find("myns") != std::string::npos);
            CHECK(guard.find("MyType") != std::string::npos);
            // Hash should also be lowercase
            CHECK_FALSE(is_uppercase(guard));
        }

        SUBCASE("Case transformation behavior") {
            // Use all lowercase input for the non-uppercase version
            auto desc_upper = make_description("ns", "type", "", "_", true);
            auto desc_lower = make_description("ns", "type", "", "_", false);

            auto guard_upper = GuardGenerator::make_guard(desc_upper, code);
            auto guard_lower = GuardGenerator::make_guard(desc_lower, code);

            // Guards should differ in case
            CHECK(guard_upper != guard_lower);

            // When upper is converted to lowercase, should match lower
            std::string upper_lower = guard_upper;
            std::transform(
                upper_lower.begin(),
                upper_lower.end(),
                upper_lower.begin(),
                [](unsigned char c) {
                    return static_cast<char>(std::tolower(static_cast<int>(c)));
                });
            CHECK(upper_lower == guard_lower);
        }
    }

    // ========================================================================
    // make_guard() Tests - Content Addressability (SHA1)
    // ========================================================================

    TEST_CASE("make_guard() - Content-Addressable Guards")
    {
        auto desc = make_description("test", "Type");

        SUBCASE("Different code produces different guards") {
            auto guard1 = GuardGenerator::make_guard(desc, "code version 1");
            auto guard2 = GuardGenerator::make_guard(desc, "code version 2");

            CHECK(guard1 != guard2);

            // Both should have same prefix (namespace_type)
            // but different hash suffix
            size_t last_sep1 = guard1.rfind('_');
            size_t last_sep2 = guard2.rfind('_');

            REQUIRE(last_sep1 != std::string::npos);
            REQUIRE(last_sep2 != std::string::npos);

            std::string prefix1 = guard1.substr(0, last_sep1);
            std::string prefix2 = guard2.substr(0, last_sep2);

            CHECK(prefix1 == prefix2);

            std::string hash1 = guard1.substr(last_sep1 + 1);
            std::string hash2 = guard2.substr(last_sep2 + 1);

            CHECK(hash1 != hash2);
        }

        SUBCASE("Same code produces same guard") {
            std::string code = "identical code";
            auto guard1 = GuardGenerator::make_guard(desc, code);
            auto guard2 = GuardGenerator::make_guard(desc, code);

            CHECK(guard1 == guard2);
        }

        SUBCASE("Empty code is handled") {
            auto guard = GuardGenerator::make_guard(desc, "");

            CHECK_FALSE(guard.empty());
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Large code is handled") {
            std::string large_code(10000, 'x');
            auto guard = GuardGenerator::make_guard(desc, large_code);

            CHECK_FALSE(guard.empty());
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Guard contains hash suffix") {
            auto guard = GuardGenerator::make_guard(desc, "test code");

            // Guard should end with SHA1 hash (40 hex chars)
            // In uppercase mode, should be 40 uppercase hex chars
            size_t last_sep = guard.rfind('_');
            REQUIRE(last_sep != std::string::npos);

            std::string hash = guard.substr(last_sep + 1);

            // SHA1 produces 40 hex characters
            CHECK(hash.length() == 40);

            // All characters should be hex digits (uppercase in default mode)
            bool all_hex = std::all_of(hash.begin(), hash.end(), [](char c) {
                return std::isxdigit(static_cast<unsigned char>(c));
            });
            CHECK(all_hex);
        }
    }

    // ========================================================================
    // make_guard() Tests - Complex Scenarios
    // ========================================================================

    TEST_CASE("make_guard() - Complex Real-World Scenarios")
    {
        SUBCASE("STL-style namespace") {
            auto desc = make_description("std::experimental", "Optional");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("STD") != std::string::npos);
            CHECK(guard.find("EXPERIMENTAL") != std::string::npos);
            CHECK(guard.find("OPTIONAL") != std::string::npos);
            CHECK_FALSE(contains_colons(guard));
        }

        SUBCASE("Very long namespace chain") {
            auto desc = make_description(
                "company::division::department::team::project::module",
                "SpecializedType");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Type name with underscores") {
            auto desc = make_description("myns", "my_special_type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("MY_SPECIAL_TYPE") != std::string::npos);
        }

        SUBCASE("Namespace and type with same name") {
            auto desc = make_description("util::util", "Util");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            // Should contain UTIL multiple times
            size_t first = guard.find("UTIL");
            size_t second = guard.find("UTIL", first + 1);
            CHECK(first != std::string::npos);
            CHECK(second != std::string::npos);
        }

        SUBCASE("Single character namespace and type") {
            auto desc = make_description("a", "B");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("A") != std::string::npos);
            CHECK(guard.find("B") != std::string::npos);
            CHECK(is_valid_cpp_identifier(guard));
        }
    }

    // ========================================================================
    // Integration Tests
    // ========================================================================

    TEST_CASE("GuardGenerator - Integration Scenarios")
    {
        SUBCASE("Generate complete header guard pattern") {
            auto desc = make_description("myproject::core", "UserId");
            std::string code = "class UserId { int value; };";

            auto guard = GuardGenerator::make_guard(desc, code);
            auto banner = GuardGenerator::make_notice_banner();

            // Simulate typical header file structure
            std::string header = "#ifndef " + guard + "\n";
            header += "#define " + guard + "\n";
            header += banner;
            header += "\n";
            header += code;
            header += "\n#endif // " + guard + "\n";

            // Verify structure
            CHECK(header.find("#ifndef") == 0);
            CHECK(header.find("#define") != std::string::npos);
            CHECK(header.find("#endif") != std::string::npos);
            CHECK(header.find("DO NOT EDIT") != std::string::npos);
        }

        SUBCASE("Multiple types in same namespace have different guards") {
            auto desc1 = make_description("myns", "Type1");
            auto desc2 = make_description("myns", "Type2");
            std::string code = "// shared code";

            auto guard1 = GuardGenerator::make_guard(desc1, code);
            auto guard2 = GuardGenerator::make_guard(desc2, code);

            // Different type names should produce different guards
            // even with same code (because namespace_type part differs)
            CHECK(guard1 != guard2);
        }

        SUBCASE("Same type in different namespaces have different guards") {
            auto desc1 = make_description("ns1", "Type");
            auto desc2 = make_description("ns2", "Type");
            std::string code = "// shared code";

            auto guard1 = GuardGenerator::make_guard(desc1, code);
            auto guard2 = GuardGenerator::make_guard(desc2, code);

            // Different namespaces should produce different guards
            CHECK(guard1 != guard2);
        }

        SUBCASE("Banner is identical for all types") {
            // Banner should be consistent across all generated files
            auto banner1 = GuardGenerator::make_notice_banner();
            auto banner2 = GuardGenerator::make_notice_banner();
            auto banner3 = GuardGenerator::make_notice_banner();

            CHECK(banner1 == banner2);
            CHECK(banner2 == banner3);
        }
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("make_guard() - Edge Cases")
    {
        SUBCASE("Type name with numbers") {
            auto desc = make_description("myns", "Type123");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("TYPE123") != std::string::npos);
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Namespace with numbers") {
            auto desc = make_description("ns2024", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(guard.find("NS2024") != std::string::npos);
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("All underscores in namespace") {
            auto desc = make_description("___", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Guard with only colons in namespace") {
            auto desc = make_description("::::::", "Type");
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(contains_colons(guard));
            CHECK(is_valid_cpp_identifier(guard));
        }

        SUBCASE("Very long type name") {
            std::string long_name(1000, 'A');
            auto desc = make_description("ns", long_name);
            auto guard = GuardGenerator::make_guard(desc, "code");

            CHECK_FALSE(guard.empty());
            CHECK_FALSE(contains_colons(guard));
        }
    }
}

} // anonymous namespace
