// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include "AtlasMain.hpp"

#include <cstdlib>
#include <sstream>
#include <string>

namespace {

using namespace wjh::atlas;

// Helper to capture stdout
struct StdoutCapture
{
    StdoutCapture() : old_buf(std::cout.rdbuf(buffer.rdbuf())) { }
    ~StdoutCapture() { std::cout.rdbuf(old_buf); }

    std::string get() const { return buffer.str(); }

private:
    std::stringstream buffer;
    std::streambuf * old_buf;
};

// Helper to capture stderr
struct StderrCapture
{
    StderrCapture() : old_buf(std::cerr.rdbuf(buffer.rdbuf())) { }
    ~StderrCapture() { std::cerr.rdbuf(old_buf); }

    std::string get() const { return buffer.str(); }

private:
    std::stringstream buffer;
    std::streambuf * old_buf;
};

} // anonymous namespace

TEST_SUITE("Atlas Main Tests")
{
    TEST_CASE("atlas_main outputs version with --version flag")
    {
        char const * argv[] = {"atlas", "--version"};

        StdoutCapture capture;
        int result = atlas_main(2, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);
        auto output = capture.get();
        CHECK(output.find("Atlas Strong Type Generator v") != std::string::npos);
    }

    TEST_CASE("atlas_main outputs warnings for redundant equality operators")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=TestType",
            "--description=strong int; <=>, ==, !="};

        StdoutCapture stdout_capture;
        StderrCapture stderr_capture;

        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);

        auto stderr_output = stderr_capture.get();
        CHECK(stderr_output.find("Warnings:") != std::string::npos);
        CHECK(stderr_output.find("TestType:") != std::string::npos);
        CHECK(
            stderr_output.find("makes '==' and '!=' redundant")
            != std::string::npos);
    }

    TEST_CASE("atlas_main outputs warnings for redundant relational operators")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=TestType",
            "--description=strong int; <=>, <, >, <=, >="};

        StdoutCapture stdout_capture;
        StderrCapture stderr_capture;

        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);

        auto stderr_output = stderr_capture.get();
        CHECK(stderr_output.find("Warnings:") != std::string::npos);
        CHECK(stderr_output.find("TestType:") != std::string::npos);
        CHECK(
            stderr_output.find("makes '<', '<=', '>', '>=' redundant")
            != std::string::npos);
    }

    TEST_CASE("atlas_main outputs multiple warnings")
    {
        char const * argv[] = {
            "atlas",
            "--kind=struct",
            "--namespace=test",
            "--name=MultiWarn",
            "--description=strong int; <=>, ==, !=, <, >, <=, >="};

        StdoutCapture stdout_capture;
        StderrCapture stderr_capture;

        int result = atlas_main(5, const_cast<char **>(argv));

        CHECK(result == EXIT_SUCCESS);

        auto stderr_output = stderr_capture.get();
        CHECK(stderr_output.find("Warnings:") != std::string::npos);
        CHECK(stderr_output.find("MultiWarn:") != std::string::npos);
        // Should have both warnings
        CHECK(
            stderr_output.find("makes '==' and '!=' redundant")
            != std::string::npos);
        CHECK(
            stderr_output.find("makes '<', '<=', '>', '>=' redundant")
            != std::string::npos);
    }
}
