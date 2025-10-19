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
    TEST_CASE("Help and Version Flags")
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

        SUBCASE("help text contains expected elements") {
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

        SUBCASE("conversion from help request fails") {
            std::vector<std::string> args{"--help"};
            auto parsed = AtlasCommandLine::parse(args);

            CHECK_THROWS_AS(
                AtlasCommandLine::to_description(parsed),
                AtlasCommandLineError);
        }
    }

    TEST_CASE("Argument Format Validation")
    {
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
    }

    TEST_CASE("Required Arguments Validation")
    {
        SUBCASE("missing --kind specifically") {
            std::vector<std::string> args{
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

            CHECK_THROWS_AS(
                AtlasCommandLine::parse(args),
                AtlasCommandLineError);
        }
    }

    TEST_CASE("Value Validation")
    {
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

    TEST_CASE("Input File Validation")
    {
        SUBCASE("nonexistent file produces helpful error") {
            AtlasCommandLine::Arguments args;
            args.input_file = "/nonexistent/file.txt";

            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse_input_file(args),
                "Cannot open input file: /nonexistent/file.txt",
                AtlasCommandLineError);
        }

        SUBCASE("empty input file path") {
            AtlasCommandLine::Arguments args;

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);
        }

        SUBCASE("file with only config and no types produces error") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "# Configuration only, no types\n";
                out << "guard_prefix=TEST\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("malformed syntax without equals") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind struct\n"; // Missing equals
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("unknown configuration key") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "gaurd_prefix=MYPROJECT\n"; // Typo: "gaurd" not "guard"
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
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
                out << "unknown_property=foo\n";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("incomplete type definition is rejected") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_input_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "[type]\n";
                out << "kind=struct\n";
                out << "namespace=test\n";
                // Missing name and description
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Interaction Mode Validation")
    {
        SUBCASE("interactions mode requires input file") {
            std::vector<std::string> args{"--interactions=true"};

            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse(args),
                "Interactions mode (--interactions=true) requires an input "
                "file. "
                "Use --input=<file> to specify the interaction file.",
                AtlasCommandLineError);
        }

        SUBCASE("interaction file nonexistent produces helpful error") {
            CHECK_THROWS_WITH_AS(
                AtlasCommandLine::parse_interaction_file(
                    "/nonexistent/file.txt"),
                "Cannot open interaction file: /nonexistent/file.txt",
                AtlasCommandLineError);
        }

        SUBCASE("invalid interaction syntax - missing operator") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A B -> C\n"; // Missing operator
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("incomplete interaction definition - missing result") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "A + B ->\n"; // Missing result type
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
                out << " + B -> C\n";
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
                out << "A +  -> C\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("malformed include without space") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "include\n"; // No space after include
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
                out << "concept=\n"; // Empty after equals
                out << "A + B -> C\n";
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("unknown directive produces error") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << "namespace=test\n";
                out << "unknown_directive=value\n";
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
            }

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_interaction_file(temp_file.string()),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }

        SUBCASE("empty enable_if value throws error") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

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
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_interactions_" + std::to_string(::getpid()) + ".txt");

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
    }

    TEST_CASE("Argument Precedence")
    {
        SUBCASE("command-line arguments override file settings") {
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

            CHECK(result.guard_prefix == "FROM_CMDLINE");

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Global Namespace Support")
    {
        SUBCASE("missing global and type namespace throws error") {
            auto temp_file = std::filesystem::temp_directory_path() /
                ("test_no_ns_" + std::to_string(::getpid()) + ".txt");

            {
                std::ofstream out(temp_file);
                out << R"([type]
kind=struct
name=NoNamespace
description=strong int
)";
            }

            AtlasCommandLine::Arguments args;
            args.input_file = temp_file.string();

            CHECK_THROWS_AS(
                AtlasCommandLine::parse_input_file(args),
                AtlasCommandLineError);

            std::filesystem::remove(temp_file);
        }
    }

    TEST_CASE("Edge Cases")
    {
        SUBCASE("empty values allowed for optional arguments") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=Type",
                "--description=strong int",
                "--guard-prefix="};

            auto result = AtlasCommandLine::parse(args);
            CHECK(result.guard_prefix.empty());
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

        SUBCASE("complex description with special characters") {
            std::vector<std::string> args{
                "--kind=struct",
                "--namespace=test",
                "--name=MyType",
                "--description=strong std::vector<int>; +, -, ==, out"};

            auto result = AtlasCommandLine::parse(args);
            CHECK(
                result.description == "strong std::vector<int>; +, -, ==, out");
        }
    }
}

} // anonymous namespace
