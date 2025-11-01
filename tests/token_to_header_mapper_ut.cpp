// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "atlas/TokenToHeaderMapper.hpp"

#include "doctest.hpp"

namespace {

using namespace wjh::atlas;

TEST_SUITE("HeaderMapper - Exact Matches")
{
    TEST_CASE("std::string")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::string");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<string>");
    }

    TEST_CASE("std::vector")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::vector");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<vector>");
    }

    TEST_CASE("std::map")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::map");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<map>");
    }

    TEST_CASE("std::optional")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::optional");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<optional>");
    }

    TEST_CASE("std::any")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::any");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<any>");
    }

    TEST_CASE("std::shared_ptr")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::shared_ptr");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<memory>");
    }

    TEST_CASE("std::unique_ptr")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::unique_ptr");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<memory>");
    }

    TEST_CASE("std::mutex")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::mutex");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<mutex>");
    }

    TEST_CASE("std::thread")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::thread");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<thread>");
    }

    TEST_CASE("std::regex")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::regex");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<regex>");
    }

    TEST_CASE("std::function")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::function");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<functional>");
    }

    TEST_CASE("std::pair")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::pair");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<utility>");
    }

    TEST_CASE("std::array")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::array");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<array>");
    }
}

TEST_SUITE("HeaderMapper - Integral Types (Qualified)")
{
    TEST_CASE("std::int8_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::int8_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::int16_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::int16_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::int32_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::int32_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::int64_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::int64_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::uint8_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::uint8_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::uint64_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::uint64_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::size_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::size_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstddef>");
    }

    TEST_CASE("std::ptrdiff_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::ptrdiff_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstddef>");
    }

    TEST_CASE("std::intptr_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::intptr_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("std::uintptr_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::uintptr_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }
}

TEST_SUITE("HeaderMapper - Integral Types (Unqualified)")
{
    TEST_CASE("int8_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("int8_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("int64_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("int64_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("uint32_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("uint32_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstdint>");
    }

    TEST_CASE("size_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("size_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstddef>");
    }

    TEST_CASE("ptrdiff_t")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("ptrdiff_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstddef>");
    }
}

TEST_SUITE("HeaderMapper - Namespace Prefixes")
{
    TEST_CASE("std::chrono::nanoseconds")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::chrono::nanoseconds");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<chrono>");
    }

    TEST_CASE("std::chrono::seconds")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::chrono::seconds");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<chrono>");
    }

    TEST_CASE("std::chrono::high_resolution_clock")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::chrono::high_resolution_clock");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<chrono>");
    }

    TEST_CASE("std::filesystem::path")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::filesystem::path");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<filesystem>");
    }

    TEST_CASE("std::filesystem::directory_entry")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::filesystem::directory_entry");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<filesystem>");
    }

    TEST_CASE("std::ranges::range")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::ranges::range");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<ranges>");
    }

    TEST_CASE("std::pmr::vector")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::pmr::vector");
        REQUIRE(headers.size() == 2);
        CHECK_EQ(headers[0], "<memory_resource>");
        CHECK_EQ(headers[1], "<vector>");
    }

    TEST_CASE("std::pmr::string")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::pmr::string");
        REQUIRE(headers.size() == 2);
        CHECK_EQ(headers[0], "<memory_resource>");
        CHECK_EQ(headers[1], "<string>");
    }

    TEST_CASE("std::pmr::memory_resource")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::pmr::memory_resource");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<memory_resource>");
    }

    TEST_CASE("std::pmr::polymorphic_allocator")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::pmr::polymorphic_allocator");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<memory_resource>");
    }

    TEST_CASE("std::execution::sequenced_policy")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("std::execution::sequenced_policy");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<execution>");
    }
}

TEST_SUITE("HeaderMapper - Unknown Types")
{
    TEST_CASE("Custom type")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("MyCustomType");
        REQUIRE(headers.empty());
    }

    TEST_CASE("Custom namespace")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("my::custom::Type");
        REQUIRE(headers.empty());
    }

    TEST_CASE("int (built-in type)")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("int");
        REQUIRE(headers.empty());
    }

    TEST_CASE("double (built-in type)")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("double");
        REQUIRE(headers.empty());
    }

    TEST_CASE("bool (built-in type)")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("bool");
        REQUIRE(headers.empty());
    }
}

TEST_SUITE("HeaderMapper - Edge Cases")
{
    TEST_CASE("Empty token")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("");
        REQUIRE(headers.empty());
    }

    TEST_CASE("Token ending with _t but not integral")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("my_custom_t");
        REQUIRE(headers.empty());
    }

    TEST_CASE("Exact match takes precedence over integral check")
    {
        // Ensure exact match is checked first
        HeaderMapper mapper;
        auto headers = mapper.get_headers("size_t");
        REQUIRE(headers.size() == 1);
        CHECK_EQ(headers[0], "<cstddef>");
    }

    TEST_CASE("Single underscore")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("_");
        REQUIRE(headers.empty());
    }

    TEST_CASE("Double underscore")
    {
        HeaderMapper mapper;
        auto headers = mapper.get_headers("__");
        REQUIRE(headers.empty());
    }
}
} // anonymous namespace
