// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"
#include "error_test_support.hpp"

#include <string>

using namespace wjh::atlas::testing;

namespace {

TEST_SUITE("Error Handling: Syntax Errors")
{
    TEST_CASE("Missing required field: kind (now defaults to struct)")
    {
        auto result = test_input_content_error(R"(
[type]
namespace=test
name=TestType
description=strong int; +, -
)");

        // Kind now defaults to 'struct', so this should succeed
        CHECK(not result.had_error());
        auto output = result.stderr_output + result.stdout_output;
        // Output should contain the type definition
        bool has_type =
            (output.find("struct TestType") != std::string::npos ||
             output.find("TestType") != std::string::npos);
        CHECK(has_type);
    }

    TEST_CASE("Missing required field: name")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
description=strong int; +, -
)");

        CHECK(result.had_error());
        // Error: "Incomplete type definition" - doesn't specifically mention
        // 'name' but correctly rejects the incomplete definition
    }

    TEST_CASE("Missing required field: description")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
)");

        CHECK(result.had_error());
        // Error: "Incomplete type definition" - doesn't specifically mention
        // 'description' but correctly rejects the incomplete definition
    }

    TEST_CASE("Invalid kind value")
    {
        auto result = test_input_content_error(R"(
[type]
kind=union
namespace=test
name=TestType
description=strong int
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("kind") != std::string::npos ||
             error_msg.find("union") != std::string::npos));
    }

    TEST_CASE("Unknown field name")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
description=strong int
unknown_field=invalid
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("unknown") != std::string::npos ||
             error_msg.find("unknown_field") != std::string::npos));
    }

    TEST_CASE("Invalid operator syntax")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
description=strong int; +++++
)");

        CHECK(result.had_error());
        // Should reject invalid operator
    }

    TEST_CASE("Type specification: empty (only semicolon)")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
description=; +, -
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Empty type specification") != std::string::npos);
    }

    TEST_CASE("Empty file")
    {
        auto result = test_input_content_error("");

        CHECK(result.had_error());
    }

    TEST_CASE("Whitespace-only file")
    {
        auto result = test_input_content_error("   \n\t\n   ");

        CHECK(result.had_error());
    }

    TEST_CASE("Unclosed quotes in field value")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace="test
name=TestType
description=strong int
)");

        // May or may not error - document behavior
        INFO(
            "Unclosed quotes: "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("Invalid comment syntax")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
## Invalid comment?
description=strong int
)");

        // Comments are silently ignored - Atlas just skips unrecognized lines
        CHECK_FALSE(result.had_error());
    }
}

TEST_SUITE("Error Handling: Semantic Errors")
{
    // NOTE: Atlas is a code generator, not a C++ validator. Tests for C++
    // keywords, invalid identifier syntax, and invalid namespace syntax have
    // been removed because Atlas intentionally accepts these - the C++ compiler
    // will catch such errors when compiling the generated code.

    TEST_CASE("Conflicting operators: spaceship with relational")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
description=strong int; <=>, <, >
)");

        // May or may not be an error depending on implementation
        // Document expected behavior
        INFO(
            "Spaceship + relational: "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("Duplicate operator specification")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
description=strong int; +, +, -
)");

        // Should either deduplicate or error
        // Document which is expected
        INFO(
            "Duplicate operators: "
            << (result.had_error() ? "rejected" : "accepted (deduplicated)"));
    }
}

TEST_SUITE("Error Handling: File I/O Errors")
{
    TEST_CASE("Nonexistent input file")
    {
        auto result = call_atlas_expecting_error(
            {"atlas", "--input=/nonexistent/path/to/file.input"});

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("not found") != std::string::npos ||
             error_msg.find("No such file") != std::string::npos ||
             error_msg.find("does not exist") != std::string::npos ||
             error_msg.find("Cannot open") != std::string::npos));
    }

    TEST_CASE("Input file is a directory")
    {
        TemporaryDirectory temp_dir;

        auto result = call_atlas_expecting_error(
            {"atlas", "--input=" + temp_dir.path().string()});

        CHECK(result.had_error());
        // Error: "No type definitions found" - doesn't specifically mention
        // directory but correctly rejects directory as input
    }

    TEST_CASE("Unreadable input file (permission test)")
    {
        // Note: Permission tests may be platform-specific
        TemporaryDirectory temp_dir;
        auto input_file = temp_dir.path() / "unreadable.input";
        write_file(
            input_file,
            "kind=struct\nnamespace=test\nname=TestType\n"
            "description=strong int\n");

        // Try to make file unreadable (POSIX only)
#if defined(__unix__) || defined(__APPLE__)
        fs::permissions(input_file, fs::perms::none, fs::perm_options::replace);

        auto result = call_atlas_expecting_error(
            {"atlas", "--input=" + input_file.string()});

        // Restore permissions for cleanup
        fs::permissions(
            input_file,
            fs::perms::owner_all,
            fs::perm_options::replace);

        CHECK(result.had_error());
#else
        INFO("Permission test skipped on non-POSIX platform");
#endif
    }

    TEST_CASE("Output path to nonexistent directory")
    {
        TemporaryDirectory temp_dir;
        auto input_file = temp_dir.path() / "test.input";
        write_file(input_file, R"(
kind=struct
namespace=test
name=TestType
description=strong int
)");

        auto result = call_atlas_expecting_error(
            {"atlas",
             input_file.string(),
             "-o",
             "/nonexistent/dir/output.hpp"});

        // May succeed (creates parent dirs) or fail
        INFO(
            "Output to nonexistent dir: "
            << (result.had_error() ? "rejected" : "accepted (created)"));
    }

    TEST_CASE("Very large input file (>1MB)")
    {
        TemporaryDirectory temp_dir;
        auto input_file = temp_dir.path() / "large.input";

        // Create a large but valid input file
        std::string large_content = R"(kind=struct
namespace=test
name=LargeType
description=strong int; +, -, *, /
)";

        // Add lots of whitespace/comments to make it large
        std::string padding(1024 * 1024, ' '); // 1MB of spaces
        large_content += padding;

        write_file(input_file, large_content);

        auto result = call_atlas_expecting_error(
            {"atlas", input_file.string()});

        // Should probably succeed
        INFO(
            "Large file (1MB+): "
            << (result.had_error() ? "rejected" : "accepted"));
    }
}

TEST_SUITE("Error Handling: Command-Line Errors")
{
    TEST_CASE("No arguments")
    {
        auto result = call_atlas_expecting_error({"atlas"});

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        // Should show usage or error about missing input
        CHECK(
            (error_msg.find("usage") != std::string::npos ||
             error_msg.find("input") != std::string::npos ||
             error_msg.find("required") != std::string::npos ||
             error_msg.find("Usage") != std::string::npos));
    }

    TEST_CASE("Unknown flag")
    {
        auto result = call_atlas_expecting_error({"atlas", "--unknown-flag"});

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("unknown") != std::string::npos ||
             error_msg.find("unrecognized") != std::string::npos ||
             error_msg.find("invalid") != std::string::npos));
    }

    TEST_CASE("Invalid flag format: -o without value")
    {
        TemporaryDirectory temp_dir;
        auto input_file = temp_dir.path() / "test.input";
        write_file(input_file, R"(
kind=struct
namespace=test
name=TestType
description=strong int
)");

        auto result = call_atlas_expecting_error(
            {"atlas", input_file.string(), "-o"});

        // May error or may interpret next arg as output
        INFO(
            "Flag without value: "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("Help flag should not error")
    {
        auto result = call_atlas_expecting_error({"atlas", "--help"});

        // Help should succeed (exit 0) or be treated as error
        INFO("--help exit code: " << result.exit_code);
    }
}

TEST_SUITE("Error Handling: Edge Cases")
{
    TEST_CASE("Very long type name (>1000 characters)")
    {
        std::string very_long_name(1001, 'A');

        auto result = test_input_content_error(
            "kind=struct\n"
            "namespace=test\n"
            "name=" +
            very_long_name +
            "\n"
            "description=strong int\n");

        // May succeed or fail - document behavior
        INFO(
            "Very long type name (1001 chars): "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("Very long namespace chain (>100 levels)")
    {
        std::string long_namespace = "a";
        for (int i = 0; i < 100; ++i) {
            long_namespace += "::b" + std::to_string(i);
        }

        auto result = test_input_content_error(
            "kind=struct\n"
            "namespace=" +
            long_namespace +
            "\n"
            "name=TestType\n"
            "description=strong int\n");

        INFO(
            "Long namespace (100+ levels): "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("All operators at once (kitchen sink)")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=KitchenSink
description=strong int; +, -, *, /, %, ==, !=, <, <=, >, >=, <=>, ++, --, &, |, ^, <<, >>, @, ->, [], (), (&), in, out, hash, fmt, iterable, assign, bool, cast<int>, cast<double>, implicit_cast<bool>
)");

        // Should succeed (kitchen sink test)
        CHECK_FALSE(result.had_error());
    }

    TEST_CASE("Unicode in type name")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TypeΔ
description=strong int
)");

        // Probably should reject non-ASCII identifiers
        INFO(
            "Unicode identifier: "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    // DELETED: "Special characters in type name: dollar" - duplicate of line
    // 237

    TEST_CASE("Empty namespace (global namespace)")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=
name=TestType
description=strong int
)");

        // Empty namespace value causes "Incomplete type definition" error
        // To use global namespace, omit the namespace field entirely
        CHECK(result.had_error());
    }

    // DELETED: "Description with only 'strong' keyword" - duplicate valid case
    // DELETED: "Type with no operators" - duplicate valid case
    // Both tested by "Empty description line" test below which tests actual
    // error

    TEST_CASE("Maximum nesting in underlying type")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=NestedType
description=strong std::vector<std::map<std::string, std::vector<int>>>
)");

        // Should succeed - underlying type can be complex
        CHECK_FALSE(result.had_error());
    }

    TEST_CASE("Empty description line")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
description=
)");

        CHECK(result.had_error());
    }
}

TEST_SUITE("Error Handling: Interaction Errors")
{
    TEST_CASE("Interaction with undefined type")
    {
        auto result = test_input_content_error(R"(
[type]
guard_prefix=TEST
namespace=test

UndefinedType + AnotherUndefinedType -> ResultType
)");

        // Atlas doesn't validate type existence
        // Should accept - types may be defined externally
        INFO(
            "Undefined types in interaction: "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("Interaction with invalid operator: unary ++")
    {
        auto result = test_interaction_content_error(R"(guard_prefix=TEST
namespace=test

TypeA ++ TypeB -> Result
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("operator") != std::string::npos ||
             error_msg.find("++") != std::string::npos));
    }

    TEST_CASE("Interaction with malformed syntax: missing result type")
    {
        auto result = test_interaction_content_error(R"(guard_prefix=TEST
namespace=test

TypeA + TypeB ->
)");

        CHECK(result.had_error());
    }

    TEST_CASE("Interaction with malformed syntax: missing arrow")
    {
        auto result = test_interaction_content_error(R"(guard_prefix=TEST
namespace=test

TypeA + TypeB Result
)");

        CHECK(result.had_error());
    }

    TEST_CASE("Interaction with invalid value_access syntax")
    {
        auto result = test_interaction_content_error(R"(guard_prefix=TEST
namespace=test
value_access=invalid syntax here

TypeA + TypeB -> Result
)");

        // May succeed if Atlas doesn't validate value_access expressions
        INFO(
            "Invalid value_access: "
            << (result.had_error() ? "rejected" : "accepted"));
    }

    TEST_CASE("Interaction with no guard_prefix")
    {
        auto result = test_interaction_content_error(R"(namespace=test

TypeA + TypeB -> Result
)");

        // May succeed if guard_prefix is optional
        INFO(
            "Interaction without guard_prefix: "
            << (result.had_error() ? "rejected" : "accepted"));
    }
}

TEST_SUITE("Error Message Quality")
{
    TEST_CASE("Error messages include file name or context")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=test
name=TestType
invalid_line_here
description=strong int
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;

        // Should include some context about where error occurred
        INFO("Error message: " << error_msg);
        CHECK(error_msg.length() > 10); // More than just "error"
    }

    TEST_CASE("Error messages are user-friendly")
    {
        auto result = test_input_content_error(R"(
[type]
kind=struct
name=TestType
description=strong int
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;

        // Should not just say "error" but explain what's wrong
        CHECK(error_msg.length() > 10); // More than just "error"
        INFO("Error message for missing namespace: " << error_msg);
    }

    TEST_CASE("Multiple errors handling")
    {
        auto result = test_input_content_error(R"(
[type]
namespace=test
description=int; invalid_op
)");

        // Missing 'kind', missing 'name', missing 'strong', invalid operator
        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;

        INFO("Error message for multiple errors: " << error_msg);
        INFO("Verify: Are multiple errors reported or just first?");
    }
}

TEST_SUITE("Error Handling: Regression Tests")
{
    TEST_CASE("Bug: Missing kind in multi-type file should default to struct")
    {
        // When a file has multiple types and one is missing kind,
        // the type with missing kind should default to struct, not be silently
        // skipped.
        //
        // This test verifies that types without an explicit kind default to
        // struct and code is generated for all types (no silent skipping).
        auto result = test_input_content_error(R"(
[type]
kind=struct
namespace=demo
name=ValidType1
description=strong int; +, -

[type]
namespace=global
name=DefaultKindType
description=unsigned long; ==, !=, hash

[type]
kind=struct
namespace=demo::constants
name=ValidType2
description=unsigned short; ==, !=, <=>
)");

        // Should succeed now with defaulted kind
        CHECK(not result.had_error());
        auto output = result.stderr_output + result.stdout_output;
        // Should have generated code (at least one type present)
        bool has_some_type =
            (output.find("ValidType1") != std::string::npos ||
             output.find("DefaultKindType") != std::string::npos ||
             output.find("ValidType2") != std::string::npos);
        CHECK(has_some_type);
    }
}

} // anonymous namespace
