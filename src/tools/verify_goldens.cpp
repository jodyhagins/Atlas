// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasMain.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

namespace fs = std::filesystem;

namespace {

/**
 * Read entire file into string.
 */
std::string
read_file(fs::path const & path)
{
    std::ifstream file(path);
    if (not file) {
        throw std::runtime_error("Cannot open: " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    if (not file && not file.eof()) {
        throw std::runtime_error("Error reading: " + path.string());
    }
    return ss.str();
}

/**
 * Generate code from input file by calling atlas_main.
 *
 * Captures stdout to get the generated code.
 */
std::pair<int, std::string>
generate_from_input_file(fs::path const & input_path)
{
    // Build argument vector for atlas_main
    std::vector<std::string> arg_strings = {
        "atlas",
        "--input=" + input_path.string()};

    // Convert to char* array
    std::vector<char *> argv;
    for (auto & arg : arg_strings) {
        argv.push_back(arg.data());
    }

    // Redirect stdout to capture output
    std::ostringstream captured_output;
    auto old_cout = std::cout.rdbuf(captured_output.rdbuf());

    // Call atlas_main
    int exit_code = wjh::atlas::atlas_main(
        static_cast<int>(argv.size()),
        argv.data());

    // Restore stdout
    std::cout.rdbuf(old_cout);

    return {exit_code, captured_output.str()};
}

/**
 * Verify a single golden file pair.
 *
 * @return true if matches, false otherwise
 */
bool
verify_golden_file(fs::path const & input_path, bool verbose)
{
    auto expected_path = input_path;
    expected_path.replace_extension(".expected");

    if (not fs::exists(expected_path)) {
        std::cerr << "MISSING: " << expected_path.string() << std::endl;
        return false;
    }

    // Generate current output
    auto [exit_code, generated] = generate_from_input_file(input_path);

    if (exit_code != EXIT_SUCCESS) {
        std::cerr << "FAIL: " << input_path.filename().string()
            << " (atlas_main returned " << exit_code << ")" << std::endl;
        return false;
    }

    // Compare
    auto expected = read_file(expected_path);

    if (generated != expected) {
        std::cerr << "FAIL: " << input_path.filename().string() << std::endl;
        std::cerr << "  Run: diff <(atlas --input=\"" << input_path.string()
            << "\") \"" << expected_path.string() << "\"" << std::endl;
        return false;
    }

    if (verbose) {
        std::cout << "PASS: " << input_path.filename().string() << std::endl;
    }

    return true;
}

/**
 * Discover all .input files in golden directory.
 */
std::vector<fs::path>
discover_golden_files(fs::path const & golden_dir)
{
    std::vector<fs::path> input_files;

    if (not fs::exists(golden_dir)) {
        return input_files;
    }

    for (auto const & entry : fs::recursive_directory_iterator(golden_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".input") {
            input_files.push_back(entry.path());
        }
    }

    std::sort(input_files.begin(), input_files.end());
    return input_files;
}

} // anonymous namespace

int
main(int argc, char ** argv)
{
    try {
        bool verbose = false;
        fs::path golden_dir;

        // Parse arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--verbose" || arg == "-v") {
                verbose = true;
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: verify_goldens [OPTIONS] [GOLDEN_DIR]\n\n"
                    << "Verify golden files match current Atlas output.\n\n"
                    << "Options:\n"
                    << "  -v, --verbose    Show all files (not just failures)\n"
                    << "  -h, --help       Show this help message\n\n"
                    << "Arguments:\n"
                    << "  GOLDEN_DIR       Path to golden files directory\n"
                    << "                   (default: tests/fixtures/golden)\n";
                return EXIT_SUCCESS;
            } else if (not arg.empty() && arg[0] != '-') {
                golden_dir = arg;
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                return EXIT_FAILURE;
            }
        }

        // Default golden directory
        if (golden_dir.empty()) {
            // Try to find repository root
            auto cwd = fs::current_path();

            // Look for tests/fixtures/golden from current directory
            if (fs::exists(cwd / "tests/fixtures/golden")) {
                golden_dir = cwd / "tests/fixtures/golden";
            } else {
                std::cerr << "Error: Cannot find golden files directory.\n"
                    << "Run from repository root or specify path.\n";
                return EXIT_FAILURE;
            }
        }

        if (not fs::exists(golden_dir)) {
            std::cerr << "Error: Golden directory not found: "
                << golden_dir.string() << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Verifying golden files..." << std::endl;
        std::cout << "Golden dir: " << golden_dir.string() << std::endl;
        std::cout << std::endl;

        auto input_files = discover_golden_files(golden_dir);

        if (input_files.empty()) {
            std::cout << "No golden files found." << std::endl;
            return EXIT_SUCCESS;
        }

        int passed = 0;
        int failed = 0;

        for (auto const & input_path : input_files) {
            if (verify_golden_file(input_path, verbose)) {
                ++passed;
            } else {
                ++failed;
            }
        }

        std::cout << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;

        if (failed > 0) {
            std::cout << std::endl;
            std::cout << "Golden files don't match current output!"
                << std::endl;
            std::cout << "If changes are intentional, run: "
                << "./tests/tools/update_goldens.sh" << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << std::endl;
        std::cout << "All golden files match ✓" << std::endl;
        return EXIT_SUCCESS;

    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
