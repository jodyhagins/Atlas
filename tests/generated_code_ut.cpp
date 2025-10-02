// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "StrongTypeGenerator.hpp"
#include "doctest.hpp"
#include "rapidcheck.hpp"
#include "test_common.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace wjh::atlas::testing;
using wjh::atlas::testing::rc::check;

namespace {

using namespace wjh::atlas;

// Helper to compile and test generated code
class CodeTester
{
    std::filesystem::path temp_dir_;
    int test_counter_ = 0;

public:
    CodeTester()
    {
        temp_dir_ = std::filesystem::temp_directory_path() /
            ("atlas_test_" + std::to_string(::getpid()));
        std::filesystem::create_directories(temp_dir_);
    }

    ~CodeTester() { std::filesystem::remove_all(temp_dir_); }

    struct CompileResult
    {
        bool success;
        std::string output;
        std::string executable_path;
    };

    CompileResult compile_and_test(
        std::string const & generated_header,
        std::string const & test_main)
    {
        auto test_id = ++test_counter_;
        auto header_path = temp_dir_ /
            ("strong_type_" + std::to_string(test_id) + ".hpp");
        auto main_path = temp_dir_ /
            ("test_main_" + std::to_string(test_id) + ".cpp");
        auto exe_path = temp_dir_ / ("test_exe_" + std::to_string(test_id));

        // Write header file
        {
            std::ofstream header(header_path);
            header << generated_header;
        }

        // Write test main
        {
            std::ofstream main_file(main_path);
            main_file << "#include \"" << header_path.filename().string()
                << "\"\n";
            main_file << "#include <iostream>\n";
            main_file << "#include <cassert>\n";
            main_file << "#include <type_traits>\n";
            main_file << test_main;
        }

        // Compile
        std::ostringstream cmd;
        cmd << "cd " << temp_dir_ << " && ";
        cmd << wjh::atlas::test::find_working_compiler()
            << " -std=c++20 -I. -o " << exe_path.filename().string() << " "
            << main_path.filename().string() << " 2>&1";

        auto result = system(cmd.str().c_str());

        CompileResult compile_result;
        compile_result.success = (result == 0);
        compile_result.executable_path = exe_path.string();

        if (compile_result.success) {
            // Run the executable and capture output
            std::ostringstream run_cmd;
            run_cmd << exe_path.string() << " 2>&1";

            FILE * pipe = popen(run_cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[128];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    compile_result.output += buffer;
                }
                int run_result = pclose(pipe);
                compile_result.success = (run_result == 0);
            }
        } else {
            // Capture compilation errors
            FILE * pipe = popen(cmd.str().c_str(), "r");
            if (pipe) {
                char buffer[128];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    compile_result.output += buffer;
                }
                pclose(pipe);
            }
        }

        return compile_result;
    }
};

auto
make_description(
    std::string kind = "struct",
    std::string type_namespace = "test",
    std::string type_name = "TestType",
    std::string description = "strong int",
    std::string default_value = "",
    std::string guard_prefix = "",
    std::string guard_separator = "_",
    bool upcase_guard = true)
{
    return StrongTypeDescription{
        .kind = std::move(kind),
        .type_namespace = std::move(type_namespace),
        .type_name = std::move(type_name),
        .description = std::move(description),
        .default_value = std::move(default_value),
        .guard_prefix = std::move(guard_prefix),
        .guard_separator = std::move(guard_separator),
        .upcase_guard = upcase_guard};
}

TEST_SUITE("Generated Code Compilation and Semantics")
{
    TEST_CASE("Basic Strong Type Compilation")
    {
        CodeTester tester;

        SUBCASE("simple int wrapper compiles") {
            auto desc =
                make_description("struct", "test", "MyInt", "strong int");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::MyInt x{42};
    test::MyInt y{10};

    // Test explicit cast
    int value = static_cast<int const&>(x);
    assert(value == 42);

    std::cout << "Basic int wrapper test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Basic int wrapper test passed") !=
                std::string::npos);
        }

        SUBCASE("string wrapper compiles") {
            auto desc = make_description(
                "class",
                "test",
                "MyString",
                "strong std::string; no-constexpr");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::MyString x{"hello"};
    test::MyString y{"world"};

    // Test explicit cast
    std::string const& value = static_cast<std::string const&>(x);
    assert(value == "hello");

    std::cout << "Basic string wrapper test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Basic string wrapper test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Arithmetic Operators")
    {
        CodeTester tester;

        SUBCASE("addition and subtraction") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; +, -");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Number x{10};
    test::Number y{5};

    // Test addition
    auto sum = x + y;
    assert(static_cast<int const&>(sum) == 15);

    // Test subtraction
    auto diff = x - y;
    assert(static_cast<int const&>(diff) == 5);

    // Test compound assignment
    x += y;
    assert(static_cast<int const&>(x) == 15);

    x -= y;
    assert(static_cast<int const&>(x) == 10);

    std::cout << "Arithmetic operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Arithmetic operators test passed") !=
                std::string::npos);
        }

        SUBCASE("unary operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; u+, u-, u~");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Number x{-5};

    // Test unary plus
    auto pos = +x;
    assert(static_cast<int const&>(pos) == -5);

    // Test unary minus
    auto neg = -x;
    assert(static_cast<int const&>(neg) == 5);

    test::Number bits{0x0F};
    // Test bitwise not
    auto inverted = ~bits;
    assert(static_cast<int const&>(inverted) == ~0x0F);

    std::cout << "Unary operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Unary operators test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Comparison Operators")
    {
        CodeTester tester;

        SUBCASE("equality and relational operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; ==, !=, <, <=, >, >=");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Number x{10};
    test::Number y{10};
    test::Number z{20};

    // Test equality
    assert(x == y);
    assert(!(x == z));

    // Test inequality
    assert(x != z);
    assert(!(x != y));

    // Test less than
    assert(x < z);
    assert(!(z < x));

    // Test less than or equal
    assert(x <= y);
    assert(x <= z);
    assert(!(z <= x));

    // Test greater than
    assert(z > x);
    assert(!(x > z));

    // Test greater than or equal
    assert(y >= x);
    assert(z >= x);
    assert(!(x >= z));

    std::cout << "Comparison operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Comparison operators test passed") !=
                std::string::npos);
        }

        SUBCASE("spaceship operator") {
            auto desc =
                make_description("struct", "test", "Number", "strong int; <=>");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Number x{10};
    test::Number y{20};
    test::Number z{10};

    // Test three-way comparison
    assert((x <=> y) < 0);
    assert((y <=> x) > 0);
    assert((x <=> z) == 0);

    std::cout << "Spaceship operator test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Spaceship operator test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Special Operators")
    {
        CodeTester tester;

        SUBCASE("increment and decrement") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int; ++, --");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Counter x{10};

    // Test prefix increment
    auto pre_inc = ++x;
    assert(static_cast<int const&>(x) == 11);
    assert(static_cast<int const&>(pre_inc) == 11);

    // Test postfix increment
    auto post_inc = x++;
    assert(static_cast<int const&>(x) == 12);
    assert(static_cast<int const&>(post_inc) == 11);

    // Test prefix decrement
    auto pre_dec = --x;
    assert(static_cast<int const&>(x) == 11);
    assert(static_cast<int const&>(pre_dec) == 11);

    // Test postfix decrement
    auto post_dec = x--;
    assert(static_cast<int const&>(x) == 10);
    assert(static_cast<int const&>(post_dec) == 11);

    std::cout << "Increment/decrement operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find(
                    "Increment/decrement operators test passed") !=
                std::string::npos);
        }

        SUBCASE("bool conversion") {
            auto desc = make_description(
                "struct",
                "test",
                "BoolConvertible",
                "strong int; bool");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::BoolConvertible zero{0};
    test::BoolConvertible nonzero{42};

    // Test explicit bool conversion
    assert(!static_cast<bool>(zero));
    assert(static_cast<bool>(nonzero));

    // Test in if statement
    if (nonzero) {
        // Should enter here
    } else {
        assert(false && "Should not reach here");
    }

    if (zero) {
        assert(false && "Should not reach here");
    } else {
        // Should enter here
    }

    std::cout << "Bool conversion test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Bool conversion test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Stream Operators")
    {
        CodeTester tester;

        SUBCASE("output stream operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Printable",
                "strong int; out");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <sstream>
int main() {
    test::Printable x{42};

    std::ostringstream oss;
    oss << x;

    assert(oss.str() == "42");

    std::cout << "Output stream operator test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Output stream operator test passed") !=
                std::string::npos);
        }

        SUBCASE("input stream operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Readable",
                "strong int; in");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <sstream>
int main() {
    test::Readable x{0};

    std::istringstream iss("123");
    iss >> x;

    assert(static_cast<int const&>(x) == 123);

    std::cout << "Input stream operator test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Input stream operator test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Advanced Features")
    {
        CodeTester tester;

        SUBCASE("call operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Callable",
                "strong int; (), (&)");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Callable x{42};

    // Test nullary call operator
    int& ref = x();
    assert(ref == 42);

    // Test callable invocation
    auto result = x([](int const& val) { return val * 2; });
    assert(result == 84);

    std::cout << "Call operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Call operators test passed") !=
                std::string::npos);
        }

        SUBCASE("pointer operators") {
            auto desc = make_description(
                "struct",
                "test",
                "PointerLike",
                "strong int; @, &of, ->");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::PointerLike x{42};

    // Test indirection
    int& ref = *x;
    assert(ref == 42);

    // Test address-of
    int* ptr1 = &x;
    int* ptr2 = x.operator->();
    assert(ptr1 == ptr2);
    assert(*ptr1 == 42);

    std::cout << "Pointer operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Pointer operators test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Quick Integration Tests")
    {
        CodeTester tester;

        SUBCASE("all major features compile together") {
            auto desc = make_description(
                "class",
                "complete::test",
                "Everything",
                "strong int; +, -, *, ==, !=, <, ++, bool, out, in");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <sstream>
int main() {
    complete::test::Everything x{42};
    complete::test::Everything y{10};

    // Test arithmetic
    auto sum = x + y;
    auto diff = x - y;
    auto prod = x * y;

    // Test comparison
    bool eq = (x == y);
    bool ne = (x != y);
    bool lt = (x < y);

    // Test increment
    ++x;

    // Test bool conversion
    if (x) { /* works */ }

    // Test stream operators
    std::ostringstream oss;
    oss << x;

    std::istringstream iss("123");
    iss >> x;

    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
        }

        SUBCASE("comprehensive feature set with hash") {
            auto desc = make_description(
                "struct",
                "complete::test",
                "Comprehensive",
                "strong int; +, -, ==, !=, <, <=>, ++, bool, out, "
                "no-constexpr-hash");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <sstream>
#include <unordered_set>
int main() {
    complete::test::Comprehensive x{42};
    complete::test::Comprehensive y{10};

    // Test arithmetic
    auto sum = x + y;
    auto diff = x - y;

    // Test comparison
    assert(x == x);
    assert(x != y);
    assert(y < x);
    assert((x <=> y) > 0);

    // Test increment
    ++x;

    // Test bool conversion
    if (x) { /* works */ }

    // Test stream operator
    std::ostringstream oss;
    oss << x;

    // Test hash in container
    std::unordered_set<complete::test::Comprehensive> values;
    values.insert(x);
    values.insert(y);
    assert(values.size() == 2);

    std::cout << "Comprehensive test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Comprehensive test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Constexpr Support")
    {
        CodeTester tester;

        SUBCASE("runtime constructors and casts") {
            auto desc = make_description(
                "struct",
                "runtime_test",
                "Value",
                "strong int; ==, <=>, no-constexpr");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <cassert>
int main() {
    runtime_test::Value v1;
    runtime_test::Value v2{42};
    int raw = static_cast<int>(v2);
    assert(raw == 42);

    runtime_test::Value v3{10};
    int raw2 = static_cast<int>(v3);
    assert(raw2 == 10);
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
        }

        SUBCASE("runtime arithmetic operations") {
            auto desc = make_description(
                "struct",
                "runtime_test",
                "Distance",
                "strong int; +, -, *, /, ==, no-constexpr");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <cassert>
int main() {
    runtime_test::Distance d1{10};
    runtime_test::Distance d2{20};
    auto sum = d1 + d2;
    auto diff = d2 - d1;
    auto prod = d1 * runtime_test::Distance{3};
    auto quot = d2 / runtime_test::Distance{2};

    assert(static_cast<int>(sum) == 30);
    assert(static_cast<int>(diff) == 10);
    assert(static_cast<int>(prod) == 30);
    assert(static_cast<int>(quot) == 10);
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
        }

        SUBCASE("runtime comprehensive test") {
            auto desc = make_description(
                "struct",
                "runtime_test",
                "Value",
                "strong int; +, -, *, u+, u-, ==, !=, <, <=>, ++, --, bool, @, "
                "&of, (), no-constexpr");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <cassert>
int main() {
    // Test explicit construction and casts
    runtime_test::Value v2{42};
    int raw = static_cast<int>(v2);
    assert(raw == 42);

    // Test arithmetic
    runtime_test::Value a{10};
    runtime_test::Value b{20};
    auto sum = a + b;
    auto diff = b - a;
    auto prod = a * runtime_test::Value{3};
    assert(static_cast<int>(sum) == 30);
    assert(static_cast<int>(diff) == 10);
    assert(static_cast<int>(prod) == 30);

    // Test unary operators
    auto pos = +a;
    auto neg = -a;
    assert(static_cast<int>(pos) == 10);
    assert(static_cast<int>(neg) == -10);

    // Test comparisons
    assert(a == runtime_test::Value{10});
    assert(a != b);
    assert(a < b);
    assert((a <=> b) < 0);

    // Test increment/decrement
    runtime_test::Value c{10};
    ++c;
    assert(static_cast<int>(c) == 11);

    // Test indirection
    assert(*runtime_test::Value{42} == 42);

    // Test nullary call
    assert(runtime_test::Value{42}() == 42);

    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
        }

        SUBCASE("no-constexpr opt-out") {
            auto desc = make_description(
                "struct",
                "no_constexpr_test",
                "Value",
                "strong int; +, -, ==, !=, no-constexpr");
            auto generated = generate_strong_type(desc);

            // Verify no constexpr keywords appear in generated code
            // (Note: "no-constexpr" will appear in the description comment)
            CHECK(generated.find("constexpr explicit") == std::string::npos);
            CHECK(generated.find("constexpr Value") == std::string::npos);

            // Verify code still compiles and works at runtime
            auto test_main = R"(
int main() {
    // These work at runtime but not in constexpr context
    no_constexpr_test::Value v1{10};
    no_constexpr_test::Value v2{20};

    auto sum = v1 + v2;
    auto diff = v2 - v1;

    assert(static_cast<int>(sum) == 30);
    assert(static_cast<int>(diff) == 10);
    assert(v1 == no_constexpr_test::Value{10});
    assert(v1 != v2);

    std::cout << "no-constexpr test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("no-constexpr test passed") !=
                std::string::npos);
        }

        SUBCASE("no-constexpr with hash and bool") {
            auto desc = make_description(
                "struct",
                "no_constexpr_test",
                "HashValue",
                "strong int; hash, bool, ==, no-constexpr");
            auto generated = generate_strong_type(desc);

            // Verify no constexpr keywords appear in code (not just comments)
            CHECK(generated.find("constexpr explicit") == std::string::npos);
            CHECK(generated.find("constexpr HashValue") == std::string::npos);
            CHECK(generated.find("constexpr std::size_t") == std::string::npos);

            auto test_main = R"(
#include <functional>
#include <unordered_map>

int main() {
    no_constexpr_test::HashValue v1{42};
    no_constexpr_test::HashValue v2{42};
    no_constexpr_test::HashValue v3{99};

    // Test bool conversion
    if (v1) {
        // non-zero value converts to true
    }

    // Test hash in unordered_map
    std::unordered_map<no_constexpr_test::HashValue, std::string> map;
    map[v1] = "forty-two";
    map[v3] = "ninety-nine";

    assert(map[v2] == "forty-two");  // v2 == v1, so same bucket
    assert(map[v3] == "ninety-nine");
    assert(map.size() == 2);

    std::cout << "no-constexpr hash test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("no-constexpr hash test passed") !=
                std::string::npos);
        }

        SUBCASE("no-constexpr with complex type") {
            auto desc = make_description(
                "struct",
                "no_constexpr_test",
                "StringWrapper",
                "strong std::string; ==, !=, out, no-constexpr");
            auto generated = generate_strong_type(desc);

            // Verify no constexpr keywords in code
            CHECK(generated.find("constexpr explicit") == std::string::npos);
            CHECK(
                generated.find("constexpr StringWrapper") == std::string::npos);
            CHECK(generated.find("constexpr bool") == std::string::npos);

            auto test_main = R"(
#include <string>
#include <sstream>

int main() {
    no_constexpr_test::StringWrapper s1{"hello"};
    no_constexpr_test::StringWrapper s2{"world"};
    no_constexpr_test::StringWrapper s3{"hello"};

    assert(s1 == s3);
    assert(s1 != s2);

    std::ostringstream oss;
    oss << s1 << " " << s2;
    assert(oss.str() == "hello world");

    std::cout << "no-constexpr string test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("no-constexpr string test passed") !=
                std::string::npos);
        }

        SUBCASE("no-constexpr-hash with everything else constexpr") {
            auto desc = make_description(
                "struct",
                "mixed_constexpr_test",
                "Value",
                "strong int; +, -, ==, !=, bool, no-constexpr-hash");
            auto generated = generate_strong_type(desc);

            // Verify regular operations have constexpr
            CHECK(
                generated.find("constexpr explicit Value") !=
                std::string::npos);
            CHECK(
                generated.find("constexpr Value & operator +=") !=
                std::string::npos);
            CHECK(
                generated.find("constexpr bool operator ==") !=
                std::string::npos);

            // Verify hash does NOT have constexpr
            CHECK(
                generated.find("constexpr std::size_t operator ()") ==
                std::string::npos);
            CHECK(
                generated.find("std::size_t operator ()") != std::string::npos);

            auto test_main = R"(
#include <functional>
#include <unordered_map>
#include <cassert>

int main() {
    // Test operations work at runtime (even though they have constexpr)
    mixed_constexpr_test::Value v1{10};
    mixed_constexpr_test::Value v2{20};
    auto sum = v1 + v2;
    assert(static_cast<int>(sum) == 30);
    assert(v1 == mixed_constexpr_test::Value{10});

    // Test hash works at runtime
    mixed_constexpr_test::Value v3{42};
    std::unordered_map<mixed_constexpr_test::Value, std::string> map;
    map[v3] = "forty-two";
    assert(map[v3] == "forty-two");

    std::cout << "no-constexpr-hash test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("no-constexpr-hash test passed") !=
                std::string::npos);
        }

        SUBCASE("std::string with no-constexpr") {
            auto desc = make_description(
                "struct",
                "string_test",
                "StringId",
                "strong std::string; ==, !=, no-constexpr, no-constexpr-hash");
            auto generated = generate_strong_type(desc);

            // Verify NO constexpr anywhere
            CHECK(
                generated.find("constexpr explicit StringId") ==
                std::string::npos);
            CHECK(
                generated.find("constexpr bool operator ==") ==
                std::string::npos);
            CHECK(
                generated.find("constexpr std::size_t operator ()") ==
                std::string::npos);

            auto test_main = R"(
#include <string>
#include <functional>
#include <unordered_set>
#include <cassert>

int main() {
    // Test operations work at runtime
    string_test::StringId id1{"user123"};
    string_test::StringId id2{"user456"};
    string_test::StringId id3{"user123"};

    assert(id1 == id3);
    assert(id1 != id2);

    // Test hash works in unordered_set
    std::unordered_set<string_test::StringId> ids;
    ids.insert(id1);
    ids.insert(id2);
    assert(ids.size() == 2);
    assert(ids.count(id3) == 1);  // id3 == id1

    std::cout << "string no-constexpr test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("string no-constexpr test passed") !=
                std::string::npos);
        }

        SUBCASE("verify hash code generation") {
            auto desc = make_description(
                "struct",
                "codegen_test",
                "Value",
                "strong int; ==, hash, no-constexpr");
            auto generated = generate_strong_type(desc);

            // Just verify correct code generation, don't compile
            CHECK(
                generated.find("struct std::hash<codegen_test::Value>") !=
                std::string::npos);
            CHECK(
                generated.find("std::size_t operator ()") != std::string::npos);
            // Should NOT have constexpr since we used no-constexpr
            CHECK(
                generated.find("constexpr std::size_t operator ()") ==
                std::string::npos);
        }
    }

    TEST_CASE("Subscript Operator Support")
    {
        CodeTester tester;

        SUBCASE("subscript with std::vector") {
            auto desc = make_description(
                "struct",
                "subscript_test",
                "IntArray",
                "strong std::vector<int>; [], #<vector>, no-constexpr");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <vector>

int main() {
    subscript_test::IntArray arr{std::vector<int>{10, 20, 30, 40, 50}};

    // Test const subscript
    subscript_test::IntArray const & const_arr = arr;
    assert(const_arr[0] == 10);
    assert(const_arr[2] == 30);
    assert(const_arr[4] == 50);

    // Test non-const subscript
    arr[1] = 200;
    assert(arr[1] == 200);

    // Test subscript returns reference
    arr[3]++;
    assert(arr[3] == 41);

    std::cout << "Vector subscript test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Vector subscript test passed") !=
                std::string::npos);
        }

        SUBCASE("subscript with custom type") {
            // Test with a custom type that has subscript operator
            auto custom_type_code = R"(
#include <string>
struct CustomContainer {
    std::string data[3] = {"zero", "one", "two"};
    std::string & operator[](int i) { return data[i]; }
    std::string const & operator[](int i) const { return data[i]; }
};
)";

            auto desc = make_description(
                "struct",
                "subscript_test",
                "CustomWrapper",
                "strong CustomContainer; [], no-constexpr");
            auto generated = std::string(custom_type_code) + "\n" +
                generate_strong_type(desc);

            auto test_main = R"(
int main() {
    subscript_test::CustomWrapper w{CustomContainer{}};

    // Test subscript access
    assert(w[0] == "zero");
    assert(w[1] == "one");
    assert(w[2] == "two");

    // Test subscript modification
    w[1] = "ONE";
    assert(w[1] == "ONE");

    std::cout << "Custom type subscript test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Custom type subscript test passed") !=
                std::string::npos);
        }

        SUBCASE("subscript with std::array") {
            auto desc = make_description(
                "struct",
                "subscript_test",
                "FixedArray",
                "strong std::array<int, 5>; [], #<array>");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <array>

int main() {
    subscript_test::FixedArray arr{std::array<int, 5>{1, 2, 3, 4, 5}};

    // Test subscript access
    assert(arr[0] == 1);
    assert(arr[4] == 5);

    // Test subscript modification
    arr[2] = 33;
    assert(arr[2] == 33);

    std::cout << "Array subscript test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Array subscript test passed") !=
                std::string::npos);
        }

        SUBCASE("array subscript runtime") {
            auto desc = make_description(
                "struct",
                "subscript_test",
                "ArrayWrapper",
                "strong std::array<int, 3>; [], #<array>, no-constexpr");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <array>
#include <cassert>

int main() {
    std::array<int, 3> data{10, 20, 30};
    subscript_test::ArrayWrapper arr{data};

    // Test runtime subscript
    int val = arr[1];
    assert(val == 20);

    int first = arr[0];
    assert(first == 10);

    std::cout << "Array subscript runtime test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Array subscript runtime test passed") !=
                std::string::npos);
        }

        SUBCASE("subscript with raw array") {
            auto desc = make_description(
                "struct",
                "subscript_test",
                "RawArrayWrapper",
                "strong int*; []");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    int data[] = {100, 200, 300};
    subscript_test::RawArrayWrapper arr{data};

    // Test subscript access
    assert(arr[0] == 100);
    assert(arr[1] == 200);
    assert(arr[2] == 300);

    // Test subscript modification
    arr[1] = 222;
    assert(arr[1] == 222);
    assert(data[1] == 222);

    std::cout << "Raw array subscript test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Raw array subscript test passed") !=
                std::string::npos);
        }
    }

    TEST_CASE("Default Value Support")
    {
        CodeTester tester;

        SUBCASE("integer default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int",
                "42");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    // Test default constructor uses default value
    test::Counter c1;
    assert(static_cast<int const&>(c1) == 42);

    // Test explicit constructor still works
    test::Counter c2{100};
    assert(static_cast<int const&>(c2) == 100);

    std::cout << "Integer default value test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Integer default value test passed") !=
                std::string::npos);
        }

        SUBCASE("double default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Pi",
                "strong double",
                "3.14159");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Pi pi;
    double val = static_cast<double const&>(pi);
    assert(val > 3.14158 && val < 3.14160);

    std::cout << "Double default value test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Double default value test passed") !=
                std::string::npos);
        }

        SUBCASE("string default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Name",
                "strong std::string; no-constexpr",
                R"("hello")");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Name name;
    std::string const& val = static_cast<std::string const&>(name);
    assert(val == "hello");

    // Test explicit construction still works
    test::Name custom{"world"};
    assert(static_cast<std::string const&>(custom) == "world");

    std::cout << "String default value test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("String default value test passed") !=
                std::string::npos);
        }

        SUBCASE("default value with operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Score",
                "strong int; +, -, ==, !=",
                "100");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Score s1;
    test::Score s2;

    // Both default-constructed should have same value
    assert(s1 == s2);
    assert(static_cast<int const&>(s1) == 100);

    // Test arithmetic with default value
    test::Score s3{10};
    auto sum = s1 + s3;
    assert(static_cast<int const&>(sum) == 110);

    auto diff = s1 - s3;
    assert(static_cast<int const&>(diff) == 90);

    std::cout << "Default value with operators test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find(
                    "Default value with operators test passed") !=
                std::string::npos);
        }

        SUBCASE("default value initialization") {
            auto desc = make_description(
                "struct",
                "test",
                "DefaultInit",
                "strong int; ==, no-constexpr",
                "999");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <cassert>
int main() {
    // Test that default constructor initializes with default value
    test::DefaultInit cd;
    int val = static_cast<int>(cd);
    assert(val == 999);

    // Test runtime behavior
    test::DefaultInit runtime;
    assert(static_cast<int const&>(runtime) == 999);

    std::cout << "Default value initialization test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find(
                    "Default value initialization test passed") !=
                std::string::npos);
        }

        SUBCASE("negative default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Negative",
                "strong int; ==",
                "-42");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::Negative neg;
    assert(static_cast<int const&>(neg) == -42);

    std::cout << "Negative default value test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Negative default value test passed") !=
                std::string::npos);
        }

        SUBCASE("complex expression default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Numbers",
                "strong std::vector<int>; no-constexpr",
                "std::vector<int>{1, 2, 3}");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <vector>
int main() {
    test::Numbers nums;
    std::vector<int> const& vec = static_cast<std::vector<int> const&>(nums);
    assert(vec.size() == 3);
    assert(vec[0] == 1);
    assert(vec[1] == 2);
    assert(vec[2] == 3);

    std::cout << "Complex expression default value test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find(
                    "Complex expression default value test passed") !=
                std::string::npos);
        }

        SUBCASE("triviality without default value") {
            auto desc = make_description(
                "struct",
                "test",
                "TrivialType",
                "strong int; ==");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <type_traits>
int main() {
    // Without default value, type should be trivially default constructible
    static_assert(std::is_trivially_default_constructible_v<test::TrivialType>,
                  "Type without default value should be trivially default constructible");

    // Note: The template constructor makes it non-trivial overall,
    // but the default constructor itself is trivial

    std::cout << "Triviality test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Triviality test passed") !=
                std::string::npos);
        }

        SUBCASE("non-triviality with default value") {
            auto desc = make_description(
                "struct",
                "test",
                "NonTrivialType",
                "strong int; ==",
                "42");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <type_traits>
int main() {
    // With default value, type is NOT trivially default constructible
    static_assert(!std::is_trivially_default_constructible_v<test::NonTrivialType>,
                  "Type with default value should NOT be trivially default constructible");

    // But it should work correctly
    test::NonTrivialType t;
    assert(static_cast<int const&>(t) == 42);

    std::cout << "Non-triviality test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Non-triviality test passed") !=
                std::string::npos);
        }
    }
}

} // anonymous namespace
