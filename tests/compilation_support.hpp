// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_EBBE79C64640467FADB97E3D9F5B97D2
#define WJH_ATLAS_EBBE79C64640467FADB97E3D9F5B97D2

#include "TestUtilities.hpp"
#include "test_common.hpp"

#include "atlas/AtlasMain.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace wjh::atlas::testing::compilation {

// Compilation result
struct CompileResult
{
    bool success;
    int exit_code;
    std::string output;
    std::string executable_path;
};

// Helper to execute a command and capture output
struct ExecResult
{
    int exit_code;
    std::string output;
};

// Note: This is only used for spawning the compiler and running the compiled
// test, NOT for running atlas. Code generation uses atlas_main() directly.
inline ExecResult
exec_command(std::string const & cmd)
{
    ExecResult result;
    FILE * pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result.output += buffer;
        }
        int status = pclose(pipe);
        // Extract actual exit code from wait status
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else {
            result.exit_code = -1;
        }
    } else {
        result.exit_code = -1;
    }
    return result;
}

// Test helper class
class CompilationTester
{
    fs::path temp_dir_;
    int counter_ = 0;

public:
    // Check if a C++ standard is supported by the compiler
    // Uses static cache so each standard is only checked once
    static bool is_cpp_standard_supported(std::string const & cpp_standard)
    {
        static std::map<std::string, bool> cache;

        auto it = cache.find(cpp_standard);
        if (it != cache.end()) {
            return it->second;
        }

        // Create minimal test file
        auto temp_dir = fs::temp_directory_path() /
            ("atlas_std_check_" + std::to_string(::getpid()));
        fs::create_directories(temp_dir);

        auto test_file = temp_dir / "test.cpp";
        std::ofstream out(test_file);
        // Check that __cplusplus is actually set to the expected value
        // not just that the compiler accepts the flag
        if (cpp_standard == "c++23" || cpp_standard == "c++2b") {
            out << "static_assert(__cplusplus >= 202302L, \"C++23 "
                   "required\");\n";
        } else if (cpp_standard == "c++20" || cpp_standard == "c++2a") {
            out << "static_assert(__cplusplus >= 202002L, \"C++20 "
                   "required\");\n";
        } else if (cpp_standard == "c++17") {
            out << "static_assert(__cplusplus >= 201703L, \"C++17 "
                   "required\");\n";
        } else if (cpp_standard == "c++14") {
            out << "static_assert(__cplusplus >= 201402L, \"C++14 "
                   "required\");\n";
        } else if (cpp_standard == "c++11") {
            out << "static_assert(__cplusplus >= 201103L, \"C++11 "
                   "required\");\n";
        }
        out << "int main() { return 0; }\n";
        out.close();

        // Try to compile with the requested standard
        auto exe_path = temp_dir / "test";
        std::ostringstream cmd;
        cmd << wjh::atlas::test::find_working_compiler()
            << " -std=" << cpp_standard << " " << test_file.string() << " -o "
            << exe_path.string() << " 2>&1";

        auto result = exec_command(cmd.str());

        // Clean up
        std::error_code ec;
        fs::remove_all(temp_dir, ec);

        // Cache and return result
        bool supported = (result.exit_code == 0);
        cache[cpp_standard] = supported;
        return supported;
    }

    CompilationTester()
    {
        temp_dir_ = fs::temp_directory_path() /
            ("atlas_compile_test_" + std::to_string(::getpid()));
        fs::create_directories(temp_dir_);
    }

    ~CompilationTester()
    {
        std::error_code ec;
        fs::remove_all(temp_dir_, ec);
    }

    // Write a file to the temp directory (useful for custom headers)
    void write_temp_file(
        std::string const & filename,
        std::string const & content)
    {
        write_file(temp_dir_ / filename, content);
    }

    // Generate header from description string, compile test, return result
    CompileResult compile_and_run(
        std::string const & atlas_description,
        std::string const & test_code,
        std::string cpp_standard = "c++20")
    {
        auto test_id = ++counter_;

        // Create input file with atlas description
        auto input_path = temp_dir_ /
            ("input_" + std::to_string(test_id) + ".txt");
        write_file(input_path, atlas_description);

        // Generate header by calling atlas_main() directly
        auto header_path = temp_dir_ /
            ("type_" + std::to_string(test_id) + ".hpp");
        {
            std::vector<std::string> arg_strings = {
                "atlas",
                "--input=" + input_path.string(),
                "--output=" + header_path.string()};

            std::vector<char *> argv;
            for (auto & arg : arg_strings) {
                argv.push_back(arg.data());
            }

            // Call atlas_main to generate the header
            int exit_code = wjh::atlas::atlas_main(
                static_cast<int>(argv.size()),
                argv.data());

            if (exit_code != EXIT_SUCCESS) {
                CompileResult result;
                result.success = false;
                result.exit_code = exit_code;
                result.output = "atlas_main failed to generate header";
                return result;
            }
        }

        // Write test code to the same temp directory
        auto test_path = temp_dir_ /
            ("test_" + std::to_string(test_id) + ".cpp");
        {
            std::ofstream test(test_path);
            test << "#include \"" << header_path.filename().string() << "\"\n";
            test << test_code;
        }

        // Compile in the temp directory where the generated header is
        auto exe_path = temp_dir_ / ("test_" + std::to_string(test_id));
        std::ostringstream compile_cmd;
        compile_cmd << "cd " << temp_dir_ << " && ";
        compile_cmd << wjh::atlas::test::find_working_compiler()
            << " -std=" << cpp_standard << " ";
        compile_cmd << "-I. -o " << exe_path.filename().string() << " ";
        compile_cmd << test_path.filename().string() << " 2>&1";

        auto compile_output = exec_command(compile_cmd.str());

        CompileResult result;
        result.success = (compile_output.exit_code == 0);
        result.output = compile_output.output;
        result.exit_code = compile_output.exit_code;
        result.executable_path = exe_path.string();

        // If compilation succeeded, run the test
        if (result.success) {
            auto run_output = exec_command(exe_path.string());
            result.success = (run_output.exit_code == 0);
            result.exit_code = run_output.exit_code;
            result.output = run_output.output;
        }

        return result;
    }

    // Generate types and interactions headers, compile test, return result
    CompileResult compile_and_run_with_interactions(
        std::string const & types_description,
        std::string const & interactions_description,
        std::string const & test_code,
        std::string cpp_standard = "c++20")
    {
        auto test_id = ++counter_;

        // Create input files
        auto types_input_path = temp_dir_ /
            ("types_input_" + std::to_string(test_id) + ".txt");
        write_file(types_input_path, types_description);

        auto interactions_input_path = temp_dir_ /
            ("interactions_input_" + std::to_string(test_id) + ".txt");
        write_file(interactions_input_path, interactions_description);

        // Generate types header
        auto types_header_path = temp_dir_ /
            ("types_" + std::to_string(test_id) + ".hpp");
        {
            std::vector<std::string> arg_strings = {
                "atlas",
                "--input=" + types_input_path.string(),
                "--output=" + types_header_path.string()};

            std::vector<char *> argv;
            for (auto & arg : arg_strings) {
                argv.push_back(arg.data());
            }

            int exit_code = wjh::atlas::atlas_main(
                static_cast<int>(argv.size()),
                argv.data());

            if (exit_code != EXIT_SUCCESS) {
                CompileResult result;
                result.success = false;
                result.exit_code = exit_code;
                result.output = "atlas_main failed to generate types header";
                return result;
            }
        }

        // Generate interactions header
        auto interactions_header_path = temp_dir_ /
            ("interactions_" + std::to_string(test_id) + ".hpp");
        {
            std::vector<std::string> arg_strings = {
                "atlas",
                "--interactions=true",
                "--input=" + interactions_input_path.string(),
                "--output=" + interactions_header_path.string()};

            std::vector<char *> argv;
            for (auto & arg : arg_strings) {
                argv.push_back(arg.data());
            }

            int exit_code = wjh::atlas::atlas_main(
                static_cast<int>(argv.size()),
                argv.data());

            if (exit_code != EXIT_SUCCESS) {
                CompileResult result;
                result.success = false;
                result.exit_code = exit_code;
                result.output =
                    "atlas_main failed to generate interactions header";
                return result;
            }
        }

        // Write test code to the same temp directory
        auto test_path = temp_dir_ /
            ("test_" + std::to_string(test_id) + ".cpp");
        {
            std::ofstream test(test_path);
            test << "#include \"" << types_header_path.filename().string()
                << "\"\n";
            test << "#include \""
                << interactions_header_path.filename().string() << "\"\n";
            test << test_code;
        }

        // Compile in the temp directory
        auto exe_path = temp_dir_ / ("test_" + std::to_string(test_id));
        std::ostringstream compile_cmd;
        compile_cmd << "cd " << temp_dir_ << " && ";
        compile_cmd << wjh::atlas::test::find_working_compiler()
            << " -std=" << cpp_standard << " ";
        compile_cmd << "-I. -o " << exe_path.filename().string() << " ";
        compile_cmd << test_path.filename().string() << " 2>&1";

        auto compile_output = exec_command(compile_cmd.str());

        CompileResult result;
        result.success = (compile_output.exit_code == 0);
        result.output = compile_output.output;
        result.exit_code = compile_output.exit_code;
        result.executable_path = exe_path.string();

        // If compilation succeeded, run the test
        if (result.success) {
            auto run_output = exec_command(exe_path.string());
            result.success = (run_output.exit_code == 0);
            result.exit_code = run_output.exit_code;
            result.output = run_output.output;
        }

        return result;
    }
};

} // namespace wjh::atlas::testing::compilation

#endif // WJH_ATLAS_EBBE79C64640467FADB97E3D9F5B97D2
