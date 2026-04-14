// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/AtlasMain.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "doctest.hpp"

namespace {

using namespace wjh::atlas;

// Helper to capture stdout
struct StdoutCapture
{
    StdoutCapture()
    : old_buf(std::cout.rdbuf(buffer.rdbuf()))
    { }

    ~StdoutCapture() { std::cout.rdbuf(old_buf); }

    std::string get() const { return buffer.str(); }

private:
    std::stringstream buffer;
    std::streambuf * old_buf;
};

// Helper to capture stderr
struct StderrCapture
{
    StderrCapture()
    : old_buf(std::cerr.rdbuf(buffer.rdbuf()))
    { }

    ~StderrCapture() { std::cerr.rdbuf(old_buf); }

    std::string get() const { return buffer.str(); }

private:
    std::stringstream buffer;
    std::streambuf * old_buf;
};

} // anonymous namespace

TEST_SUITE("Atlas Main Tests")
{
    TEST_CASE("atlas_main outputs version with --version flag")
    {
        char const * argv[] = {"atlas", "--version"};

        StdoutCapture capture;
        int result = atlas_main(2, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("Atlas Strong Type Generator v") != std::string::npos);
    }

    TEST_CASE("atlas_main outputs warnings for redundant equality operators")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=TestType",
            "--description=strong int; <=>, ==, !="};

        StdoutCapture stdout_capture;
        StderrCapture stderr_capture;

        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);

        auto stderr_output = stderr_capture.get();
        CHECK(stderr_output.find("Warnings:") != std::string::npos);
        CHECK(stderr_output.find("TestType:") != std::string::npos);
        CHECK(
            stderr_output.find("makes '==' and '!=' redundant") !=
            std::string::npos);
    }

    TEST_CASE("atlas_main outputs warnings for redundant relational operators")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=TestType",
            "--description=strong int; <=>, <, >, <=, >="};

        StdoutCapture stdout_capture;
        StderrCapture stderr_capture;

        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);

        auto stderr_output = stderr_capture.get();
        CHECK(stderr_output.find("Warnings:") != std::string::npos);
        CHECK(stderr_output.find("TestType:") != std::string::npos);
        CHECK(
            stderr_output.find("makes '<', '<=', '>', '>=' redundant") !=
            std::string::npos);
    }

    TEST_CASE("atlas_main outputs multiple warnings")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=MultiWarn",
            "--description=strong int; <=>, ==, !=, <, >, <=, >="};

        StdoutCapture stdout_capture;
        StderrCapture stderr_capture;

        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);

        auto stderr_output = stderr_capture.get();
        CHECK(stderr_output.find("Warnings:") != std::string::npos);
        CHECK(stderr_output.find("MultiWarn:") != std::string::npos);
        // Should have both warnings
        CHECK(
            stderr_output.find("makes '==' and '!=' redundant") !=
            std::string::npos);
        CHECK(
            stderr_output.find("makes '<', '<=', '>', '>=' redundant") !=
            std::string::npos);
    }

    TEST_CASE("CLI --auto-hash=yes generates auto hash boilerplate")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=HashType",
            "--description=strong int",
            "--auto-hash=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("WJH_ATLAS_83B11BF12B6945019DF71C7517A1D6DA") !=
            std::string::npos);
    }

    TEST_CASE("auto_hash omits constexpr operator() by default")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=HashType",
            "--description=strong int",
            "--auto-hash=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        // C++20 path should NOT have constexpr by default
        CHECK(
            output.find("constexpr std::size_t operator()") ==
            std::string::npos);
        // But should still have the hash boilerplate
        CHECK(
            output.find("WJH_ATLAS_83B11BF12B6945019DF71C7517A1D6DA") !=
            std::string::npos);
    }

    TEST_CASE("auto_hash with constexpr_hash=true enables constexpr")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=HashType",
            "--description=strong int",
            "--auto-hash=yes",
            "--constexpr-hash=true"};

        StdoutCapture capture;
        int result = atlas_main(7, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        // C++20 path should have constexpr when explicitly enabled
        CHECK(
            output.find("constexpr std::size_t operator()") !=
            std::string::npos);
    }

    TEST_CASE("auto_hash generates pre-C++20 hash_base in preamble")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=HashType",
            "--description=strong int",
            "--auto-hash=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(output.find("struct hash_base") != std::string::npos);
        CHECK(output.find("#else // pre-C++20") != std::string::npos);
    }

    TEST_CASE("auto_hash generates per-type hash specialization for pre-C++20")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=HashType",
            "--description=strong int",
            "--auto-hash=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        // Per-type one-liner for pre-C++20
        CHECK(
            output.find(
                "struct std::hash<test::HashType>\n"
                ": atlas::atlas_detail::hash_base<test::HashType>") !=
            std::string::npos);
        // Wrapped in pre-C++20 guard
        CHECK(
            output.find("#if __cplusplus < 202002L") != std::string::npos);
    }

    TEST_CASE("CLI --auto-ostream=yes generates auto ostream boilerplate")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=OstreamType",
            "--description=strong int",
            "--auto-ostream=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("WJH_ATLAS_204DEDF8AD1A4B8EA2910C659C4523BC") !=
            std::string::npos);
    }

    TEST_CASE("CLI --auto-istream=yes generates auto istream boilerplate")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=IstreamType",
            "--description=strong int",
            "--auto-istream=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("WJH_ATLAS_8E1585765002403FBFA3B92531F0A25E") !=
            std::string::npos);
    }

    TEST_CASE("CLI --auto-format=yes generates auto format boilerplate")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=FormatType",
            "--description=strong int",
            "--auto-format=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("WJH_ATLAS_A3A2ADA707CA47BE9EB94254C729C906") !=
            std::string::npos);
    }

    TEST_CASE("CLI --auto-* flags work with --input=file")
    {
        // Create temp file with a simple type and NO auto_* directives
        std::string const temp_file = [] {
            std::string file_template;
            if (auto tmpdir = ::getenv("TMPDIR")) {
                file_template = tmpdir;
            } else {
                file_template = "/tmp";
            }
            file_template += "/atlas.XXXXXX";
            int fd = ::mkstemp(file_template.data());
            if (fd < 0) {
                throw std::runtime_error(
                    "mkstemp failed with \"" + file_template +
                    "\": maybe set TMPDIR to a directory you can write to");
            }
            auto const s = std::string("[test::AutoCliFile]\n"
                                       "kind=struct\n"
                                       "description=strong int\n");
            if (::write(fd, s.data(), s.size()) == -1) { ::close(fd); FAIL("write failed"); }
            ::close(fd);
            return file_template;
        }();

        std::string const input_arg = "--input=" + temp_file;
        char const * argv[] = {
            "atlas",
            input_arg.c_str(),
            "--auto-hash=yes",
            "--auto-ostream=yes",
            "--auto-istream=yes",
            "--auto-format=yes"};

        StdoutCapture capture;
        int result = atlas_main(6, const_cast<char **>(argv));

        std::remove(temp_file.c_str());

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("WJH_ATLAS_83B11BF12B6945019DF71C7517A1D6DA") !=
            std::string::npos);
        CHECK(
            output.find("WJH_ATLAS_204DEDF8AD1A4B8EA2910C659C4523BC") !=
            std::string::npos);
        CHECK(
            output.find("WJH_ATLAS_8E1585765002403FBFA3B92531F0A25E") !=
            std::string::npos);
        CHECK(
            output.find("WJH_ATLAS_A3A2ADA707CA47BE9EB94254C729C906") !=
            std::string::npos);
    }

    TEST_CASE("CLI --auto-* flags merge with file-level auto settings")
    {
        // Create temp file with auto_hash=true at file level
        std::string const temp_file = [] {
            std::string file_template;
            if (auto tmpdir = ::getenv("TMPDIR")) {
                file_template = tmpdir;
            } else {
                file_template = "/tmp";
            }
            file_template += "/atlas.XXXXXX";
            int fd = ::mkstemp(file_template.data());
            if (fd < 0) {
                throw std::runtime_error(
                    "mkstemp failed with \"" + file_template +
                    "\": maybe set TMPDIR to a directory you can write to");
            }
            auto const s = std::string("auto_hash=true\n"
                                       "[test::MergeAutoType]\n"
                                       "kind=struct\n"
                                       "description=strong int\n");
            if (::write(fd, s.data(), s.size()) == -1) { ::close(fd); FAIL("write failed"); }
            ::close(fd);
            return file_template;
        }();

        std::string const input_arg = "--input=" + temp_file;
        char const * argv[] = {
            "atlas",
            input_arg.c_str(),
            "--auto-ostream=yes"};

        StdoutCapture capture;
        int result = atlas_main(3, const_cast<char **>(argv));

        std::remove(temp_file.c_str());

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        // auto hash from file-level
        CHECK(
            output.find("WJH_ATLAS_83B11BF12B6945019DF71C7517A1D6DA") !=
            std::string::npos);
        // auto ostream from CLI
        CHECK(
            output.find("WJH_ATLAS_204DEDF8AD1A4B8EA2910C659C4523BC") !=
            std::string::npos);
    }

    TEST_CASE("Without --auto-* flags boilerplates are absent")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=PlainType",
            "--description=strong int"};

        StdoutCapture capture;
        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(
            output.find("WJH_ATLAS_83B11BF12B6945019DF71C7517A1D6DA") ==
            std::string::npos);
        CHECK(
            output.find("WJH_ATLAS_204DEDF8AD1A4B8EA2910C659C4523BC") ==
            std::string::npos);
        CHECK(
            output.find("WJH_ATLAS_8E1585765002403FBFA3B92531F0A25E") ==
            std::string::npos);
        CHECK(
            output.find("WJH_ATLAS_A3A2ADA707CA47BE9EB94254C729C906") ==
            std::string::npos);
    }

    TEST_CASE("Command-line and file-based generation produce identical output")
    {
        // Create a comprehensive test with many features to ensure consistency
        std::string const description =
            "description="
            "strong std::string; +, -, ==, <, ++, @, ->, out, in, hash";

        // Test via command-line with explicit guard prefix
        std::string const desc_arg = "--" + description;
        char const * cmd_argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=ComprehensiveType",
            "--guard-prefix=TEST_GUARD",
            desc_arg.c_str()};

        StdoutCapture cmd_stdout;
        StderrCapture cmd_stderr;
        int cmd_result = atlas_main(6, const_cast<char **>(cmd_argv));

        CHECK(cmd_result == EXIT_SUCCESS);
        std::string cmd_output = cmd_stdout.get();

        // Create temporary file with same specification using raw string
        // literal
        std::string const temp_file = [&description] {
            std::string file_template;
            if (auto tmpdir = ::getenv("TMPDIR")) {
                file_template = tmpdir;
            } else {
                file_template = "/tmp";
            }
            file_template += "/atlas.XXXXXX";
            int fd = ::mkstemp(file_template.data());
            if (fd < 0) {
                throw std::runtime_error(
                    "mkstemp failed with \"" + file_template +
                    "\": maybe set TMPDIR to a directory you can write to");
            }
            auto const s = std::string("guard_prefix=TEST_GUARD\n"
                                       "[test::ComprehensiveType]\n"
                                       "kind=struct\n") +
                description + "\n";
            if (::write(fd, s.data(), s.size()) == -1) { ::close(fd); FAIL("write failed"); }
            ::close(fd);
            return file_template;
        }();

        // Test via file-based input
        std::string const input_arg = "--input=" + temp_file;
        char const * file_argv[] = {"atlas", input_arg.c_str()};

        StdoutCapture file_stdout;
        StderrCapture file_stderr;
        int file_result = atlas_main(2, const_cast<char **>(file_argv));

        CHECK(file_result == EXIT_SUCCESS);
        std::string file_output = file_stdout.get();

        // Clean up temp file
        std::remove(temp_file.c_str());

        // With identical guard prefix, outputs should be completely identical
        CHECK(cmd_output == file_output);
    }
}
