// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "StrongTypeGenerator.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"
#include "rapidcheck.hpp"

using namespace wjh::atlas::testing;
using wjh::atlas::testing::rc::check;

namespace {

using namespace wjh::atlas;

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

bool
contains_all(
    std::string_view code,
    std::vector<std::string_view> const & patterns)
{
    return std::all_of(
        patterns.begin(),
        patterns.end(),
        [code](auto const & pattern) {
            return code.find(pattern) != std::string_view::npos;
        });
}

bool
contains_none(
    std::string_view code,
    std::vector<std::string_view> const & patterns)
{
    return std::none_of(
        patterns.begin(),
        patterns.end(),
        [code](auto const & pattern) {
            return code.find(pattern) != std::string_view::npos;
        });
}

TEST_SUITE("StrongTypeGenerator")
{
    TEST_CASE("Basic Structure Generation")
    {
        SUBCASE("struct generation") {
            auto desc =
                make_description("struct", "test", "MyInt", "strong int");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"#ifndef", "#define", "#endif"}));
            CHECK(contains_all(code, {"struct MyInt", "int value;"}));
            CHECK(contains_none(code, {"public:", "private:"}));
        }

        SUBCASE("class generation") {
            auto desc = make_description(
                "class",
                "test::nested",
                "MyString",
                "strong std::string");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"class MyString", "std::string value;", "public:"}));
            CHECK(code.find("private:") == std::string::npos);
        }

        SUBCASE("namespace handling") {
            auto desc =
                make_description("struct", "a::b::c", "MyType", "strong int");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"namespace a::b::c {", "} // namespace a::b::c"}));
        }
    }

    TEST_CASE("Header Guard Generation")
    {
        SUBCASE("default guard generation") {
            auto desc =
                make_description("struct", "test", "MyType", "strong int");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#ifndef TEST_MYTYPE_") != std::string::npos);
            CHECK(code.find("#define TEST_MYTYPE_") != std::string::npos);
        }

        SUBCASE("custom guard prefix") {
            auto desc = make_description(
                "struct",
                "test",
                "MyType",
                "strong int",
                "", // default_value
                "CUSTOM"); // guard_prefix
            auto code = generate_strong_type(desc);

            CHECK(code.find("#ifndef CUSTOM_") != std::string::npos);
            CHECK(code.find("#define CUSTOM_") != std::string::npos);
        }

        SUBCASE("custom guard separator") {
            auto desc = make_description(
                "struct",
                "test",
                "MyType",
                "strong int",
                "", // default_value
                "", // guard_prefix
                "__"); // guard_separator
            auto code = generate_strong_type(desc);

            CHECK(code.find("#ifndef TEST__MYTYPE__") != std::string::npos);
        }

        SUBCASE("lowercase guard") {
            auto desc = make_description(
                "struct",
                "test",
                "MyType",
                "strong int",
                "", // default_value
                "", // guard_prefix
                "_", // guard_separator
                false); // upcase_guard
            auto code = generate_strong_type(desc);

            CHECK(code.find("#ifndef test_MyType_") != std::string::npos);
        }
    }

    TEST_CASE("Arithmetic Operators")
    {
        SUBCASE("binary arithmetic operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; +, -, *, /, %");
            auto code = generate_strong_type(desc);

            for (auto op : {"+", "-", "*", "/", "%"}) {
                CHECK(
                    code.find(std::string("operator ") + op + "=") !=
                    std::string::npos);
                CHECK(
                    code.find(std::string("operator ") + op + " (") !=
                    std::string::npos);
            }
        }

        SUBCASE("unary arithmetic operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; u+, u-, u~");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"operator + (", "operator - (", "operator ~ ("}));
        }

        SUBCASE("combined binary and unary operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; +*, -*");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"operator +=",
                 "operator + (",
                 "operator -=",
                 "operator - ("}));
        }

        SUBCASE("bitwise operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Bits",
                "strong int; &, |, ^, <<, >>");
            auto code = generate_strong_type(desc);

            for (auto op : {"&", "|", "^", "<<", ">>"}) {
                CHECK(
                    code.find(std::string("operator ") + op + "=") !=
                    std::string::npos);
                CHECK(
                    code.find(std::string("operator ") + op + " (") !=
                    std::string::npos);
            }
        }
    }

    TEST_CASE("Comparison Operators")
    {
        SUBCASE("relational operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Number",
                "strong int; ==, !=, <, <=, >, >=");
            auto code = generate_strong_type(desc);

            for (auto op : {"==", "!=", "<", "<=", ">", ">="}) {
                CHECK(
                    code.find(std::string("operator ") + op + " (") !=
                    std::string::npos);
            }
        }

        SUBCASE("spaceship operator") {
            auto desc =
                make_description("struct", "test", "Number", "strong int; <=>");
            auto code = generate_strong_type(desc);

            CHECK(code.find("operator <=> (") != std::string::npos);
            CHECK(code.find("= default") != std::string::npos);
        }
    }

    TEST_CASE("Special Operators")
    {
        SUBCASE("increment/decrement operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int; ++, --");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"operator ++ (", "operator -- ("}));
        }

        SUBCASE("indirection operator") {
            auto desc =
                make_description("struct", "test", "Ptr", "strong int*; @");
            auto code = generate_strong_type(desc);

            CHECK(code.find("operator * ()") != std::string::npos);
        }

        SUBCASE("address-of operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Ref",
                "strong int; &of, ->");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"operator & ()", "operator -> ()"}));
        }

        SUBCASE("call operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Callable",
                "strong int; (), (&)");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"operator () ()", "operator () (InvocableT"}));
        }

        SUBCASE("bool conversion") {
            auto desc = make_description(
                "struct",
                "test",
                "BoolConv",
                "strong int; bool");
            auto code = generate_strong_type(desc);

            CHECK(code.find("explicit operator bool ()") != std::string::npos);
        }
    }

    TEST_CASE("Stream Operators")
    {
        SUBCASE("output stream operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Printable",
                "strong int; out");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"#include <ostream>", "operator << ("}));
        }

        SUBCASE("input stream operator") {
            auto desc = make_description(
                "struct",
                "test",
                "Readable",
                "strong int; in");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"#include <istream>", "operator >> ("}));
        }

        SUBCASE("both stream operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Streamable",
                "strong int; in, out");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"#include <istream>", "#include <ostream>"}));
        }
    }

    TEST_CASE("Automatic Header Detection")
    {
        SUBCASE("standard string types") {
            auto desc =
                make_description("struct", "test", "Str", "strong std::string");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include <string>") != std::string::npos);
        }

        SUBCASE("standard containers") {
            auto desc = make_description(
                "struct",
                "test",
                "Vec",
                "strong std::vector<int>");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include <vector>") != std::string::npos);
        }

        SUBCASE("standard optional") {
            auto desc = make_description(
                "struct",
                "test",
                "Opt",
                "strong std::optional<int>");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include <optional>") != std::string::npos);
        }

        SUBCASE("chrono types") {
            auto desc = make_description(
                "struct",
                "test",
                "Time",
                "strong std::chrono::seconds");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include <chrono>") != std::string::npos);
        }
    }

    TEST_CASE("Custom Headers")
    {
        SUBCASE("custom header with quotes") {
            auto desc = make_description(
                "struct",
                "test",
                "Custom",
                "strong MyType; #\"my/header.hpp\"");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include \"my/header.hpp\"") != std::string::npos);
        }

        SUBCASE("custom header with single quotes") {
            auto desc = make_description(
                "struct",
                "test",
                "Custom",
                "strong MyType; #'my/header.hpp'");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include \"my/header.hpp\"") != std::string::npos);
        }

        SUBCASE("custom header with angle brackets") {
            auto desc = make_description(
                "struct",
                "test",
                "Custom",
                "strong MyType; #<system/header>");
            auto code = generate_strong_type(desc);

            CHECK(code.find("#include <system/header>") != std::string::npos);
        }
    }

    TEST_CASE("Default Value Support")
    {
        SUBCASE("integer default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Counter",
                "strong int",
                "42");
            auto code = generate_strong_type(desc);

            CHECK(code.find("value{42}") != std::string::npos);
            CHECK(code.find("value{}") == std::string::npos);
        }

        SUBCASE("double default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Pi",
                "strong double",
                "3.14159");
            auto code = generate_strong_type(desc);

            CHECK(code.find("value{3.14159}") != std::string::npos);
        }

        SUBCASE("string default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Name",
                "strong std::string",
                R"("hello")");
            auto code = generate_strong_type(desc);

            CHECK(code.find("value{\"hello\"}") != std::string::npos);
        }

        SUBCASE("complex expression default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Numbers",
                "strong std::vector<int>",
                "std::vector<int>{1, 2, 3}");
            auto code = generate_strong_type(desc);

            CHECK(
                code.find("value{std::vector<int>{1, 2, 3}}") !=
                std::string::npos);
        }

        SUBCASE("string with commas") {
            auto desc = make_description(
                "struct",
                "test",
                "CommaString",
                "strong std::string",
                R"("42,43")");
            auto code = generate_strong_type(desc);

            CHECK(code.find(R"(value{"42,43"})") != std::string::npos);
        }

        SUBCASE("default value with other operators") {
            auto desc = make_description(
                "struct",
                "test",
                "Score",
                "strong int; +, -, ==, !=",
                "100");
            auto code = generate_strong_type(desc);

            CHECK(code.find("value{100}") != std::string::npos);
            CHECK(contains_all(
                code,
                {"operator +=", "operator -=", "operator ==", "operator !="}));
        }

        SUBCASE("no default value uses default construction") {
            auto desc =
                make_description("struct", "test", "Regular", "strong int");
            auto code = generate_strong_type(desc);

            CHECK(code.find("int value;") != std::string::npos);
            CHECK(code.find("value{") == std::string::npos);
        }

        SUBCASE("negative default value") {
            auto desc = make_description(
                "struct",
                "test",
                "Negative",
                "strong int",
                "-42");
            auto code = generate_strong_type(desc);

            CHECK(code.find("value{-42}") != std::string::npos);
        }
    }

    TEST_CASE("Error Handling")
    {
        SUBCASE("invalid kind throws exception") {
            auto desc =
                make_description("invalid", "test", "Bad", "strong int");

            CHECK_THROWS_AS(generate_strong_type(desc), std::invalid_argument);
        }

        SUBCASE("empty description still generates basic structure") {
            auto desc =
                make_description("struct", "test", "Empty", "strong int; ");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"struct Empty", "int value;"}));
        }
    }

    TEST_CASE("Complex Type Descriptions")
    {
        SUBCASE("all operators together") {
            auto desc = make_description(
                "struct",
                "test",
                "Complete",
                "strong int; +, -, *, /, %, &, |, ^, <<, >>, ==, !=, <, <=, >, "
                ">=, <=>, ++, --, bool, out, in");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(
                code,
                {"operator +", "operator ==", "operator <=>", "operator ++"}));
            CHECK(contains_all(
                code,
                {"#include <ostream>", "#include <istream>"}));
        }

        SUBCASE("nested namespace and complex type") {
            auto desc = make_description(
                "class",
                "company::product::module",
                "Container::Element",
                "strong std::unique_ptr<Data>; ->, bool, ==, !=");
            auto code = generate_strong_type(desc);

            CHECK(contains_all(code, {"namespace company::product::module {"}));
            CHECK(contains_all(
                code,
                {"class Container::Element", "std::unique_ptr<Data> value;"}));
        }
    }

    TEST_CASE("Property-Based Tests")
    {
        SUBCASE("generated code is valid C++ structure") {
            check("Generated code has proper guard structure", [](int seed) {
                auto name = "Type" + std::to_string(std::abs(seed) % 1000);
                auto desc =
                    make_description("struct", "test", name, "strong int");
                auto code = generate_strong_type(desc);

                RC_ASSERT(code.find("#ifndef") < code.find("#define"));
                RC_ASSERT(code.find("#define") < code.find("struct " + name));
                RC_ASSERT(code.find("struct " + name) < code.find("#endif"));
            });
        }

        SUBCASE("all generated guards are unique") {
            check(
                "Guards are unique for different types",
                [](int seed1, int seed2) {
                    RC_PRE(seed1 != seed2);

                    auto name1 = "Type" +
                        std::to_string(std::abs(seed1) % 1000);
                    auto name2 = "Type" +
                        std::to_string(std::abs(seed2) % 1000);

                    if (name1 == name2) {
                        return;
                    }

                    auto desc1 =
                        make_description("struct", "test", name1, "strong int");
                    auto desc2 =
                        make_description("struct", "test", name2, "strong int");

                    auto code1 = generate_strong_type(desc1);
                    auto code2 = generate_strong_type(desc2);

                    std::regex guard_regex{R"(#ifndef\s+([A-Z_0-9]+))"};
                    std::smatch match1, match2;

                    RC_ASSERT(std::regex_search(code1, match1, guard_regex));
                    RC_ASSERT(std::regex_search(code2, match2, guard_regex));
                    RC_ASSERT(match1[1] != match2[1]);
                });
        }

        SUBCASE("operator selection is consistent") {
            check("Requested operators appear in generated code", []() {
                auto operators = *::rc::gen::container<
                    std::vector<std::string>>(
                    ::rc::gen::element("+", "-", "*", "==", "!=", "++", "out"));

                if (operators.empty()) {
                    return;
                }

                std::string desc_str = "strong int";
                for (size_t i = 0; i < operators.size(); ++i) {
                    if (i == 0) {
                        desc_str += "; ";
                    } else {
                        desc_str += ", ";
                    }
                    desc_str += operators[i];
                }

                auto desc =
                    make_description("struct", "test", "TestOp", desc_str);
                auto code = generate_strong_type(desc);

                for (auto const & op : operators) {
                    if (op == "out") {
                        RC_ASSERT(
                            code.find("operator << (") != std::string::npos);
                    } else {
                        RC_ASSERT(
                            code.find("operator " + op) != std::string::npos);
                    }
                }
            });
        }
    }
}

} // anonymous namespace
