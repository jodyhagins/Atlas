// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"
#include "CodeStructureParser.hpp"
#include "ProfileSystem.hpp"
#include "StrongTypeGenerator.hpp"
#include "version.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"
#include "rapidcheck.hpp"

using namespace wjh::atlas::testing;
using wjh::atlas::testing::rc::check;

namespace {

using namespace wjh::atlas;

auto
make_description(
    std::string kind = "struct",
    std::string type_namespace = "test",
    std::string type_name = "TestType",
    std::string description = "strong int",
    std::string default_value = "",
    std::string guard_prefix = "",
    std::string guard_separator = "_",
    bool upcase_guard = true)
{
    return StrongTypeDescription{
        .kind = std::move(kind),
        .type_namespace = std::move(type_namespace),
        .type_name = std::move(type_name),
        .description = std::move(description),
        .default_value = std::move(default_value),
        .guard_prefix = std::move(guard_prefix),
        .guard_separator = std::move(guard_separator),
        .upcase_guard = upcase_guard};
}

// Helper function for simple code generation without warnings
std::string
generate_strong_type(StrongTypeDescription const & desc)
{
    StrongTypeGenerator gen;
    return gen(desc);
}

struct SplitCode
{
    std::string full_code;
    std::string preamble; // Everything before the marker
    std::string type_specific; // Everything after the marker
};

SplitCode
split_generated_code(std::string const & code)
{
    constexpr auto marker = "/// These are the droids you are looking for!";
    auto pos = code.find(marker);

    SplitCode result;
    result.full_code = code;
    if (pos != std::string::npos) {
        result.preamble = code.substr(0, pos);
        result.type_specific = code.substr(pos + std::strlen(marker));
    } else {
        // Fallback if marker not found
        result.type_specific = code;
    }
    return result;
}

TEST_SUITE("StrongTypeGenerator")
{
    TEST_CASE("Default Initialization Code Generation")
    {
        CodeStructureParser parser;

        SUBCASE("explicit non-zero default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithDefault",
                "strong int; ==",
                "42"));

            // Verify the generated code contains the correct initialization
            CHECK(code.find("int value{42};") != std::string::npos);
        }

        SUBCASE("explicit zero default value") {
            auto code = generate_strong_type(make_description(
                "struct",
                "test",
                "WithZero",
                "strong int; ==",
                "0"));

            // Verify zero is explicitly initialized
            CHECK(code.find("int value{0};") != std::string::npos);
        }
    }

    TEST_CASE("Error Handling")
    {
        CodeStructureParser parser;

        SUBCASE("invalid kind throws exception") {
            auto desc =
                make_description("invalid", "test", "Bad", "strong int");

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("empty description still generates basic structure") {
            auto code = generate_strong_type(
                make_description("struct", "test", "Empty", "strong int; "));
            auto structure = parser.parse(code);

            CHECK(structure.kind == "struct");
            CHECK(structure.type_name == "Empty");
            CHECK(structure.member_type == "int");
            CHECK(structure.member_name == "value");
        }
    }

    TEST_CASE("Template Assignment Operator")
    {
        SUBCASE("assign keyword generates template assignment") {
            auto desc = make_description(
                "struct",
                "test",
                "AssignableString",
                "strong std::string; ==, assign");

            auto code = generate_strong_type(desc);

            // Verify template assignment is generated
            CHECK(code.find("template <typename T>") != std::string::npos);
            CHECK(code.find("operator=(T&& t)") != std::string::npos);

            // Verify C++20 version with concepts
            CHECK(
                code.find("requires (std::assignable_from") !=
                std::string::npos);
            CHECK(
                code.find("not std::same_as<std::decay_t<T>") !=
                std::string::npos);

            // Verify C++11 fallback
            CHECK(code.find("#else") != std::string::npos);
            CHECK(code.find("std::enable_if") != std::string::npos);
            CHECK(code.find("std::is_assignable<") != std::string::npos);

            // Verify perfect forwarding
            CHECK(code.find("std::forward<T>(t)") != std::string::npos);

            // Verify noexcept specification
            CHECK(code.find("noexcept(noexcept(") != std::string::npos);
        }

        SUBCASE("no-constexpr with assign") {
            auto desc = make_description(
                "struct",
                "test",
                "NonConstexprAssign",
                "strong std::string; ==, assign, no-constexpr");

            auto code = generate_strong_type(desc);

            // Should still have template assignment
            CHECK(code.find("operator=(T&& t)") != std::string::npos);
        }
    }

    TEST_CASE("Cast Operators")
    {
        SUBCASE("invalid cast syntax throws - missing closing bracket") {
            auto desc = make_description(
                "struct",
                "test",
                "Bad",
                "strong int; cast<bool");

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("invalid implicit_cast syntax throws") {
            auto desc = make_description(
                "struct",
                "test",
                "Bad",
                "strong int; implicit_cast<bool");

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }
    }

    TEST_CASE("Multi-Type File Generation")
    {
        SUBCASE("preamble appears exactly once") {
            // Create multiple type descriptions
            std::vector<StrongTypeDescription> descriptions = {
                make_description("struct", "test", "Type1", "strong int; +, -"),
                make_description(
                    "struct",
                    "test",
                    "Type2",
                    "strong double; *, /"),
                make_description(
                    "struct",
                    "test",
                    "Type3",
                    "strong std::string; ==, !=")};

            auto code =
                generate_strong_types_file(descriptions, "EXAMPLE", "_", true);

            // The preamble guard should appear exactly 4 times: #ifndef,
            // #define, #endif, and once in documentation comment
            std::string preamble_guard =
                "WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90";

            size_t count = 0;
            size_t pos = 0;
            while ((pos = code.find(preamble_guard, pos)) != std::string::npos)
            {
                ++count;
                pos += preamble_guard.length();
            }

            // Should appear exactly 4 times: #ifndef, #define at start,
            // #endif at end, and once in the boilerplate documentation comment
            CHECK(count == 4);

            // Verify the preamble marker appears exactly once
            std::string preamble_marker =
                "These are the droids you are looking for!";
            count = 0;
            pos = 0;
            while ((pos = code.find(preamble_marker, pos)) != std::string::npos)
            {
                ++count;
                pos += preamble_marker.length();
            }
            CHECK(count == 1);

            // Verify strong_type_tag is defined exactly once
            std::string strong_type_tag = "struct strong_type_tag";
            count = 0;
            pos = 0;
            while ((pos = code.find(strong_type_tag, pos)) != std::string::npos)
            {
                ++count;
                pos += strong_type_tag.length();
            }
            CHECK(count == 1);
        }

        SUBCASE("all types are present in generated file") {
            std::vector<StrongTypeDescription> descriptions = {
                make_description("struct", "ns1", "TypeA", "strong int"),
                make_description("struct", "ns2", "TypeB", "strong double"),
                make_description("struct", "ns3", "TypeC", "strong float")};

            auto code = generate_strong_types_file(descriptions, "", "_", true);

            // Verify all three types are present
            CHECK(code.find("struct TypeA") != std::string::npos);
            CHECK(code.find("struct TypeB") != std::string::npos);
            CHECK(code.find("struct TypeC") != std::string::npos);

            // Verify all three namespaces are present
            CHECK(code.find("namespace ns1") != std::string::npos);
            CHECK(code.find("namespace ns2") != std::string::npos);
            CHECK(code.find("namespace ns3") != std::string::npos);
        }
    }

    TEST_CASE("C++11 Compatibility")
    {
        CodeStructureParser parser;

        SUBCASE("type traits use C++11 syntax") {
            auto desc = make_description(
                "struct",
                "test",
                "TypeTraitsTest",
                "strong int");
            auto code = generate_strong_type(desc);

            // Should use std::enable_if<...>::type not std::enable_if_t<...>
            CHECK(code.find("std::enable_if_t") == std::string::npos);
            CHECK(code.find("typename std::enable_if<") != std::string::npos);
            CHECK(code.find(">::type") != std::string::npos);

            // Should use std::is_constructible<...>::value not
            // std::is_constructible_v<...>
            CHECK(code.find("std::is_constructible_v") == std::string::npos);
            CHECK(code.find("std::is_constructible<") != std::string::npos);
            CHECK(code.find(">::value") != std::string::npos);
        }

        SUBCASE("subscript operator uses C++11 trailing return type") {
            auto desc = make_description(
                "struct",
                "test",
                "SubscriptTest",
                "strong std::vector<int>; []");
            auto code = generate_strong_type(desc);

            // Should NOT use decltype(auto) which is C++14
            // Look in the #else branch (non-C++23) for the single-argument
            // version
            auto subscript_if = code.find("__cpp_multidimensional_subscript");
            REQUIRE(subscript_if != std::string::npos);
            auto else_pos = code.find("#else", subscript_if);
            REQUIRE(else_pos != std::string::npos);
            auto endif_pos = code.find("#endif", else_pos);
            REQUIRE(endif_pos != std::string::npos);
            std::string cpp11_section = code.substr(
                else_pos,
                endif_pos - else_pos);

            // Should not have decltype(auto) in C++11 section
            CHECK(cpp11_section.find("decltype(auto)") == std::string::npos);

            // Should have trailing return type: auto ... -> decltype(...)
            CHECK(cpp11_section.find("auto operator []") != std::string::npos);
            CHECK(
                cpp11_section.find("-> decltype(value[") != std::string::npos);
        }
    }

    TEST_CASE("Version Information")
    {
        SUBCASE("version constants are defined") {
            CHECK(wjh::atlas::codegen::version_major >= 0);
            CHECK(wjh::atlas::codegen::version_minor >= 0);
            CHECK(wjh::atlas::codegen::version_patch >= 0);
        }

        SUBCASE("version string format is correct") {
            std::string version = wjh::atlas::codegen::version_string;

            // Should be in format "MAJOR.MINOR.PATCH"
            auto first_dot = version.find('.');
            auto last_dot = version.rfind('.');

            CHECK(first_dot != std::string::npos);
            CHECK(last_dot != std::string::npos);
            CHECK(first_dot != last_dot);
        }

        SUBCASE("generated code includes version") {
            auto desc =
                make_description("struct", "test", "TestType", "strong int");
            auto code = generate_strong_type(desc);

            // Should contain version information in header comment
            CHECK(
                code.find("Atlas Strong Type Generator v") !=
                std::string::npos);
            CHECK(
                code.find(wjh::atlas::codegen::version_string) !=
                std::string::npos);
        }

        SUBCASE("generated code includes DO NOT EDIT warning") {
            auto desc =
                make_description("struct", "test", "TestType", "strong int");
            auto code = generate_strong_type(desc);

            CHECK(code.find("DO NOT EDIT") != std::string::npos);
        }

        SUBCASE("multi-file generation includes version") {
            std::vector<StrongTypeDescription> descriptions = {
                make_description("struct", "test", "Type1", "strong int"),
                make_description("struct", "test", "Type2", "strong double")};
            auto code = generate_strong_types_file(descriptions);

            // Should contain version in the banner
            CHECK(
                code.find("Atlas Strong Type Generator v") !=
                std::string::npos);
            CHECK(
                code.find(wjh::atlas::codegen::version_string) !=
                std::string::npos);
        }
    }

    TEST_CASE("Warning System")
    {
        SUBCASE("no warnings for spaceship alone") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            CHECK(warnings.empty());
        }

        SUBCASE("warning for spaceship with equality operators") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, !=, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE(warnings.size() >= 1);
            bool found_equality_warning = false;
            for (auto const & w : warnings) {
                if (w.message.find("'==' and '!='") != std::string::npos &&
                    w.message.find("redundant") != std::string::npos)
                {
                    found_equality_warning = true;
                    CHECK(w.type_name == "test::TestType");
                }
            }
            CHECK(found_equality_warning);
        }

        SUBCASE("warning for spaceship with relational operators") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; <, <=, >, >=, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE(warnings.size() >= 1);
            bool found_relational_warning = false;
            for (auto const & w : warnings) {
                if (w.message.find("'<', '<=', '>', '>='") !=
                        std::string::npos &&
                    w.message.find("redundant") != std::string::npos)
                {
                    found_relational_warning = true;
                    CHECK(w.type_name == "test::TestType");
                }
            }
            CHECK(found_relational_warning);
        }

        SUBCASE("both warnings for spaceship with all comparison operators") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, !=, <, <=, >, >=, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE(warnings.size() == 2);
        }

        SUBCASE("no warning when only == != specified without spaceship") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, !=");
            gen(desc);
            auto warnings = gen.get_warnings();

            CHECK(warnings.empty());
        }

        SUBCASE("no warning when only relational operators without spaceship") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; <, <=, >, >=");
            gen(desc);
            auto warnings = gen.get_warnings();

            CHECK(warnings.empty());
        }

        SUBCASE("warning includes correct type name with namespace") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "my::nested::namespace",
                "MyType",
                "strong double; ==, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE_FALSE(warnings.empty());
            CHECK(warnings[0].type_name == "my::nested::namespace::MyType");
        }

        SUBCASE("warning includes correct type name without namespace") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "",
                "GlobalType",
                "strong int; <, <=>");
            gen(desc);
            auto warnings = gen.get_warnings();

            REQUIRE_FALSE(warnings.empty());
            CHECK(warnings[0].type_name == "GlobalType");
        }

        SUBCASE("clear_warnings() clears collected warnings") {
            StrongTypeGenerator gen;
            auto desc = make_description(
                "struct",
                "test",
                "TestType",
                "strong int; ==, <=>");
            gen(desc);
            CHECK_FALSE(gen.get_warnings().empty());

            gen.clear_warnings();
            CHECK(gen.get_warnings().empty());
        }

        SUBCASE("warnings accumulate across multiple generations") {
            StrongTypeGenerator gen;

            auto desc1 = make_description(
                "struct",
                "test",
                "Type1",
                "strong int; ==, <=>");
            gen(desc1);
            CHECK(gen.get_warnings().size() == 1);

            auto desc2 = make_description(
                "struct",
                "test",
                "Type2",
                "strong int; <, <=>");
            gen(desc2);
            CHECK(gen.get_warnings().size() == 2);
        }
    }

    TEST_CASE("C++ Standard Specification")
    {
        SUBCASE("parse_cpp_standard - valid inputs") {
            CHECK(parse_cpp_standard("11") == 11);
            CHECK(parse_cpp_standard("14") == 14);
            CHECK(parse_cpp_standard("17") == 17);
            CHECK(parse_cpp_standard("20") == 20);
            CHECK(parse_cpp_standard("23") == 23);
            CHECK(parse_cpp_standard("c++11") == 11);
            CHECK(parse_cpp_standard("c++20") == 20);
            CHECK(parse_cpp_standard("C++17") == 17);
            CHECK(parse_cpp_standard("C++23") == 23);
        }

        SUBCASE("parse_cpp_standard - invalid inputs") {
            CHECK_THROWS_AS(parse_cpp_standard("18"), std::invalid_argument);
            CHECK_THROWS_AS(parse_cpp_standard("21"), std::invalid_argument);
            CHECK_THROWS_AS(parse_cpp_standard("26"), std::invalid_argument);
            CHECK_THROWS_AS(parse_cpp_standard("foo"), std::invalid_argument);
            CHECK_THROWS_AS(parse_cpp_standard(""), std::invalid_argument);
            CHECK_THROWS_AS(parse_cpp_standard("2a"), std::invalid_argument);
            CHECK_THROWS_AS(parse_cpp_standard("2b"), std::invalid_argument);
        }

        SUBCASE("generate_cpp_standard_assertion - C++11 no assertion") {
            auto result = generate_cpp_standard_assertion(11);
            CHECK(result == "");
        }

        SUBCASE("generate_cpp_standard_assertion - C++14") {
            auto result = generate_cpp_standard_assertion(14);
            CHECK(result.find("static_assert") != std::string::npos);
            CHECK(result.find("201402L") != std::string::npos);
            CHECK(result.find("C++14") != std::string::npos);
            CHECK(result.find("-std=c++14") != std::string::npos);
        }

        SUBCASE("generate_cpp_standard_assertion - C++17") {
            auto result = generate_cpp_standard_assertion(17);
            CHECK(result.find("201703L") != std::string::npos);
            CHECK(result.find("C++17") != std::string::npos);
        }

        SUBCASE("generate_cpp_standard_assertion - C++20") {
            auto result = generate_cpp_standard_assertion(20);
            CHECK(result.find("202002L") != std::string::npos);
            CHECK(result.find("C++20") != std::string::npos);
        }

        SUBCASE("generate_cpp_standard_assertion - C++23") {
            auto result = generate_cpp_standard_assertion(23);
            CHECK(result.find("202302L") != std::string::npos);
            CHECK(result.find("C++23") != std::string::npos);
        }

        SUBCASE("description-level cpp_standard parsing") {
            StrongTypeGenerator gen;
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "MyType",
                .description = "strong int; +, -, c++20"};

            auto result = gen(desc);
            CHECK(
                result.find("static_assert(__cplusplus >= 202002L") !=
                std::string::npos);
            CHECK(result.find("C++20") != std::string::npos);
        }

        SUBCASE("file generates cpp_standard at top") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "MyType",
                .description = "strong int; <=>",
                .cpp_standard = 20};

            StrongTypeGenerator gen;
            auto result = gen(desc);

            // Find header guard
            auto ifndef_pos = result.find("#ifndef");
            auto define_pos = result.find("#define", ifndef_pos);
            auto assert_pos = result.find("static_assert", define_pos);

            CHECK(ifndef_pos != std::string::npos);
            CHECK(define_pos != std::string::npos);
            CHECK(assert_pos != std::string::npos);

            // static_assert should come before NOTICE banner
            auto notice_pos = result.find("NOTICE");
            CHECK(assert_pos < notice_pos);
        }

        SUBCASE("multi-type file uses max cpp_standard") {
            std::vector<StrongTypeDescription> types = {
                {.kind = "struct",
                 .type_namespace = "test",
                 .type_name = "Type1",
                 .description = "strong int; +, -",
                 .cpp_standard = 14},
                {.kind = "struct",
                 .type_namespace = "test",
                 .type_name = "Type2",
                 .description = "strong int; ==, !=",
                 .cpp_standard = 20},
                {.kind = "struct",
                 .type_namespace = "test",
                 .type_name = "Type3",
                 .description = "strong int; *",
                 .cpp_standard = 17}};

            auto result = generate_strong_types_file(types);

            // Should use C++20 (max of 14, 20, 17)
            CHECK(result.find("202002L") != std::string::npos);
            // Should NOT have C++14 or C++17 assertions
            CHECK(result.find("201402L") == std::string::npos);
            CHECK(result.find("201703L") == std::string::npos);
        }
    }
}

TEST_CASE("ProfileSystem basic functionality")
{
    ProfileSystem ps;
    ps.clear(); // Start fresh

    SUBCASE("register and query profiles") {
        auto numeric_spec = parse_specification("NUMERIC; +, -, *, /, ==, !=");
        ps.register_profile("NUMERIC", numeric_spec);

        CHECK(ps.has_profile("NUMERIC"));
        CHECK_FALSE(ps.has_profile("NONEXISTENT"));

        auto names = ps.get_profile_names();
        CHECK(names.size() == 1);
        CHECK(names[0] == "NUMERIC");
    }

    SUBCASE("profile name validation") {
        // Valid names
        CHECK_NOTHROW(ps.register_profile(
            "valid_name",
            parse_specification("valid_name; +")));
        CHECK_NOTHROW(ps.register_profile(
            "Valid123",
            parse_specification("Valid123; +")));
        CHECK_NOTHROW(ps.register_profile(
            "name-with-dash",
            parse_specification("name-with-dash; +")));

        // Invalid names
        CHECK_THROWS(ps.register_profile("", parse_specification("; +")));
        CHECK_THROWS(ps.register_profile(
            "name with space",
            parse_specification("name with space; +")));
        CHECK_THROWS(ps.register_profile(
            "name$symbol",
            parse_specification("name$symbol; +")));
    }

    SUBCASE("duplicate profile registration") {
        ps.register_profile("TEST", parse_specification("TEST; +, -"));
        CHECK_THROWS(
            ps.register_profile("TEST", parse_specification("TEST; *, /")));
    }

    SUBCASE("get profile spec") {
        ps.register_profile("MATH", parse_specification("MATH; +, -, *, /"));

        // Profile expansion is now done in AtlasCommandLine, not in
        // ProfileSystem Just verify the profile can be retrieved
        auto const & math_profile = ps.get_profile("MATH");
        CHECK(math_profile.operators.size() == 4);
        CHECK(math_profile.operators.count("+") == 1);
        CHECK(math_profile.operators.count("-") == 1);
        CHECK(math_profile.operators.count("*") == 1);
        CHECK(math_profile.operators.count("/") == 1);
    }

    SUBCASE("profile with forward=") {
        ps.register_profile(
            "STRING_LIKE",
            parse_specification("STRING_LIKE; forward=size,empty; ==, !="));

        auto const & profile = ps.get_profile("STRING_LIKE");
        CHECK(profile.forwards.size() == 2);
        CHECK(profile.forwards[0] == "size");
        CHECK(profile.forwards[1] == "empty");
        CHECK(profile.operators.size() == 2);
        CHECK(profile.operators.count("==") == 1);
        CHECK(profile.operators.count("!=") == 1);
    }

    SUBCASE("unknown profile throws") {
        CHECK_THROWS(ps.get_profile("UNKNOWN"));
    }

    // TODO: Add integration tests for profile expansion in descriptions
    // These tests should be in atlas_command_line_ut.cpp or a similar
    // integration test file since profile expansion now happens during
    // description parsing in AtlasCommandLine
}

// Note: expand_features() was removed from ProfileSystem
// Profile expansion is now done in AtlasCommandLine.cpp during description
// parsing Integration tests should be added to verify profile expansion works
// correctly with forward=

} // anonymous namespace
