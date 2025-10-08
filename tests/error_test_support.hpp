// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_C29D03282338498E82B7D8FD67765E9B
#define WJH_ATLAS_C29D03282338498E82B7D8FD67765E9B

#include "AtlasMain.hpp"
#include "TestUtilities.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

namespace fs = std::filesystem;

namespace wjh::atlas::testing {

/**
 * Result of calling atlas_main() expecting an error
 */
struct ErrorTestResult
{
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;

    [[nodiscard]]
    bool had_error() const
    {
        return exit_code != EXIT_SUCCESS;
    }
};

/**
 * Call atlas_main() expecting it to fail with an error
 *
 * Captures both stdout and stderr to check error messages.
 * Catches exceptions and treats them as errors (exit code 1).
 */
inline ErrorTestResult
call_atlas_expecting_error(std::vector<std::string> const & args)
{
    // Build argument vector
    std::vector<char *> argv;
    std::vector<std::string> arg_storage = args; // Keep alive

    for (auto & arg : arg_storage) {
        argv.push_back(arg.data());
    }

    // Redirect stdout
    std::ostringstream captured_stdout;
    auto old_cout = std::cout.rdbuf(captured_stdout.rdbuf());

    // Redirect stderr
    std::ostringstream captured_stderr;
    auto old_cerr = std::cerr.rdbuf(captured_stderr.rdbuf());

    int exit_code = EXIT_SUCCESS;

    try {
        // Call atlas_main
        exit_code = atlas_main(static_cast<int>(argv.size()), argv.data());
    } catch (std::exception const & e) {
        // Exception = error condition
        exit_code = EXIT_FAILURE;
        captured_stderr << "Exception: " << e.what() << "\n";
    } catch (...) {
        // Unknown exception = error condition
        exit_code = EXIT_FAILURE;
        captured_stderr << "Unknown exception\n";
    }

    // Restore streams
    std::cout.rdbuf(old_cout);
    std::cerr.rdbuf(old_cerr);

    return ErrorTestResult{
        .exit_code = exit_code,
        .stdout_output = captured_stdout.str(),
        .stderr_output = captured_stderr.str()};
}

/**
 * Call atlas_main() with an input file, expecting an error
 */
inline ErrorTestResult
test_input_file_error(fs::path const & input_file)
{
    return call_atlas_expecting_error(
        {"atlas", "--input=" + input_file.string()});
}

/**
 * Create a temporary input file with given content and test it
 */
inline ErrorTestResult
test_input_content_error(std::string const & content)
{
    TemporaryDirectory temp_dir;
    auto input_file = temp_dir.path() / "test.input";
    write_file(input_file, content);
    return test_input_file_error(input_file);
}

/**
 * Create a temporary interaction file and test it
 *
 * Interaction files don't use [type] markers and require --interactions=true
 */
inline ErrorTestResult
test_interaction_content_error(std::string const & content)
{
    TemporaryDirectory temp_dir;
    auto input_file = temp_dir.path() / "test.interaction";
    write_file(input_file, content);
    return call_atlas_expecting_error(
        {"atlas", "--input=" + input_file.string(), "--interactions=true"});
}

} // namespace wjh::atlas::testing

#endif // WJH_ATLAS_C29D03282338498E82B7D8FD67765E9B
