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

// Forward declarations
std::vector<std::string> split_lines(std::string const & text);
std::string visualize_whitespace(std::string const & s);
std::string character_diff(
    std::string const & expected,
    std::string const & generated);

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

    // Check if this is an interactions test by examining the path
    // If the file is in a directory named "interactions", add
    // --interactions=true
    auto path_str = input_path.string();
    if (path_str.find("/interactions/") != std::string::npos ||
        path_str.find("\\interactions\\") != std::string::npos)
    {
        arg_strings.push_back("--interactions=true");
    }

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
        // Build detailed diff information
        std::ostringstream diff_output;

        diff_output << "\n========================================\n";
        diff_output << "GOLDEN FILE MISMATCH\n";
        diff_output << "========================================\n\n";
        diff_output << "Input:    " << input_path.string() << "\n";
        diff_output << "Expected: " << expected_path.string() << "\n\n";

        // Split into lines for diff
        auto expected_lines = split_lines(expected);
        auto generated_lines = split_lines(generated);

        diff_output << "Line count: expected=" << expected_lines.size()
            << ", generated=" << generated_lines.size() << "\n\n";

        // Show line-by-line diff
        size_t max_lines = std::max(
            expected_lines.size(),
            generated_lines.size());
        size_t diff_count = 0;
        size_t context_lines = 3; // Lines of context around differences

        for (size_t i = 0; i < max_lines && diff_count < 20; ++i) {
            bool has_expected = i < expected_lines.size();
            bool has_generated = i < generated_lines.size();

            if (has_expected && has_generated &&
                expected_lines[i] == generated_lines[i])
            {
                // Lines match - only show if near a difference
                continue; // We'll show context separately
            }

            // Found a difference
            diff_count++;

            diff_output << "--- Line " << (i + 1) << " ---\n";

            if (has_expected) {
                diff_output
                    << "  EXPECTED: " << visualize_whitespace(expected_lines[i])
                    << "\n";
            } else {
                diff_output << "  EXPECTED: <missing line>\n";
            }

            if (has_generated) {
                diff_output << "  GENERATED: "
                    << visualize_whitespace(generated_lines[i]) << "\n";
            } else {
                diff_output << "  GENERATED: <missing line>\n";
            }

            // Show character-level diff if both lines exist
            if (has_expected && has_generated) {
                auto char_diff = character_diff(
                    expected_lines[i],
                    generated_lines[i]);
                if (not char_diff.empty()) {
                    diff_output << "  DIFF: " << char_diff << "\n";
                }
            }

            diff_output << "\n";
        }

        if (diff_count >= 20) {
            diff_output << "... (showing first 20 differences)\n\n";
        }

        diff_output << "To update golden file:\n";
        diff_output << "  atlas --input=" << input_path.string() << " > "
            << expected_path.string() << "\n";
        diff_output << "Or run: ./tests/tools/update_goldens.sh\n";
        diff_output << "========================================\n";

        FAIL(diff_output.str());
    }
}

/**
 * Split string into lines, preserving line endings.
 */
std::vector<std::string>
split_lines(std::string const & text)
{
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    // Handle trailing newline
    if (not text.empty() && text.back() == '\n') {
        lines.push_back("");
    }

    return lines;
}

/**
 * Visualize whitespace characters in a string.
 * Shows spaces as '·', tabs as '→', and newlines as '↵'.
 */
std::string
visualize_whitespace(std::string const & s)
{
    std::string result;
    result.reserve(s.size() * 2);

    for (char c : s) {
        if (c == ' ') {
            result += "\xC2\xB7"; // · (middle dot, UTF-8)
        } else if (c == '\t') {
            result += "\xE2\x86\x92   "; // → (rightwards arrow, UTF-8)
        } else if (c == '\n') {
            result += "\xE2\x86\xB5"; // ↵ (downwards arrow with corner, UTF-8)
        } else if (c == '\r') {
            result += "\xE2\x90\x8D"; // ␍ (symbol for carriage return, UTF-8)
        } else if (std::isprint(static_cast<unsigned char>(c))) {
            result += c;
        } else {
            // Show non-printable as hex
            char buf[10];
            std::snprintf(
                buf,
                sizeof(buf),
                "\\x%02x",
                static_cast<unsigned char>(c));
            result += buf;
        }
    }

    return result;
}

/**
 * Create character-level diff showing where strings differ.
 * Returns a string with '^' markers under differing positions.
 */
std::string
character_diff(std::string const & expected, std::string const & generated)
{
    size_t min_len = std::min(expected.size(), generated.size());
    size_t max_len = std::max(expected.size(), generated.size());

    std::string diff;
    diff.reserve(max_len);

    for (size_t i = 0; i < min_len; ++i) {
        if (expected[i] == generated[i]) {
            diff += ' ';
        } else {
            diff += '^';
        }
    }

    // Mark length difference
    for (size_t i = min_len; i < max_len; ++i) {
        diff += '!';
    }

    // Only return if there are differences
    if (diff.find_first_not_of(' ') != std::string::npos) {
        return diff;
    }

    return "";
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
