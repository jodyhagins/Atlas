// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "AtlasCommandLine.hpp"
#include "doctest.hpp"
#include "rapidcheck.hpp"

#include <filesystem>
#include <fstream>

using namespace wjh::atlas;
using namespace wjh::atlas::testing;

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

        SUBCASE("unknown argument throws error") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Value",
                "--description=strong int",
                "--unknown-flag=value"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("empty arguments throws error") {
            std::vector<std::string> args{};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("argument without equals sign throws error") {
            std::vector<std::string> args{"--kind", "struct"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("argument without dashes throws error") {
            std::vector<std::string> args{"kind=struct"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }

        SUBCASE("invalid boolean for upcase-guard throws error") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Value",
                "--description=strong int",
                "--upcase-guard=maybe"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
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
                "Interactions mode (--interactions=true) requires an input "
                "file. "
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
            // USER EXPECTATION: A properly formatted interaction file should
            // parse successfully
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
                auto result = AtlasCommandLine::parse_interaction_file(
                    temp_file.string());
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

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

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

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            // User expects includes to be captured for code generation
            REQUIRE(result.includes.size() == 2);
            CHECK(result.includes[0] == "<vector>");
            CHECK(result.includes[1] == "\"my_types.hpp\"");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("constexpr setting affects subsequent interactions") {
            // USER EXPECTATION: constexpr/no-constexpr should control generated
            // code
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

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

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

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

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
                auto result = AtlasCommandLine::parse_interaction_file(
                    temp_file.string());
                CHECK(result.interactions.size() == 10);
            });

            std::filesystem::remove(temp_file);
        }

        SUBCASE("nonexistent file produces helpful error") {
            // USER EXPECTATION: Clear error when file doesn't exist
            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse_interaction_file(
                    "/nonexistent/file.txt"),
                "Cannot open interaction file: /nonexistent/file.txt",
                AtlasCommandLineError);
        }

        SUBCASE("invalid syntax produces error") {
            // USER EXPECTATION: Malformed interactions should be rejected with
            // error
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A B -> C\n"; // Missing operator
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
                out << "A + B ->\n"; // Missing result type
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

        SUBCASE("type with all optional fields specified") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Global config\n";
                out << "guard_prefix=GLOBAL_\n";
                out << "guard_separator=__\n";
                out << "upcase_guard=true\n";
                out << "\n";
                out << "[type]\n";
                out << "kind=class\n";
                out << "namespace=example::nested\n";
                out << "name=ComplexType\n";
                out << "description=strong std::string; +, ==, !=, out\n";
                out << "default_value=\"\"\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            auto result = AtlasCommandLine::parse_input_file(args);

            REQUIRE(result.types.size() == 1);
            CHECK(result.types[0].kind == "class");
            CHECK(result.types[0].type_namespace == "example::nested");
            CHECK(result.types[0].type_name == "ComplexType");
            CHECK(result.types[0].default_value == "\"\"");
            CHECK(result.types[0].guard_prefix == "GLOBAL_");
            CHECK(result.upcase_guard == true);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("upcase_guard false") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "upcase_guard=false\n";
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=Value\n";
                out << "description=strong int\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            auto result = AtlasCommandLine::parse_input_file(args);
            CHECK(result.upcase_guard == false);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("invalid line format without equals") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "this line has no equals sign\n";
                out << "[type]\n";
                out << "kind=struct\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

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
            // USER EXPECTATION: Misspelled keys should fail, not be silently
            // ignored
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "gaurd_prefix=MYPROJECT\n"; // Oops, typo: "gaurd" not
                                                   // "guard"
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            // User expects error so they can fix the typo
            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("empty input file path") {
            AtlasCommandLine::Arguments args;
            // args.input_file is empty

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);
        }

        SUBCASE("non-existent input file") {
            AtlasCommandLine::Arguments args;
            args.input_file = "/tmp/nonexistent_file_12345678.txt";

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);
        }

        SUBCASE("unknown property inside type definition") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                out << "name=Value\n";
                out << "description=strong int\n";
                out << "unknown_property=foo\n"; // Unknown!
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Specific Error Validation Paths")
    {
        SUBCASE("missing --kind specifically") {
            std::vector<std::string> args{// Missing --kind
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

        SUBCASE("invalid kind value") {
            std::vector<std::string> args{
                "--kind=invalid_kind",
                "--namespace=test",
                "--name=Value",
                "--description=strong int"};

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }
    }

    TEST_CASE("Interaction File Error Paths")
    {
        SUBCASE("malformed include without space") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "include\n"; // No space after include!
                out << "A + B -> C\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("empty concept definition value") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "concept=\n"; // Empty after equals!
                out << "A + B -> C\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }


        SUBCASE("multiple enable_if for same constraint") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "concept=T\n";
                out << "enable_if=std::is_integral_v<T>\n"; // First enable_if
                                                            // clears pending
                out << "enable_if=std::is_integral_v<T>\n"; // Second should
                                                            // apply to last
                                                            // constraint (T)
                out << "A + B -> C\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            REQUIRE(result.constraints.size() == 1);
            // Second enable_if should have overwritten the first
            CHECK(
                result.constraints["T"].enable_if_expr ==
                "std::is_integral_v<T>");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("unknown directive produces error") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "unknown_directive=value\n"; // Unknown!
                out << "A + B -> C\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("no interactions in file produces error") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Just comments\n";
                out << "namespace=test\n";
                // No actual interactions!
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("interaction with missing operator") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A B -> C\n"; // Missing operator between A and B!
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("interaction with missing result type") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A + B ->\n"; // Missing result type after arrow!
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("interaction with empty LHS") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << " + B -> C\n"; // Empty LHS!
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("interaction with empty RHS") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A +  -> C\n"; // Empty RHS!
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("interaction file with all features") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Test all features\n";
                out << "guard_prefix=MYPROJECT\n";
                out << "guard_separator=__\n";
                out << "upcase_guard=false\n";
                out << "namespace=algebra\n";
                out << "include <numeric>\n";
                out << "include \"myheader.hpp\"\n";
                out << "concept=std::integral T\n";
                out << "enable_if=std::is_arithmetic_v<T>\n";
                out << "concept=std::integral U\n";
                out << "constexpr\n";
                out << "lhs_value_access=getValue\n";
                out << "rhs_value_access=getData\n";
                out << "value_access=extract\n";
                out << "\n";
                out << "TypeA + TypeB -> TypeC\n";
                out << "TypeA * TypeB <-> TypeC\n";
                out << "no-constexpr\n";
                out << "T - U -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            CHECK(result.guard_prefix == "MYPROJECT");
            CHECK(result.guard_separator == "__");
            CHECK(result.upcase_guard == false);
            CHECK(result.includes.size() == 2);
            CHECK(result.includes[0] == "<numeric>");
            CHECK(result.includes[1] == "\"myheader.hpp\"");
            CHECK(result.constraints.size() == 2);
            CHECK(result.constraints.contains("T"));
            CHECK(result.constraints.contains("U"));
            CHECK(result.interactions.size() == 3);
            CHECK(result.interactions[0].interaction_namespace == "algebra");
            CHECK(result.interactions[0].symmetric == false);
            CHECK(result.interactions[1].symmetric == true);
            CHECK(result.interactions[0].is_constexpr == true);
            CHECK(result.interactions[2].is_constexpr == false);
            CHECK(result.interactions[0].lhs_value_access == "getValue");
            CHECK(result.interactions[0].rhs_value_access == "getData");
            CHECK(result.interactions[2].lhs_type == "T");
            CHECK(result.interactions[2].rhs_type == "U");

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Property: Boolean value parsing")
    {
        ::rc::check(
            "valid boolean strings parse without error",
            [](bool expected_value) {
                std::vector<std::string> bool_strs = expected_value
                    ? std::vector<
                          std::string>{"true", "1", "yes", "True", "YES"}
                    : std::vector<std::string>{
                          "false",
                          "0",
                          "no",
                          "False",
                          "NO"};

                for (auto const & bool_str : bool_strs) {
                    std::vector<std::string> args{
                        "--kind=struct",
                        "--namespace=test",
                        "--name=Value",
                        "--description=strong int",
                        "--upcase-guard=" + bool_str};

                    auto result = AtlasCommandLine::parse(args);
                    RC_ASSERT(result.upcase_guard == expected_value);
                }
            });

        ::rc::check(
            "invalid boolean strings throw error",
            [](std::string const & invalid_str) {
                RC_PRE(
                    invalid_str != "true" && invalid_str != "false" &&
                    invalid_str != "1" && invalid_str != "0" &&
                    invalid_str != "yes" && invalid_str != "no" &&
                    invalid_str != "True" && invalid_str != "False" &&
                    invalid_str != "YES" && invalid_str != "NO");

                std::vector<std::string> args{
                    "--kind=struct",
                    "--namespace=test",
                    "--name=Value",
                    "--description=strong int",
                    "--upcase-guard=" + invalid_str};

                RC_ASSERT_THROWS_AS(
                    AtlasCommandLine::parse(args),
                    AtlasCommandLineError);
            });
    }

    TEST_CASE("Property: File parsing with various global configs")
    {
        ::rc::check(
            "all combinations of guard settings work",
            [](bool upcase, std::string const & sep) {
                RC_PRE(sep.size() >= 1 && sep.size() <= 3);
                RC_PRE(sep.find('\n') == std::string::npos);
                RC_PRE(sep.find('\r') == std::string::npos);
                RC_PRE(sep.find('=') == std::string::npos);
                // Exclude whitespace-only strings since they get trimmed
                RC_PRE(not sep.empty());
                RC_PRE(std::any_of(sep.begin(), sep.end(), [](char c) {
                    return not std::isspace(static_cast<unsigned char>(c));
                }));

                auto temp_file = std::filesystem::temp_directory_path() /
                    ("test_prop_" + std::to_string(::getpid()) + "_" +
                     std::to_string(rand()) + ".txt");

                {
                    std::ofstream out(temp_file);
                    out << "guard_separator=" << sep << "\n";
                    out << "upcase_guard=" << (upcase ? "true" : "false")
                        << "\n";
                    out << "[type]\n";
                    out << "kind=struct\n";
                    out << "namespace=test\n";
                    out << "name=Value\n";
                    out << "description=strong int\n";
                }

                AtlasCommandLine::Arguments args;
                args.input_file = temp_file.string();

                auto result = AtlasCommandLine::parse_input_file(args);

                RC_ASSERT(result.guard_separator == sep);
                RC_ASSERT(result.upcase_guard == upcase);

                std::filesystem::remove(temp_file);
            });
    }

    TEST_CASE("Property: Interaction file with varying operators")
    {
        ::rc::check(
            "all operators parse correctly",
            [](std::string const & op, bool symmetric) {
                // Preconditions: operator must be non-empty, contain no
                // whitespace, and be a known operator symbol (not just letters
                // which would be parsed as types)
                RC_PRE(not op.empty());
                RC_PRE(op.find(' ') == std::string::npos);
                RC_PRE(op.find('\t') == std::string::npos);
                RC_PRE(op.find('\n') == std::string::npos);
                RC_PRE(op.find('\r') == std::string::npos);
                // Must contain at least one operator symbol character
                RC_PRE(std::any_of(op.begin(), op.end(), [](char c) {
                    return not std::isalnum(static_cast<unsigned char>(c)) &&
                        c != '_';
                }));

                auto temp_file = std::filesystem::temp_directory_path() /
                    ("test_inter_" + std::to_string(::getpid()) + "_" +
                     std::to_string(rand()) + ".txt");

                {
                    std::ofstream out(temp_file);
                    out << "namespace=test\n";
                    out << "TypeA " << op << " TypeB -> TypeC";
                    if (symmetric) {
                        out << " symmetric";
                    }
                    out << "\n";
                }

                auto result = AtlasCommandLine::parse_interaction_file(
                    temp_file.string());

                RC_ASSERT(result.interactions.size() == 1);
                RC_ASSERT(result.interactions[0].op_symbol == op);
                RC_ASSERT(result.interactions[0].symmetric == symmetric);
                RC_ASSERT(result.interactions[0].lhs_type == "TypeA");
                RC_ASSERT(result.interactions[0].rhs_type == "TypeB");
                RC_ASSERT(result.interactions[0].result_type == "TypeC");

                std::filesystem::remove(temp_file);
            });
    }

    TEST_CASE("Property: Invalid interaction formats always error")
    {
        ::rc::check(
            "missing operator in interaction errors",
            [](std::string const & lhs, std::string const & rhs) {
                RC_PRE(not lhs.empty() && not rhs.empty());
                RC_PRE(lhs.find(' ') == std::string::npos);
                RC_PRE(rhs.find(' ') == std::string::npos);
                RC_PRE(lhs.find('\n') == std::string::npos);
                RC_PRE(rhs.find('\n') == std::string::npos);

                auto temp_file = std::filesystem::temp_directory_path() /
                    ("test_bad_" + std::to_string(::getpid()) + "_" +
                     std::to_string(rand()) + ".txt");

                {
                    std::ofstream out(temp_file);
                    out << "namespace=test\n";
                    // No operator between types!
                    out << lhs << " " << rhs << " -> Result\n";
                }

                RC_ASSERT_THROWS_AS(
                    AtlasCommandLine::parse_interaction_file(
                        temp_file.string()),
                    AtlasCommandLineError);

                std::filesystem::remove(temp_file);
            });
    }

    TEST_CASE("Template Constraint Parsing")
    {
        SUBCASE("concept with space-separated syntax: std::integral T") {
            // USER EXPECTATION: Natural C++20 syntax should work
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_concept_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "concept=std::integral T\n";
                out << "T + T -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            // User expects T to be recognized as template parameter
            REQUIRE(result.constraints.contains("T"));
            CHECK(result.constraints["T"].concept_expr == "std::integral");
            CHECK(result.interactions[0].lhs_is_template == true);
            CHECK(result.interactions[0].rhs_is_template == true);

            std::filesystem::remove(temp_file);
        }


        SUBCASE("enable_if extracts template parameter from expression") {
            // USER EXPECTATION: "std::is_floating_point<U>::value" should
            // extract U
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_enable_if_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "enable_if=std::is_floating_point<U>::value\n";
                out << "U * U -> U\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            // User expects U to be extracted and recognized as template
            REQUIRE(result.constraints.contains("U"));
            CHECK(
                result.constraints["U"].enable_if_expr ==
                "std::is_floating_point<U>::value");
            CHECK(result.interactions[0].lhs_is_template == true);
            CHECK(result.interactions[0].rhs_is_template == true);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("both concept and enable_if for same parameter") {
            // USER EXPECTATION: Can specify both concept and enable_if
            // constraints
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_both_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "concept=std::integral V\n";
                out << "enable_if=sizeof(V) <= 8\n";
                out << "V - V -> V\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            // User expects both constraints to be captured
            REQUIRE(result.constraints.contains("V"));
            CHECK(result.constraints["V"].concept_expr == "std::integral");
            CHECK(result.constraints["V"].enable_if_expr == "sizeof(V) <= 8");
            CHECK(result.interactions[0].lhs_is_template == true);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("multiple different template parameters") {
            // USER EXPECTATION: Can define multiple template parameters
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_multi_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "concept=std::integral T\n";
                out << "concept=std::floating_point U\n";
                out << "T + T -> T\n";
                out << "U * U -> U\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            // User expects both T and U to be recognized
            REQUIRE(result.constraints.contains("T"));
            REQUIRE(result.constraints.contains("U"));
            CHECK(result.constraints["T"].concept_expr == "std::integral");
            CHECK(
                result.constraints["U"].concept_expr == "std::floating_point");
            CHECK(result.interactions.size() == 2);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("concept with complex expression") {
            // USER EXPECTATION: Complex concepts should work
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_complex_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "concept=std::convertible_to<int> T\n";
                out << "T + T -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            REQUIRE(result.constraints.contains("T"));
            CHECK(
                result.constraints["T"].concept_expr ==
                "std::convertible_to<int>");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("template parameter used with non-template types") {
            // USER EXPECTATION: Can mix template and concrete types
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_mixed_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "concept=std::integral T\n";
                out << "T + int -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            REQUIRE(result.interactions.size() == 1);
            CHECK(result.interactions[0].lhs_is_template == true);
            CHECK(result.interactions[0].rhs_is_template == false);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("empty concept value throws error") {
            // USER EXPECTATION: Must provide a concept expression
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_empty_concept_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "concept=\n";
                out << "T + T -> T\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("empty enable_if value throws error") {
            // USER EXPECTATION: Must provide an enable_if expression
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_empty_enable_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "enable_if=\n";
                out << "T + T -> T\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("enable_if without template parameter in angle brackets throws "
                "error")
        {
            // USER EXPECTATION: enable_if must have extractable parameter
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_bad_enable_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "enable_if=some_condition\n";
                out << "T + T -> T\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("concept with only parameter name uses same for both") {
            // USER EXPECTATION: "concept=T" means both name and concept are T
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_simple_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "concept=T\n";
                out << "T + T -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            REQUIRE(result.constraints.contains("T"));
            CHECK(result.constraints["T"].concept_expr == "T");

            std::filesystem::remove(temp_file);
        }

        SUBCASE("enable_if with comma in template args") {
            // USER EXPECTATION: Extract first parameter even with commas
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_comma_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "enable_if=std::is_same<T, int>::value\n";
                out << "T + T -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            // Should extract just "T" before the comma
            REQUIRE(result.constraints.contains("T"));

            std::filesystem::remove(temp_file);
        }

        SUBCASE("concept with trailing whitespace") {
            // USER EXPECTATION: Whitespace should be trimmed
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_whitespace_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=math\n";
                out << "concept=std::integral   T  \n";
                out << "T + T -> T\n";
            }

            auto result = AtlasCommandLine::parse_interaction_file(
                temp_file.string());

            REQUIRE(result.constraints.contains("T"));
            CHECK(result.constraints["T"].concept_expr == "std::integral");

            std::filesystem::remove(temp_file);
        }

    }
}

} // anonymous namespace
