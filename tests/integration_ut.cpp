// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "StrongTypeGenerator.hpp"
#include "doctest.hpp"
#include "test_common.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace wjh::atlas;

namespace {

// Helper to write generated code to a file, compile it, and check if it
// compiles
bool
compile_and_test_code(std::string const & code)
{
    namespace fs = std::filesystem;

    // Create a temporary directory for our test
    auto temp_dir = fs::temp_directory_path() /
        ("atlas_test_" + std::to_string(::getpid()));
    fs::create_directories(temp_dir);

    // Write the generated header
    auto header_path = temp_dir / "generated.hpp";
    {
        std::ofstream out(header_path);
        out << code;
    }

    // Write a simple test program that uses the generated type
    auto test_path = temp_dir / "test.cpp";
    {
        std::ofstream out(test_path);
        out << "#include \"generated.hpp\"\n";
        out << "int main() { return 0; }\n";
    }

    // Use the common compiler finder
    std::string compiler = wjh::atlas::test::find_working_compiler();

    std::string compile_cmd = compiler + " -std=c++20 -I" + temp_dir.string() +
        " -c " + test_path.string() + " -o " + (temp_dir / "test.o").string() +
        " 2>&1";

    int result = std::system(compile_cmd.c_str());

    // Clean up
    fs::remove_all(temp_dir);

    return result == 0;
}

TEST_SUITE("Integration Tests")
{
    TEST_CASE("Generated Code Compilation")
    {
        SUBCASE("basic int wrapper compiles") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "IntWrapper",
                .description = "strong int"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("arithmetic operators compile") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Number",
                .description = "strong int; +, -, *, /"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("comparison operators compile") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Comparable",
                .description = "strong int; ==, !=, <, <=, >, >="};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("spaceship operator compiles") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Ordered",
                .description = "strong int; <=>"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("iostream operators compile") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Streamable",
                .description = "strong int; in, out"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("class with private members compiles") {
            StrongTypeDescription desc{
                .kind = "class",
                .type_namespace = "test",
                .type_name = "Private",
                .description = "strong int"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("complex type with std::string compiles") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "StringWrapper",
                .description = "strong std::string; ==, !=, out"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("nested namespace compiles") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "a::b::c",
                .type_name = "Nested",
                .description = "strong int"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("pointer-like operators compile") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "PointerLike",
                .description = "strong int; @, &of, ->"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("callable operators compile") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Callable",
                .description = "strong int; (), (&)"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }

        SUBCASE("all operators combined compile") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Everything",
                .description = "strong int; +, -, *, /, ==, !=, <, <=>, ++, "
                               "--, bool, out"};

            auto code = generate_strong_type(desc);
            CHECK(compile_and_test_code(code));
        }
    }

    TEST_CASE("Generated Code Structure")
    {
        SUBCASE("header guard is unique") {
            StrongTypeDescription desc1{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Type1",
                .description = "strong int"};

            StrongTypeDescription desc2{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "Type2",
                .description = "strong int"};

            auto code1 = generate_strong_type(desc1);
            auto code2 = generate_strong_type(desc2);

            // Extract header guards
            auto find_guard = [](std::string const & code) {
                auto pos = code.find("#ifndef ");
                if (pos == std::string::npos) {
                    return std::string{};
                }
                pos += 8; // strlen("#ifndef ")
                auto end = code.find('\n', pos);
                return code.substr(pos, end - pos);
            };

            auto guard1 = find_guard(code1);
            auto guard2 = find_guard(code2);

            CHECK(guard1 != guard2);
            CHECK(not guard1.empty());
            CHECK(not guard2.empty());
        }

        SUBCASE("generated code includes copyright notice") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("AUTOMATICALLY GENERATED") != std::string::npos);
            CHECK(code.find("DO NOT EDIT") != std::string::npos);
        }

        SUBCASE("generated code documents parameters") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "my::namespace",
                .type_name = "MyType",
                .description = "strong double; +, -"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("kind: struct") != std::string::npos);
            CHECK(
                code.find("type_namespace: my::namespace") !=
                std::string::npos);
            CHECK(code.find("type_name: MyType") != std::string::npos);
            CHECK(
                code.find("description: strong double; +, -") !=
                std::string::npos);
        }

        SUBCASE("struct generates public members") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int"};

            auto code = generate_strong_type(desc);
            // Should not have explicit public: specifier for struct
            size_t struct_pos = code.find("struct TestType");
            size_t public_pos = code.find("public:");
            // public: should either not exist, or come after struct definition
            // starts
            CHECK((public_pos == std::string::npos || public_pos > struct_pos));
        }

        SUBCASE("class generates private members with public section") {
            StrongTypeDescription desc{
                .kind = "class",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("class TestType") != std::string::npos);
            CHECK(code.find("public:") != std::string::npos);
        }
    }

    TEST_CASE("Standard Library Type Detection")
    {
        SUBCASE("std::string includes string header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong std::string"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <string>") != std::string::npos);
        }

        SUBCASE("std::vector includes vector header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong std::vector<int>"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <vector>") != std::string::npos);
        }

        SUBCASE("std::optional includes optional header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong std::optional<int>"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <optional>") != std::string::npos);
        }

        SUBCASE("std::chrono includes chrono header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong std::chrono::seconds"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <chrono>") != std::string::npos);
        }

        SUBCASE("explicit include is added") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; #<custom/header.hpp>"};

            auto code = generate_strong_type(desc);
            CHECK(
                code.find("#include <custom/header.hpp>") != std::string::npos);
        }

        SUBCASE("quoted include is converted") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; #'my/header.hpp'"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include \"my/header.hpp\"") != std::string::npos);
        }
    }

    TEST_CASE("Operator Code Generation")
    {
        SUBCASE("spaceship operator includes compare header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; <=>"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <compare>") != std::string::npos);
            CHECK(code.find("operator <=>") != std::string::npos);
        }

        SUBCASE("out operator includes ostream header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; out"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <ostream>") != std::string::npos);
            CHECK(code.find("operator <<") != std::string::npos);
        }

        SUBCASE("in operator includes istream header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; in"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <istream>") != std::string::npos);
            CHECK(code.find("operator >>") != std::string::npos);
        }

        SUBCASE("address-of operator includes memory header") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; &of"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <memory>") != std::string::npos);
            CHECK(code.find("std::addressof") != std::string::npos);
        }

        SUBCASE("callable operator includes utility and functional headers") {
            StrongTypeDescription desc{
                .kind = "struct",
                .type_namespace = "test",
                .type_name = "TestType",
                .description = "strong int; (&)"};

            auto code = generate_strong_type(desc);
            CHECK(code.find("#include <utility>") != std::string::npos);
            CHECK(code.find("#include <functional>") != std::string::npos);
            CHECK(code.find("std::invoke") != std::string::npos);
        }
    }
}

} // anonymous namespace
