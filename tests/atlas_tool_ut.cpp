// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"
#include "test_common.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string
build_dir()
{
    if (auto p = ::getenv("BUILD_DIR"); p != nullptr) {
        return p;
    }
    return std::filesystem::current_path();
}

class AtlasTester
{
public:
    std::filesystem::path build_dir_;
    std::filesystem::path temp_dir_;

public:
    AtlasTester()
    {
        build_dir_ = build_dir();
        temp_dir_ = std::filesystem::temp_directory_path() /
            ("atlas_tool_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(temp_dir_);
    }

    ~AtlasTester() { std::filesystem::remove_all(temp_dir_); }

    struct AtlasResult
    {
        bool success;
        std::string output;
        std::string generated_code;
    };

    AtlasResult run_atlas(
        std::string const & kind,
        std::string const & ns,
        std::string const & name,
        std::string const & description)
    {
        std::ostringstream cmd;
        cmd << "cd " << build_dir_ << " && ";
        cmd << "./bin/atlas --kind=" << kind << " --namespace=" << ns
            << " --name=" << name << " --description=\"" << description
            << "\" 2>&1";

        AtlasResult result;
        FILE * pipe = popen(cmd.str().c_str(), "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result.output += buffer;
            }
            int exit_code = pclose(pipe);
            result.success = (exit_code == 0);

            // Extract generated code (everything after the debug output)
            auto pos = result.output.find("#ifndef");
            if (pos != std::string::npos) {
                result.generated_code = result.output.substr(pos);
                // Clean up any trailing debug output after the #endif
                auto endif_pos = result.generated_code.rfind("#endif");
                if (endif_pos != std::string::npos) {
                    auto line_end = result.generated_code.find('\n', endif_pos);
                    if (line_end != std::string::npos) {
                        result.generated_code = result.generated_code.substr(
                            0,
                            line_end + 1);
                    }
                }
            }
        }

        return result;
    }

    bool test_generated_code_compiles(std::string const & generated_code)
    {
        auto header_path = temp_dir_ / "test.hpp";
        auto main_path = temp_dir_ / "main.cpp";
        auto exe_path = temp_dir_ / "test";

        // Write header
        {
            std::ofstream header(header_path);
            header << generated_code;
        }

        // Write simple test main
        {
            std::ofstream main_file(main_path);
            main_file << "#include \"test.hpp\"\nint main() { return 0; }\n";
        }

        // Compile
        std::ostringstream cmd;
        cmd << "cd " << temp_dir_ << " && ";
        cmd << wjh::atlas::test::find_working_compiler()
            << " -std=c++20 -I. -o test main.cpp 2>/dev/null";

        int result = system(cmd.str().c_str());
        return (result == 0);
    }
};

TEST_SUITE("Atlas Tool Integration Tests")
{
    TEST_CASE("Command Line Interface")
    {
        SUBCASE("help command works") {
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ./bin/atlas --help 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                pclose(pipe);
            }

            CHECK(
                output.find("Atlas Strong Type Generator") !=
                std::string::npos);
            CHECK(output.find("--kind=") != std::string::npos);
            CHECK(output.find("EXAMPLES:") != std::string::npos);
        }

        SUBCASE("error handling for missing arguments") {
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ./bin/atlas --kind=struct 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int exit_code = pclose(pipe);
                CHECK(exit_code != 0); // Should fail
            }

            CHECK(output.find("Error:") != std::string::npos);
            CHECK(
                output.find("Missing required arguments") != std::string::npos);
        }

        SUBCASE("error handling for invalid arguments") {
            std::ostringstream cmd;
            cmd << "cd " << build_dir()
                << "  && ./bin/atlas --invalid-arg=value 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int exit_code = pclose(pipe);
                CHECK(exit_code != 0); // Should fail
            }

            CHECK(output.find("Error:") != std::string::npos);
        }
    }

    TEST_CASE("Atlas Tool Basic Functionality")
    {
        AtlasTester tester;

        SUBCASE("generates basic struct") {
            auto result =
                tester.run_atlas("struct", "test", "MyInt", "strong int");

            CHECK(result.success);
            CHECK(
                result.generated_code.find("struct MyInt") !=
                std::string::npos);
            CHECK(
                result.generated_code.find("int value;") != std::string::npos);
            CHECK(tester.test_generated_code_compiles(result.generated_code));
        }

        SUBCASE("generates class with operators") {
            auto result = tester.run_atlas(
                "class",
                "example",
                "Number",
                "strong int; +, -, ==, !=");

            CHECK(result.success);
            CHECK(
                result.generated_code.find("class Number") !=
                std::string::npos);
            CHECK(result.generated_code.find("public:") != std::string::npos);
            CHECK(
                result.generated_code.find("operator +") != std::string::npos);
            CHECK(
                result.generated_code.find("operator -") != std::string::npos);
            CHECK(
                result.generated_code.find("operator ==") != std::string::npos);
            CHECK(
                result.generated_code.find("operator !=") != std::string::npos);
            CHECK(tester.test_generated_code_compiles(result.generated_code));
        }

        SUBCASE("generates type with stream operators") {
            auto result = tester.run_atlas(
                "struct",
                "io",
                "Printable",
                "strong std::string; in, out");

            CHECK(result.success);
            CHECK(
                result.generated_code.find("#include <istream>") !=
                std::string::npos);
            CHECK(
                result.generated_code.find("#include <ostream>") !=
                std::string::npos);
            CHECK(
                result.generated_code.find("#include <string>") !=
                std::string::npos);
            CHECK(
                result.generated_code.find("operator <<") != std::string::npos);
            CHECK(
                result.generated_code.find("operator >>") != std::string::npos);
            CHECK(tester.test_generated_code_compiles(result.generated_code));
        }

        SUBCASE("generates complex type with many operators") {
            auto result = tester.run_atlas(
                "struct",
                "advanced",
                "CompleteType",
                "strong int; +, -, *, ==, !=, <, <=, >, >=, ++, bool, out");

            CHECK(result.success);
            CHECK(
                result.generated_code.find("struct CompleteType") !=
                std::string::npos);
            CHECK(
                result.generated_code.find("operator +") != std::string::npos);
            CHECK(
                result.generated_code.find("operator *") != std::string::npos);
            CHECK(
                result.generated_code.find("operator ==") != std::string::npos);
            CHECK(
                result.generated_code.find("operator <") != std::string::npos);
            CHECK(
                result.generated_code.find("operator ++") != std::string::npos);
            CHECK(
                result.generated_code.find("explicit operator bool") !=
                std::string::npos);
            CHECK(
                result.generated_code.find("operator <<") != std::string::npos);
            CHECK(tester.test_generated_code_compiles(result.generated_code));
        }
    }

    TEST_CASE("Generated Code Semantic Correctness")
    {
        AtlasTester tester;

        SUBCASE("arithmetic operators work correctly") {
            auto result = tester.run_atlas(
                "struct",
                "test",
                "Number",
                "strong int; +, -");

            CHECK(result.success);
            CHECK(tester.test_generated_code_compiles(result.generated_code));

            // Create a more comprehensive test
            auto header_path = tester.temp_dir_ / "number.hpp";
            auto test_path = tester.temp_dir_ / "test_arithmetic.cpp";

            {
                std::ofstream header(header_path);
                header << result.generated_code;
            }

            {
                std::ofstream test_file(test_path);
                test_file << R"(
#include "number.hpp"
#include <cassert>

int main() {
    test::Number a{10};
    test::Number b{5};

    // Test addition
    auto sum = a + b;
    assert(static_cast<int const&>(sum) == 15);

    // Test subtraction
    auto diff = a - b;
    assert(static_cast<int const&>(diff) == 5);

    // Test compound assignment
    a += b;
    assert(static_cast<int const&>(a) == 15);

    return 0;
}
)";
            }

            std::ostringstream cmd;
            cmd << "cd " << tester.temp_dir_ << " && ";
            cmd << wjh::atlas::test::find_working_compiler()
                << " -std=c++20 -I. -o test_arithmetic test_arithmetic.cpp "
                   "2>/dev/null && ./test_arithmetic 2>/dev/null";

            int compile_result = system(cmd.str().c_str());
            CHECK(compile_result == 0);
        }

        SUBCASE("comparison operators work correctly") {
            auto result = tester.run_atlas(
                "struct",
                "test",
                "Comparable",
                "strong int; ==, !=, <, >");

            CHECK(result.success);
            CHECK(tester.test_generated_code_compiles(result.generated_code));

            auto header_path = tester.temp_dir_ / "comparable.hpp";
            auto test_path = tester.temp_dir_ / "test_comparison.cpp";

            {
                std::ofstream header(header_path);
                header << result.generated_code;
            }

            {
                std::ofstream test_file(test_path);
                test_file << R"(
#include "comparable.hpp"
#include <cassert>

int main() {
    test::Comparable a{10};
    test::Comparable b{10};
    test::Comparable c{20};

    // Test equality
    assert(a == b);
    assert(!(a == c));

    // Test inequality
    assert(a != c);
    assert(!(a != b));

    // Test less/greater
    assert(a < c);
    assert(c > a);

    return 0;
}
)";
            }

            std::ostringstream cmd;
            cmd << "cd " << tester.temp_dir_ << " && ";
            cmd << wjh::atlas::test::find_working_compiler()
                << " -std=c++20 -I. -o test_comparison test_comparison.cpp "
                   "2>/dev/null && ./test_comparison 2>/dev/null";

            int result_code = system(cmd.str().c_str());
            CHECK(result_code == 0);
        }
    }

    TEST_CASE("File Input/Output Functionality")
    {
        AtlasTester tester;

        SUBCASE("generate from input file") {
            // Create input file
            auto input_path = tester.temp_dir_ / "types.txt";
            {
                std::ofstream input(input_path);
                input << "# Strong types configuration\n";
                input << "guard_prefix=MY_TYPES\n\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=math\n";
                input << "name=Distance\n";
                input << "description=strong int; +, -, ==, !=\n\n";
                input << "[type]\n";
                input << "kind=class\n";
                input << "namespace=util\n";
                input << "name=Counter\n";
                input << "description=strong int; ++, --, bool\n";
            }

            // Run atlas with input file
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ";
            cmd << "./bin/atlas --input=" << input_path << " 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int exit_code = pclose(pipe);
                CHECK(exit_code == 0);
            }

            // Verify single header guard with prefix + SHA
            CHECK(output.find("#ifndef MY_TYPES_") != std::string::npos);
            CHECK(output.find("#define MY_TYPES_") != std::string::npos);

            // Verify both types are generated
            CHECK(output.find("struct Distance") != std::string::npos);
            CHECK(output.find("namespace math") != std::string::npos);
            CHECK(output.find("class Counter") != std::string::npos);
            CHECK(output.find("namespace util") != std::string::npos);
        }

        SUBCASE("generate to output file") {
            auto output_path = tester.temp_dir_ / "output.hpp";

            // Run atlas with output file
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ";
            cmd << "./bin/atlas --kind=struct --namespace=test --name=MyInt "
                << "--description=\"strong int\" "
                << "--output=" << output_path << " 2>&1";

            int exit_code = system(cmd.str().c_str());
            CHECK(exit_code == 0);

            // Verify output file exists and has content
            std::ifstream output_file(output_path);
            CHECK(output_file.good());

            std::string content(
                (std::istreambuf_iterator<char>(output_file)),
                std::istreambuf_iterator<char>());
            CHECK(content.find("struct MyInt") != std::string::npos);
            CHECK(content.find("namespace test") != std::string::npos);
        }

        SUBCASE("input and output file together") {
            auto input_path = tester.temp_dir_ / "input.txt";
            auto output_path = tester.temp_dir_ / "result.hpp";

            // Create input file
            {
                std::ofstream input(input_path);
                input << "guard_prefix=GEO_TYPES\n\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=geo\n";
                input << "name=Point\n";
                input << "description=strong int; ==, !=\n";
            }

            // Run atlas
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ";
            cmd << "./bin/atlas --input=" << input_path
                << " --output=" << output_path << " 2>&1";

            int exit_code = system(cmd.str().c_str());
            CHECK(exit_code == 0);

            // Verify output
            std::ifstream output_file(output_path);
            CHECK(output_file.good());

            std::string content(
                (std::istreambuf_iterator<char>(output_file)),
                std::istreambuf_iterator<char>());
            CHECK(content.find("#ifndef GEO_TYPES_") != std::string::npos);
            CHECK(content.find("struct Point") != std::string::npos);
            CHECK(content.find("namespace geo") != std::string::npos);
        }

        SUBCASE("input file with comments and empty lines") {
            auto input_path = tester.temp_dir_ / "commented.txt";
            {
                std::ofstream input(input_path);
                input << "# This is a comment\n";
                input << "guard_prefix=TEST_TYPES\n";
                input << "\n";
                input << "# First type\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=test\n";
                input << "name=Type1\n";
                input << "description=strong int\n";
                input << "\n";
                input << "# Another comment\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=test\n";
                input << "name=Type2\n";
                input << "description=strong double\n";
            }

            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ";
            cmd << "./bin/atlas --input=" << input_path << " 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int exit_code = pclose(pipe);
                CHECK(exit_code == 0);
            }

            CHECK(output.find("#ifndef TEST_TYPES_") != std::string::npos);
            CHECK(output.find("struct Type1") != std::string::npos);
            CHECK(output.find("struct Type2") != std::string::npos);
        }

        SUBCASE("error handling for missing input file") {
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ";
            cmd << "./bin/atlas --input=nonexistent.txt 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int exit_code = pclose(pipe);
                CHECK(exit_code != 0);
            }

            CHECK(output.find("Error:") != std::string::npos);
        }

        SUBCASE("file mode with multiple hash specializations") {
            // Create input file with multiple types that have hash support
            auto input_path = tester.temp_dir_ / "hash_types.txt";
            {
                std::ofstream input(input_path);
                input << "guard_prefix=HASH_TYPES\n\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=ids\n";
                input << "name=UserId\n";
                input << "description=strong int; ==, no-constexpr-hash\n\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=ids\n";
                input << "name=ProductId\n";
                input << "description=strong unsigned; ==, no-constexpr-hash\n\n";
                input << "[type]\n";
                input << "kind=struct\n";
                input << "namespace=strings\n";
                input << "name=Label\n";
                input << "description=strong std::string; ==, no-constexpr-hash\n";
            }

            // Run atlas with input file
            std::ostringstream cmd;
            cmd << "cd " << build_dir() << " && ";
            cmd << "./bin/atlas --input=" << input_path << " 2>&1";

            std::string output;
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                }
                int exit_code = pclose(pipe);
                CHECK(exit_code == 0);
            }

            // Verify single header guard
            CHECK(output.find("#ifndef HASH_TYPES_") != std::string::npos);
            CHECK(output.find("#define HASH_TYPES_") != std::string::npos);

            // Verify all types are generated
            CHECK(output.find("struct UserId") != std::string::npos);
            CHECK(output.find("struct ProductId") != std::string::npos);
            CHECK(output.find("struct Label") != std::string::npos);

            // Verify all hash specializations exist
            CHECK(
                output.find("struct std::hash<ids::UserId>") !=
                std::string::npos);
            CHECK(
                output.find("struct std::hash<ids::ProductId>") !=
                std::string::npos);
            CHECK(
                output.find("struct std::hash<strings::Label>") !=
                std::string::npos);

            // Verify each hash specialization comes IMMEDIATELY after its
            // respective namespace closure
            auto userid_struct_end = output.find("} // namespace ids");
            auto userid_hash = output.find("struct std::hash<ids::UserId>");
            CHECK(userid_struct_end != std::string::npos);
            CHECK(userid_hash != std::string::npos);
            CHECK(userid_struct_end < userid_hash);

            // Check UserId hash comes before ProductId struct
            auto productid_struct = output.find("struct ProductId");
            CHECK(userid_hash < productid_struct);

            // ProductId hash should follow its namespace closure
            auto productid_ns_close = output.find(
                "} // namespace ids",
                userid_struct_end + 1);
            auto productid_hash = output.find(
                "struct std::hash<ids::ProductId>");
            CHECK(productid_ns_close < productid_hash);

            // Label hash should follow its namespace closure
            auto label_ns_close = output.find("} // namespace strings");
            auto label_hash = output.find("struct std::hash<strings::Label>");
            CHECK(label_ns_close < label_hash);

            // Verify NOTICE banner appears only once at the top
            auto first_notice = output.find("// NOTICE");
            auto second_notice = output.find("// NOTICE", first_notice + 1);
            CHECK(second_notice == std::string::npos);
        }
    }
}

} // anonymous namespace
