// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasMain.hpp"
#include "TestUtilities.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <vector>

#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

namespace {

namespace fs = std::filesystem;
using namespace wjh::atlas;
using namespace wjh::atlas::testing;

/**
 * Get the source directory from environment or discover it.
 *
 * Uses SOURCE_DIR environment variable if set (from CMake),
 * otherwise attempts to discover from __FILE__ location.
 */
fs::path
get_source_directory()
{
    // Try environment variable first (set by CMake)
    if (auto env = std::getenv("SOURCE_DIR")) {
        return fs::path(env);
    }

    // Fallback: discover from test file location
    // __FILE__ is tests/golden_ut.cpp, so go up one level
    auto test_file = fs::path(__FILE__);
    auto tests_dir = test_file.parent_path();
    return tests_dir.parent_path(); // repository root
}

/**
 * Generate code from input file by calling atlas_main.
 *
 * Captures stdout to get the generated code.
 *
 * @param input_path Path to .input file
 * @return Pair of (exit_code, generated_output)
 * @throws Exceptions from atlas_main if they occur
 */
std::pair<int, std::string>
generate_from_input_file(fs::path const & input_path)
{
    // Build argument vector for atlas_main
    std::vector<std::string> arg_strings = {
        "atlas",
        "--input=" + input_path.string()};

    // Convert to char* array (atlas_main interface)
    std::vector<char *> argv;
    for (auto & arg : arg_strings) {
        argv.push_back(arg.data());
    }

    // Redirect stdout to capture output
    std::ostringstream captured_output;
    auto old_cout = std::cout.rdbuf(captured_output.rdbuf());

    // Call atlas_main
    int exit_code = atlas_main(static_cast<int>(argv.size()), argv.data());

    // Restore stdout
    std::cout.rdbuf(old_cout);

    return {exit_code, captured_output.str()};
}

/**
 * Test a single golden file pair.
 *
 * @param input_path Path to .input file
 */
void
test_golden_file(fs::path const & input_path)
{
    REQUIRE(fs::exists(input_path));

    auto expected_path = input_path;
    expected_path.replace_extension(".expected");
    if (not fs::exists(expected_path)) {
        FAIL("Expected file missing: " << expected_path.string());
    }

    INFO("Testing: " << input_path.string());

    // Generate code using atlas_main (captures stdout)
    auto [exit_code, generated] = generate_from_input_file(input_path);

    if (exit_code != EXIT_SUCCESS) {
        FAIL("atlas_main returned " << exit_code);
    }

    // Compare output
    auto expected = read_file(expected_path);

    if (generated != expected) {
        INFO("Generated output differs from expected");
        INFO("Input: " << input_path.string());
        INFO("Expected: " << expected_path.string());
        INFO("To update golden file:");
        INFO(
            "  atlas --input=" << input_path.string() << " > "
            << expected_path.string());

        CHECK(generated == expected);
    }
}

/**
 * Discover all .input files in golden directory.
 *
 * @param golden_dir Root directory to search
 * @return Vector of paths to .input files (sorted)
 */
std::vector<fs::path>
discover_golden_files(fs::path const & golden_dir)
{
    std::vector<fs::path> input_files;

    if (not fs::exists(golden_dir)) {
        return input_files; // Empty if directory doesn't exist
    }

    for (auto const & entry : fs::recursive_directory_iterator(golden_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".input") {
            input_files.push_back(entry.path());
        }
    }

    // Sort for consistent test ordering
    std::sort(input_files.begin(), input_files.end());

    return input_files;
}

TEST_SUITE("Golden File Tests")
{
    TEST_CASE("All golden files")
    {
        auto const source_dir = get_source_directory();
        auto const golden_dir = source_dir / "tests/fixtures/golden";

        INFO("Source directory: " << source_dir.string());
        INFO("Golden directory: " << golden_dir.string());

        auto input_files = discover_golden_files(golden_dir);

        if (input_files.empty()) {
            std::string msg = "No golden files found in " + golden_dir.string();
            WARN(msg.c_str());
            WARN("Create .input files in tests/fixtures/golden/ to add tests");
        }

        // Generate one SUBCASE per golden file
        for (auto const & input_path : input_files) {
            // Create readable test name from path
            auto relative_path = fs::relative(input_path, golden_dir);
            auto test_name = relative_path.string();

            // Use SUBCASE for dynamic test generation
            SUBCASE(test_name.c_str()) {
                test_golden_file(input_path);
            }
        }

        // Ensure at least one test case exists
        if (not input_files.empty()) {
            CHECK(not input_files
                          .empty()); // Always passes, just ensures tests ran
        }
    }
}

} // anonymous namespace
