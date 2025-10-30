// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TypeTokenizer.hpp"
#include "doctest.hpp"

#include <algorithm>

namespace {

using namespace wjh::atlas;

TEST_SUITE("TypeTokenizer")
{
    TEST_CASE("Empty input")
    {
        auto tokens = tokenize_type("");
        REQUIRE(tokens.empty());
    }

    TEST_CASE("Simple unqualified type")
    {
        auto tokens = tokenize_type("int");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "int");
    }

    TEST_CASE("Simple qualified type")
    {
        auto tokens = tokenize_type("std::string");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::string");
    }

    TEST_CASE("Type with spaces around namespace separator")
    {
        auto tokens = tokenize_type("std :: string");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::string");
    }

    TEST_CASE("Three-level namespace")
    {
        auto tokens = tokenize_type("std::chrono::nanoseconds");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::chrono::nanoseconds");
    }

    TEST_CASE("Simple template with one parameter")
    {
        auto tokens = tokenize_type("std::vector<int>");
        REQUIRE(tokens.size() == 2);
        CHECK_EQ(tokens[0], "std::vector");
        CHECK_EQ(tokens[1], "int");
    }

    TEST_CASE("Template with two parameters")
    {
        auto tokens = tokenize_type("std::map<std::string, int>");
        REQUIRE(tokens.size() == 3);
        CHECK_EQ(tokens[0], "std::map");
        CHECK_EQ(tokens[1], "std::string");
        CHECK_EQ(tokens[2], "int");
    }

    TEST_CASE("Nested templates")
    {
        auto tokens = tokenize_type("std::vector<std::vector<int>>");
        REQUIRE(tokens.size() == 3);
        CHECK_EQ(tokens[0], "std::vector");
        CHECK_EQ(tokens[1], "std::vector");
        CHECK_EQ(tokens[2], "int");
    }

    TEST_CASE("Complex nested template")
    {
        auto tokens = tokenize_type("std::map<std::string, std::int64_t>");
        REQUIRE(tokens.size() == 3);
        CHECK_EQ(tokens[0], "std::map");
        CHECK_EQ(tokens[1], "std::string");
        CHECK_EQ(tokens[2], "std::int64_t");
    }

    TEST_CASE("Type with underscores")
    {
        auto tokens = tokenize_type("std::int64_t");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::int64_t");
    }

    TEST_CASE("Multiple spaces")
    {
        auto tokens = tokenize_type("std  ::  vector  <  int  >");
        REQUIRE(tokens.size() == 2);
        CHECK_EQ(tokens[0], "std::vector");
        CHECK_EQ(tokens[1], "int");
    }

    TEST_CASE("Integral type without qualification")
    {
        auto tokens = tokenize_type("int8_t");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "int8_t");
    }

    TEST_CASE("Size_t unqualified")
    {
        auto tokens = tokenize_type("size_t");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "size_t");
    }

    TEST_CASE("Very deeply nested templates")
    {
        auto tokens = tokenize_type("std::vector<std::map<std::string, "
                                    "std::vector<std::int64_t>>>");
        REQUIRE(tokens.size() == 5);
        CHECK_EQ(tokens[0], "std::vector");
        CHECK_EQ(tokens[1], "std::map");
        CHECK_EQ(tokens[2], "std::string");
        CHECK_EQ(tokens[3], "std::vector");
        CHECK_EQ(tokens[4], "std::int64_t");
    }

    TEST_CASE("Identifier with mixed case")
    {
        auto tokens = tokenize_type("MyCustomType");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "MyCustomType");
    }

    TEST_CASE("Namespace with custom type")
    {
        auto tokens = tokenize_type("my::custom::Type");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "my::custom::Type");
    }

    TEST_CASE("Template with user-defined type")
    {
        auto tokens = tokenize_type("std::vector<MyType>");
        REQUIRE(tokens.size() == 2);
        CHECK_EQ(tokens[0], "std::vector");
        CHECK_EQ(tokens[1], "MyType");
    }

    TEST_CASE("Spaces at beginning and end")
    {
        auto tokens = tokenize_type("  std::string  ");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::string");
    }

    TEST_CASE("Pointer and reference characters are ignored")
    {
        auto tokens = tokenize_type("std::string*");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::string");
    }

    TEST_CASE("Reference characters are ignored")
    {
        auto tokens = tokenize_type("std::string&");
        REQUIRE(tokens.size() == 1);
        CHECK_EQ(tokens[0], "std::string");
    }

    TEST_CASE("Const qualifier is extracted as separate token")
    {
        auto tokens = tokenize_type("const std::string");
        // "const" is extracted as a token since it's alphanumeric
        REQUIRE(tokens.size() == 2);
        CHECK_EQ(tokens[0], "const");
        CHECK_EQ(tokens[1], "std::string");
    }
}
} // anonymous namespace

TEST_SUITE("DeclductHeadersFromType")
{
    TEST_CASE("Simple string type")
    {
        auto headers = deduce_headers_from_type("std::string");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<string>");
    }

    TEST_CASE("Int8_t type")
    {
        auto headers = deduce_headers_from_type("int8_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("Std qualified int64_t")
    {
        auto headers = deduce_headers_from_type("std::int64_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("Size_t type")
    {
        auto headers = deduce_headers_from_type("size_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstddef>");
    }

    TEST_CASE("Vector type")
    {
        auto headers = deduce_headers_from_type("std::vector");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<vector>");
    }

    TEST_CASE("Chrono namespace type")
    {
        auto headers = deduce_headers_from_type("std::chrono::nanoseconds");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<chrono>");
    }

    TEST_CASE("Filesystem namespace type")
    {
        auto headers = deduce_headers_from_type("std::filesystem::path");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<filesystem>");
    }

    TEST_CASE("Map with string and int64_t")
    {
        auto headers = deduce_headers_from_type(
            "std::map<std::string, std::int64_t>");
        // Should include <cstdint>, <map>, <string>
        REQUIRE(headers.size() == 3);
        CHECK(
            std::find(headers.begin(), headers.end(), "<cstdint>") !=
            headers.end());
        CHECK(
            std::find(headers.begin(), headers.end(), "<map>") !=
            headers.end());
        CHECK(
            std::find(headers.begin(), headers.end(), "<string>") !=
            headers.end());
    }

    TEST_CASE("Nested vectors")
    {
        auto headers = deduce_headers_from_type(
            "std::vector<std::vector<int>>");
        // Should include <vector> only once (deduplicated)
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<vector>");
    }

    TEST_CASE("Unknown type")
    {
        auto headers = deduce_headers_from_type("MyCustomType");
        REQUIRE(headers.empty());
    }

    TEST_CASE("Template with unknown type")
    {
        auto headers = deduce_headers_from_type("std::vector<MyCustomType>");
        // Only <vector> should be included
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<vector>");
    }

    TEST_CASE("Headers are sorted")
    {
        auto headers = deduce_headers_from_type(
            "std::map<std::string, std::int64_t>");
        // Headers should be in sorted order
        auto sorted = headers;
        std::sort(sorted.begin(), sorted.end());
        CHECK(headers == sorted);
    }
}
