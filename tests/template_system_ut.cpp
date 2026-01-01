// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "error_test_support.hpp"

#include <string>

#include "doctest.hpp"

using namespace wjh::atlas::testing;

namespace {

TEST_SUITE("Template System: Definition Errors")
{
    TEST_CASE("Template with no parameters")
    {
        auto result = test_input_content_error(R"(
[template NoParams]
kind=struct
description=strong int; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("parameter") != std::string::npos);
    }

    TEST_CASE("Template with invalid name - starts with digit")
    {
        auto result = test_input_content_error(R"(
[template 123Invalid T]
kind=struct
description=strong {T}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Invalid") != std::string::npos);
    }

    TEST_CASE("Template with invalid name - contains special chars")
    {
        auto result = test_input_content_error(R"(
[template My-Template T]
kind=struct
description=strong {T}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Invalid") != std::string::npos);
    }

    TEST_CASE("Template with invalid parameter name")
    {
        auto result = test_input_content_error(R"(
[template MyTemplate 123bad]
kind=struct
description=strong {123bad}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Invalid") != std::string::npos);
    }

    TEST_CASE("Template with duplicate parameter names")
    {
        auto result = test_input_content_error(R"(
[template MyTemplate T T]
kind=struct
description=strong {T}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Duplicate") != std::string::npos);
    }

    TEST_CASE("Template parameter conflicts with profile name")
    {
        auto result = test_input_content_error(R"(
profile=NUMERIC; +, -, *, /

[template MyTemplate NUMERIC]
kind=struct
description=strong {NUMERIC}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("conflict") != std::string::npos);
    }

    TEST_CASE("Duplicate template registration")
    {
        auto result = test_input_content_error(R"(
[template MyTemplate T]
kind=struct
description=strong {T}; ==

[template MyTemplate U]
kind=struct
description=strong {U}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("already") != std::string::npos);
    }
}

TEST_SUITE("Template System: Instantiation Errors")
{
    TEST_CASE("Use unknown template")
    {
        auto result = test_input_content_error(R"(
[use UnknownTemplate int]
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Unknown") != std::string::npos);
    }

    TEST_CASE("Use template with too few arguments")
    {
        auto result = test_input_content_error(R"(
[template Pair K V]
kind=struct
description=strong std::pair<{K}, {V}>; ==

[use Pair int]
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("argument") != std::string::npos ||
             error_msg.find("parameter") != std::string::npos));
    }

    TEST_CASE("Use template with too many arguments")
    {
        auto result = test_input_content_error(R"(
[template Optional T]
kind=struct
description=strong std::optional<{T}>; ==

[use Optional int string bool]
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(
            (error_msg.find("argument") != std::string::npos ||
             error_msg.find("parameter") != std::string::npos));
    }

    TEST_CASE("Use template before it is defined")
    {
        auto result = test_input_content_error(R"(
[use LaterTemplate int]

[template LaterTemplate T]
kind=struct
description=strong {T}; ==
)");

        CHECK(result.had_error());
        auto error_msg = result.stderr_output + result.stdout_output;
        CHECK(error_msg.find("Unknown") != std::string::npos);
    }
}

} // anonymous namespace
