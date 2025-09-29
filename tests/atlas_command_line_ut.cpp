// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "AtlasCommandLine.hpp"
#include "doctest.hpp"

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
}

} // anonymous namespace
