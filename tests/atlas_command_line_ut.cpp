// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "AtlasCommandLine.hpp"
#include "doctest.hpp"

#include <filesystem>
#include <fstream>

using namespace wjh::atlas;

namespace {

TEST_SUITE("AtlasCommandLine")
{
    TEST_CASE("Basic Argument Parsing")
    {
        SUBCASE("valid minimal arguments") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=MyType",
                "--description=strong int"};

            auto result = AtlasCommandLine::parse(args);

            CHECK(result.kind == "struct");
            CHECK(result.type_namespace == "test");
            CHECK(result.type_name == "MyType");
            CHECK(result.description == "strong int");
            CHECK(result.guard_separator == "_"); // default
            CHECK(result.upcase_guard == true); // default
            CHECK(result.help == false);
        }

        SUBCASE("all arguments specified") {
            std::vector<std::string> args{
                "--kind=class",
                "--namespace=example::nested",
                "--name=Counter",
                "--description=strong int; +, -, ==, !=",
                "--guard-prefix=MYPROJECT",
                "--guard-separator=__",
                "--upcase-guard=false"};

            auto result = AtlasCommandLine::parse(args);

            CHECK(result.kind == "class");
            CHECK(result.type_namespace == "example::nested");
            CHECK(result.type_name == "Counter");
            CHECK(result.description == "strong int; +, -, ==, !=");
            CHECK(result.guard_prefix == "MYPROJECT");
            CHECK(result.guard_separator == "__");
            CHECK(result.upcase_guard == false);
            CHECK(result.help == false);
        }

        SUBCASE("complex description with operators") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=math",
                "--name=Number",
                "--description=strong double; +, -, *, /, ==, !=, <, <=, >, "
                ">=, ++, bool, out, in"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(
                result.description ==
                "strong double; +, -, *, /, ==, !=, <, <=, >, >=, ++, bool, "
                "out, in");
        }
    }

    TEST_CASE("Help Argument")
    {
        SUBCASE("--help flag") {
            std::vector<std::string> args{"--help"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.help == true);
        }

        SUBCASE("-h flag") {
            std::vector<std::string> args{"-h"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.help == true);
        }

        SUBCASE("help with other arguments") {
            std::vector<std::string> args{
                "--kind=struct",
                "--help",
                "--namespace=test"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.help == true);
            // Other arguments should be ignored when help is requested
        }
    }

    TEST_CASE("Boolean Value Parsing")
    {
        SUBCASE("upcase-guard true values") {
            for (auto const & value : {"true", "1", "yes"}) {
                std::vector<std::string> args{
                    "--kind=struct",
                    "--namespace=test",
                    "--name=Type",
                    "--description=strong int",
                    "--upcase-guard=" + std::string(value)};

                auto result = AtlasCommandLine::parse(args);
                CHECK(result.upcase_guard == true);
            }
        }

        SUBCASE("upcase-guard false values") {
            for (auto const & value : {"false", "0", "no"}) {
                std::vector<std::string> args{
                    "--kind=struct",
                    "--namespace=test",
                    "--name=Type",
                    "--description=strong int",
                    "--upcase-guard=" + std::string(value)};

                auto result = AtlasCommandLine::parse(args);
                CHECK(result.upcase_guard == false);
            }
        }

        SUBCASE("invalid boolean value") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int",
                "--upcase-guard=invalid"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }
    }

    TEST_CASE("Error Cases")
    {
        SUBCASE("no arguments") {
            std::vector<std::string> args{};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("missing required arguments") {
            std::vector<std::string> args{"--kind=struct"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("invalid argument format - no equals") {
            std::vector<std::string> args{
                "--kind",
                "struct", // Wrong format
                "--namespace=test",
                "--name=Type",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("invalid argument format - no double dash") {
            std::vector<std::string> args{
                "kind=struct", // Missing --
                "--namespace=test",
                "--name=Type",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("unknown argument") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int",
                "--unknown=value"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("invalid kind value") {
            std::vector<std::string> args{
                "--kind=invalid",
                "--namespace=test",
                "--name=Type",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("invalid namespace characters") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test-invalid",
                "--name=Type",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("invalid type name starting with digit") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=123Invalid",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("empty type name") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }
    }

    TEST_CASE("Conversion to StrongTypeDescription")
    {
        SUBCASE("successful conversion") {
            std::vector<std::string> args{
                "--kind=class",
                "--namespace=example",
                "--name=MyType",
                "--description=strong int; +, -, ==",
                "--guard-prefix=CUSTOM",
                "--guard-separator=__",
                "--upcase-guard=false"};

            auto parsed = AtlasCommandLine::parse(args);
            auto desc = AtlasCommandLine::to_description(parsed);

            CHECK(desc.kind == "class");
            CHECK(desc.type_namespace == "example");
            CHECK(desc.type_name == "MyType");
            CHECK(desc.description == "strong int; +, -, ==");
            CHECK(desc.guard_prefix == "CUSTOM");
            CHECK(desc.guard_separator == "__");
            CHECK(desc.upcase_guard == false);
        }

        SUBCASE("conversion from help request fails") {
            std::vector<std::string> args{"--help"};
            auto parsed = AtlasCommandLine::parse(args);

            CHECK_THROWS_AS(
                AtlasCommandLine::to_description(parsed),
                AtlasCommandLineError);
        }
    }

    TEST_CASE("Help Text")
    {
        SUBCASE("help text is non-empty and contains expected elements") {
            auto help = AtlasCommandLine::get_help_text();

            CHECK(not help.empty());
            CHECK(
                help.find("Atlas Strong Type Generator") != std::string::npos);
            CHECK(help.find("--kind=") != std::string::npos);
            CHECK(help.find("--namespace=") != std::string::npos);
            CHECK(help.find("--name=") != std::string::npos);
            CHECK(help.find("--description=") != std::string::npos);
            CHECK(help.find("EXAMPLES:") != std::string::npos);
            CHECK(help.find("OPERATOR REFERENCE:") != std::string::npos);
        }
    }

    TEST_CASE("Edge Cases")
    {
        SUBCASE("empty values are allowed for optional arguments") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int",
                "--guard-prefix="};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.guard_prefix.empty());
        }

        SUBCASE("values with spaces and special characters") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=MyType",
                "--description=strong std::vector<int>; +, -, ==, out"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(
                result.description == "strong std::vector<int>; +, -, ==, out");
        }

        SUBCASE("nested namespaces") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=a::b::c::d",
                "--name=Type",
                "--description=strong int"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.type_namespace == "a::b::c::d");
        }

        SUBCASE("type name with scoping") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Container::Element",
                "--description=strong int"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.type_name == "Container::Element");
        }
    }

    TEST_CASE("Argument Order Independence")
    {
        SUBCASE("different argument orders produce same result") {
            std::vector<std::string> args1{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int"};
            std::vector<std::string> args2{
                "--description=strong int",
                "--name=Type",
                "--kind=struct",
                "--namespace=test"};

            auto result1 = AtlasCommandLine::parse(args1);
            auto result2 = AtlasCommandLine::parse(args2);

            CHECK(result1.kind == result2.kind);
            CHECK(result1.type_namespace == result2.type_namespace);
            CHECK(result1.type_name == result2.type_name);
            CHECK(result1.description == result2.description);
        }
    }

    TEST_CASE("File Input/Output Options")
    {
        SUBCASE("input file option") {
            std::vector<std::string> args{"--input=types.txt"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.input_file == "types.txt");
            CHECK(result.help == false);
        }

        SUBCASE("output file option") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int",
                "--output=output.hpp"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.output_file == "output.hpp");
        }

        SUBCASE("input and output together") {
            std::vector<std::string> args{
                "--input=input.txt",
                "--output=output.hpp"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.input_file == "input.txt");
            CHECK(result.output_file == "output.hpp");
        }

        SUBCASE("input file bypasses normal validation") {
            // With input file, we don't need kind, namespace, name, description
            std::vector<std::string> args{"--input=types.txt"};

            CHECK_NOTHROW(AtlasCommandLine::parse(args));
        }

        SUBCASE("output file can be used with command-line mode") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int",
                "--output=result.hpp"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.kind == "struct");
            CHECK(result.output_file == "result.hpp");
        }
    }

    TEST_CASE("Interactions Mode")
    {
        SUBCASE("interactions flag parsing - true values") {
            for (auto const & value : {"true", "1", "yes"}) {
                std::vector<std::string> args{
                    "--input=interactions.txt",
                    "--interactions=" + std::string(value)};

                auto result = AtlasCommandLine::parse(args);
                CHECK(result.interactions_mode == true);
            }
        }

        SUBCASE("interactions flag parsing - false values") {
            for (auto const & value : {"false", "0", "no"}) {
                std::vector<std::string> args{
                    "--input=interactions.txt",
                    "--interactions=" + std::string(value)};

                auto result = AtlasCommandLine::parse(args);
                CHECK(result.interactions_mode == false);
            }
        }

        SUBCASE("interactions mode requires input file") {
            std::vector<std::string> args{"--interactions=true"};

            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse(args),
                "Interactions mode (--interactions=true) requires an input file. "
                "Use --input=<file> to specify the interaction file.",
                AtlasCommandLineError);
        }

        SUBCASE("interactions mode with input file succeeds") {
            std::vector<std::string> args{
                "--interactions=true",
                "--input=interactions.txt"};

            CHECK_NOTHROW(AtlasCommandLine::parse(args));
        }
    }

    TEST_CASE("Interaction File Parsing")
    {
        SUBCASE("valid interaction file can be parsed without errors") {
            // USER EXPECTATION: A properly formatted interaction file should parse successfully
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Define how Price and Discount interact\n";
                out << "namespace=finance\n";
                out << "\n";
                out << "Price + Discount -> Price\n";
                out << "Price - Discount -> Price\n";
            }

            // User expects this to succeed and return interaction descriptions
            CHECK_NOTHROW({
                auto result = AtlasCommandLine::parse_interaction_file(temp_file.string());
                CHECK(result.interactions.size() == 2);
            });

            std::filesystem::remove(temp_file);
        }

        SUBCASE("symmetric interactions work in both directions") {
            // USER EXPECTATION: <-> should define bidirectional operators
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "Vector + Vector <-> Vector\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(temp_file.string());

            // User expects exactly one interaction that is marked symmetric
            REQUIRE(result.interactions.size() == 1);
            CHECK(result.interactions[0].symmetric == true);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("include directives are preserved for code generation") {
            // USER EXPECTATION: Includes should appear in generated code
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "include <vector>\n";
                out << "include \"my_types.hpp\"\n";
                out << "namespace=test\n";
                out << "A + B -> C\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(temp_file.string());

            // User expects includes to be captured for code generation
            REQUIRE(result.includes.size() == 2);
            CHECK(result.includes[0] == "<vector>");
            CHECK(result.includes[1] == "\"my_types.hpp\"");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("constexpr setting affects subsequent interactions") {
            // USER EXPECTATION: constexpr/no-constexpr should control generated code
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "constexpr\n";
                out << "IntValue + IntValue -> IntValue\n";
                out << "no-constexpr\n";
                out << "StringValue + StringValue -> StringValue\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(temp_file.string());

            // User expects first to be constexpr, second not to be
            REQUIRE(result.interactions.size() == 2);
            CHECK(result.interactions[0].is_constexpr == true);
            CHECK(result.interactions[1].is_constexpr == false);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("comments and blank lines are ignored") {
            // USER EXPECTATION: Comments shouldn't affect parsing
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Configuration section\n";
                out << "namespace=test\n";
                out << "\n";
                out << "# Define addition\n";
                out << "A + B -> C\n";
                out << "\n";
                out << "# Define subtraction\n";
                out << "A - B -> C\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(temp_file.string());

            // User expects only the 2 interactions, comments ignored
            CHECK(result.interactions.size() == 2);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("all common arithmetic and comparison operators supported") {
            // USER EXPECTATION: Common C++ operators should work
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A + B -> C\n";
                out << "A - B -> C\n";
                out << "A * B -> C\n";
                out << "A / B -> C\n";
                out << "A == B -> bool\n";
                out << "A != B -> bool\n";
                out << "A < B -> bool\n";
                out << "A > B -> bool\n";
                out << "A <= B -> bool\n";
                out << "A >= B -> bool\n";
            }

            // User expects all standard operators to parse successfully
            CHECK_NOTHROW({
                auto result = AtlasCommandLine::parse_interaction_file(temp_file.string());
                CHECK(result.interactions.size() == 10);
            });

            std::filesystem::remove(temp_file);
        }

        SUBCASE("nonexistent file produces helpful error") {
            // USER EXPECTATION: Clear error when file doesn't exist
            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse_interaction_file("/nonexistent/file.txt"),
                "Cannot open interaction file: /nonexistent/file.txt",
                AtlasCommandLineError);
        }

        SUBCASE("invalid syntax produces error") {
            // USER EXPECTATION: Malformed interactions should be rejected with error
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A B -> C\n";  // Missing operator
            }

            // User expects clear error message for syntax problems
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("incomplete interaction definition produces error") {
            // USER EXPECTATION: Missing result type should be caught
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A + B ->\n";  // Missing result type
            }

            // User expects validation error for incomplete definitions
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Default Value Option")
    {
        SUBCASE("default-value with numeric value") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Counter",
                "--description=strong int",
                "--default-value=0"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.default_value == "0");
        }

        SUBCASE("default-value with complex expression") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Size",
                "--description=strong std::size_t",
                "--default-value=SIZE_MAX"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.default_value == "SIZE_MAX");
        }

        SUBCASE("default-value is optional") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Value",
                "--description=strong int"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.default_value.empty());
        }

        SUBCASE("default-value with explicit empty string") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Value",
                "--description=strong int",
                "--default-value="};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.default_value.empty());
        }
    }

    TEST_CASE("Input File Parsing with Type Definitions")
    {
        SUBCASE("single type definition file works") {
            // USER EXPECTATION: Simple input file with one type should parse
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=Counter\n";
                out << "description=strong int; ++, --\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            // User expects to get one type definition back
            CHECK_NOTHROW({
                auto result = AtlasCommandLine::parse_input_file(args);
                CHECK(result.types.size() == 1);
                CHECK(result.types[0].type_name == "Counter");
            });

            std::filesystem::remove(temp_file);
        }

        SUBCASE("multiple types can be defined in one file") {
            // USER EXPECTATION: Batch definition of types in single file
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=finance\n";
                out << "name=Price\n";
                out << "description=strong double; +, -, *\n";
                out << "\n";
                out << "[type]\n";
                out << "kind=class\n";
                out << "namespace=finance\n";
                out << "name=Quantity\n";
                out << "description=strong int; +, -\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            auto result = AtlasCommandLine::parse_input_file(args);

            // User expects both types to be captured
            REQUIRE(result.types.size() == 2);
            CHECK(result.types[0].type_name == "Price");
            CHECK(result.types[1].type_name == "Quantity");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("global configuration applies to all types") {
            // USER EXPECTATION: Set config once, applies to all types in file
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Project-wide settings\n";
                out << "guard_prefix=MYPROJECT\n";
                out << "guard_separator=__\n";
                out << "upcase_guard=false\n";
                out << "\n";
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=FirstType\n";
                out << "description=strong int\n";
                out << "\n";
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=SecondType\n";
                out << "description=strong int\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            auto result = AtlasCommandLine::parse_input_file(args);

            // User expects config to apply to all types
            REQUIRE(result.types.size() == 2);
            CHECK(result.types[0].guard_prefix == "MYPROJECT");
            CHECK(result.types[1].guard_prefix == "MYPROJECT");
            CHECK(result.upcase_guard == false);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("default values can be specified per type") {
            // USER EXPECTATION: Each type can have its own default value
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=Counter\n";
                out << "description=strong int\n";
                out << "default_value=0\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            auto result = AtlasCommandLine::parse_input_file(args);

            // User expects default value to be captured
            REQUIRE(result.types.size() == 1);
            CHECK(result.types[0].default_value == "0");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("comments and whitespace don't affect parsing") {
            // USER EXPECTATION: Can document input files with comments
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Configuration for my strong types\n";
                out << "\n";
                out << "[type]\n";
                out << "# This is a counter type\n";
                out << "kind=struct\n";
                out << "\n";
                out << "namespace=test\n";
                out << "name=Value\n";
                out << "description=strong int\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            auto result = AtlasCommandLine::parse_input_file(args);

            // User expects comments/blanks ignored, type still parsed
            CHECK(result.types.size() == 1);
            CHECK(result.types[0].type_name == "Value");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("command-line arguments override file settings") {
            // USER EXPECTATION: Command-line takes precedence over file
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "guard_prefix=FROM_FILE\n";
                out << "\n";
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=Value\n";
                out << "description=strong int\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();
            args.guard_prefix = "FROM_CMDLINE";

            auto result = AtlasCommandLine::parse_input_file(args);

            // User expects command-line value to win
            CHECK(result.guard_prefix == "FROM_CMDLINE");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("missing file produces helpful error") {
            // USER EXPECTATION: Clear error message when file doesn't exist
            AtlasCommandLine::Arguments args;
            args.input_file = "/nonexistent/file.txt";

            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse_input_file(args),
                "Cannot open input file: /nonexistent/file.txt",
                AtlasCommandLineError);
        }

        SUBCASE("file with only config and no types produces error") {
            // USER EXPECTATION: Must have at least one type definition
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Configuration only, no types\n";
                out << "guard_prefix=TEST\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            // User expects error: you need at least one type!
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("incomplete type definition is rejected") {
            // USER EXPECTATION: All required fields must be present
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                // Missing name and description - incomplete!
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            // User expects validation error
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("malformed syntax is rejected") {
            // USER EXPECTATION: Invalid format should produce error
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind struct\n"; // Oops, forgot the = sign!
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            // User expects syntax error
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("typos in configuration keys are caught") {
            // USER EXPECTATION: Misspelled keys should fail, not be silently ignored
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "gaurd_prefix=MYPROJECT\n";  // Oops, typo: "gaurd" not "guard"
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            // User expects error so they can fix the typo
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Specific Error Validation Paths")
    {
        SUBCASE("missing --kind specifically") {
            std::vector<std::string> args{
                // Missing --kind
                "--namespace=test",
                "--name=Value",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("missing --namespace specifically") {
            std::vector<std::string> args{
                "--kind=struct",
                // Missing --namespace
                "--name=Value",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("missing --name specifically") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                // Missing --name
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("missing --description specifically") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Value"};
            // Missing --description

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }
    }
}

} // anonymous namespace
