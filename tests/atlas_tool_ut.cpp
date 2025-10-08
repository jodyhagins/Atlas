// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestUtilities.hpp"
#include "doctest.hpp"

#include <cstdlib>
#include <fstream>
#include <string>

// ATLAS_TOOL_PATH is defined via CMake compile definition
#ifndef ATLAS_TOOL_PATH
    #error "ATLAS_TOOL_PATH must be defined"
#endif

namespace {

using namespace wjh::atlas::testing;

struct CommandResult
{
    int exit_code;
    std::string stderr_output;
};

// Helper to run atlas tool and capture stderr
CommandResult
run_atlas(std::string const & args)
{
    TemporaryDirectory temp_dir;
    auto stderr_file = temp_dir.path() / "stderr.txt";

    std::string cmd = std::string(ATLAS_TOOL_PATH) + " " + args +
        " >/dev/null 2>" + stderr_file.string();

    int result = std::system(cmd.c_str());

    // Read stderr output
    std::string stderr_output = read_file(stderr_file);

    // Extract actual exit code from system() return value
    int exit_code;
#ifdef _WIN32
    exit_code = result;
#else
    exit_code = WEXITSTATUS(result);
#endif

    return {exit_code, stderr_output};
}

} // anonymous namespace

TEST_SUITE("Atlas Tool Exception Handling")
{
    TEST_CASE("atlas tool handles AtlasCommandLineError")
    {
        // Invalid flag should trigger AtlasCommandLineError
        auto result = run_atlas("--invalid-flag");

        CHECK(result.exit_code == EXIT_FAILURE);
        CHECK(result.stderr_output.find("Error:") != std::string::npos);
        CHECK(result.stderr_output.find("--help") != std::string::npos);
    }

    TEST_CASE("atlas tool handles std::exception from file write error")
    {
        // Attempting to write to invalid path should trigger std::runtime_error
        auto result = run_atlas(
            "--kind=struct "
            "--namespace=test "
            "--name=TestType "
            "--description='strong int' "
            "--output=/nonexistent/impossible/path/file.hpp");

        CHECK(result.exit_code == EXIT_FAILURE);
        CHECK(result.stderr_output.find("Error:") != std::string::npos);
        // This should NOT have the --help message (different catch block)
        CHECK(result.stderr_output.find("--help") == std::string::npos);
    }
}
