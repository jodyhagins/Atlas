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
        cmd << "c++ -std=c++20 -I. -o " << exe_path.filename().string() << " "
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
                "strong std::string");
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

    TEST_CASE("Hash Support")
    {
        CodeTester tester;

        SUBCASE("hash with int type") {
            auto desc = make_description(
                "struct",
                "test",
                "HashableInt",
                "strong int; ==, hash");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <unordered_set>
#include <unordered_map>
int main() {
    // Test hash with unordered_set
    std::unordered_set<test::HashableInt> int_set;
    test::HashableInt x{42};
    test::HashableInt y{42};
    test::HashableInt z{99};

    int_set.insert(x);
    assert(int_set.size() == 1);

    // Same value should not increase size
    int_set.insert(y);
    assert(int_set.size() == 1);

    // Different value should increase size
    int_set.insert(z);
    assert(int_set.size() == 2);

    // Test find
    assert(int_set.find(x) != int_set.end());
    assert(int_set.find(test::HashableInt{999}) == int_set.end());

    // Test hash with unordered_map
    std::unordered_map<test::HashableInt, std::string> int_map;
    int_map[test::HashableInt{1}] = "one";
    int_map[test::HashableInt{2}] = "two";
    int_map[test::HashableInt{3}] = "three";

    assert(int_map.size() == 3);
    assert(int_map[test::HashableInt{1}] == "one");
    assert(int_map[test::HashableInt{2}] == "two");
    assert(int_map[test::HashableInt{3}] == "three");

    // Verify hash function is callable directly
    std::hash<test::HashableInt> hasher;
    auto hash_val = hasher(x);
    assert(hash_val == hasher(y)); // Same values should have same hash
    (void)hash_val;

    std::cout << "Hash with int test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Hash with int test passed") !=
                std::string::npos);
        }

        SUBCASE("hash with string type") {
            auto desc = make_description(
                "struct",
                "test",
                "HashableString",
                "strong std::string; ==, hash");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <unordered_set>
#include <unordered_map>
int main() {
    std::unordered_set<test::HashableString> string_set;
    test::HashableString hello{"hello"};
    test::HashableString world{"world"};
    test::HashableString hello2{"hello"};

    string_set.insert(hello);
    string_set.insert(world);
    string_set.insert(hello2);

    // Should only have 2 unique values
    assert(string_set.size() == 2);
    assert(string_set.find(hello) != string_set.end());
    assert(string_set.find(world) != string_set.end());

    // Test with map
    std::unordered_map<test::HashableString, int> string_map;
    string_map[test::HashableString{"apple"}] = 1;
    string_map[test::HashableString{"banana"}] = 2;
    assert(string_map.size() == 2);
    assert(string_map[test::HashableString{"apple"}] == 1);

    std::cout << "Hash with string test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Hash with string test passed") !=
                std::string::npos);
        }

        SUBCASE("hash with namespaced type") {
            auto desc = make_description(
                "struct",
                "my::deep::ns",
                "HashableValue",
                "strong unsigned; ==, hash");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <unordered_set>
int main() {
    std::unordered_set<my::deep::ns::HashableValue> val_set;
    val_set.insert(my::deep::ns::HashableValue{10});
    val_set.insert(my::deep::ns::HashableValue{20});
    val_set.insert(my::deep::ns::HashableValue{10});

    assert(val_set.size() == 2);

    std::cout << "Hash with namespaced type test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Hash with namespaced type test passed") !=
                std::string::npos);
        }

        SUBCASE("hash consistency") {
            auto desc = make_description(
                "struct",
                "test",
                "ConsistentHash",
                "strong int; ==, hash");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    test::ConsistentHash a{42};
    test::ConsistentHash b{42};
    test::ConsistentHash c{43};

    std::hash<test::ConsistentHash> hasher;

    // Equal values must have equal hashes
    assert(hasher(a) == hasher(b));

    // Different values should (very likely) have different hashes
    // This is not guaranteed by the standard but extremely likely
    assert(hasher(a) != hasher(c));

    // Hash should be consistent across multiple calls
    auto hash1 = hasher(a);
    auto hash2 = hasher(a);
    assert(hash1 == hash2);

    std::cout << "Hash consistency test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Hash consistency test passed") !=
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
                "strong int; +, -, ==, !=, <, <=>, ++, bool, out, hash");
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

        SUBCASE("constexpr constructors and casts") {
            auto desc = make_description(
                "struct",
                "constexpr_test",
                "Value",
                "strong int; ==, <=>");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    // Default construction (not constexpr without default value)
    constexpr_test::Value v1;

    // Explicit construction is constexpr
    constexpr constexpr_test::Value v2{42};
    constexpr int raw = static_cast<int>(v2);
    static_assert(raw == 42);

    constexpr constexpr_test::Value v3{10};
    constexpr int raw2 = static_cast<int>(v3);
    static_assert(raw2 == 10);
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
        }

        SUBCASE("constexpr arithmetic operations") {
            auto desc = make_description(
                "struct",
                "constexpr_test",
                "Distance",
                "strong int; +, -, *, /, ==");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    constexpr constexpr_test::Distance d1{10};
    constexpr constexpr_test::Distance d2{20};
    constexpr auto sum = d1 + d2;
    constexpr auto diff = d2 - d1;
    constexpr auto prod = d1 * constexpr_test::Distance{3};
    constexpr auto quot = d2 / constexpr_test::Distance{2};

    static_assert(static_cast<int>(sum) == 30);
    static_assert(static_cast<int>(diff) == 10);
    static_assert(static_cast<int>(prod) == 30);
    static_assert(static_cast<int>(quot) == 10);
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
        }

        SUBCASE("constexpr comprehensive test") {
            auto desc = make_description(
                "struct",
                "constexpr_test",
                "Value",
                "strong int; +, -, *, u+, u-, ==, !=, <, <=>, ++, --, bool, @, "
                "&of, ()");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    // Test explicit construction and casts
    constexpr constexpr_test::Value v2{42};
    constexpr int raw = static_cast<int>(v2);
    static_assert(raw == 42);

    // Test arithmetic
    constexpr constexpr_test::Value a{10};
    constexpr constexpr_test::Value b{20};
    constexpr auto sum = a + b;
    constexpr auto diff = b - a;
    constexpr auto prod = a * constexpr_test::Value{3};
    static_assert(static_cast<int>(sum) == 30);
    static_assert(static_cast<int>(diff) == 10);
    static_assert(static_cast<int>(prod) == 30);

    // Test unary operators
    constexpr auto pos = +a;
    constexpr auto neg = -a;
    static_assert(static_cast<int>(pos) == 10);
    static_assert(static_cast<int>(neg) == -10);

    // Test comparisons
    static_assert(a == constexpr_test::Value{10});
    static_assert(a != b);
    static_assert(a < b);
    static_assert((a <=> b) < 0);

    // Test increment/decrement in constexpr context
    constexpr auto test_inc = []() {
        constexpr_test::Value c{10};
        ++c;
        return static_cast<int>(c) == 11;
    }();
    static_assert(test_inc);

    // Test indirection - use inline value to avoid reference issues
    static_assert(*constexpr_test::Value{42} == 42);

    // Test nullary call - use inline value to avoid reference issues
    static_assert(constexpr_test::Value{42}() == 42);

    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
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
                "strong std::vector<int>; [], #<vector>");
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
                "strong CustomContainer; []");
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

        SUBCASE("constexpr subscript") {
            auto desc = make_description(
                "struct",
                "subscript_test",
                "ConstArray",
                "strong std::array<int, 3>; [], #<array>");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
#include <array>

int main() {
    constexpr std::array<int, 3> data{10, 20, 30};
    constexpr subscript_test::ConstArray arr{data};

    // Test constexpr subscript
    constexpr int val = arr[1];
    static_assert(val == 20);

    constexpr int first = arr[0];
    static_assert(first == 10);

    std::cout << "Constexpr subscript test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Constexpr subscript test passed") !=
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
                "strong std::string",
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

        SUBCASE("constexpr default value") {
            auto desc = make_description(
                "struct",
                "test",
                "ConstDefault",
                "strong int; ==",
                "999");
            auto generated = generate_strong_type(desc);

            auto test_main = R"(
int main() {
    // Test that default constructor is constexpr
    constexpr test::ConstDefault cd;
    constexpr int val = static_cast<int>(cd);
    static_assert(val == 999);

    // Test runtime behavior
    test::ConstDefault runtime;
    assert(static_cast<int const&>(runtime) == 999);

    std::cout << "Constexpr default value test passed\n";
    return 0;
}
)";

            auto result = tester.compile_and_test(generated, test_main);
            CHECK(result.success);
            CHECK(
                result.output.find("Constexpr default value test passed") !=
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
                "strong std::vector<int>",
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
