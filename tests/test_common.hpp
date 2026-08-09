// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_70AD3818F19147BD802AEA760A3BACF3
#define WJH_ATLAS_70AD3818F19147BD802AEA760A3BACF3

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace wjh::atlas::test {

/**
 * Find a working C++ compiler for compiling generated headers.
 * Result is cached after first call for performance.
 *
 * Set ATLAS_TEST_CXX to compile with a specific compiler instead of the first
 * one found on PATH. This matters because the search below picks `c++`, which
 * on macOS is Apple clang even when it is reached through a `g++` alias -- so
 * warnings that only one compiler raises (`-Wfloat-equal` on a cast to bool,
 * for instance) go unexercised unless that compiler is named explicitly:
 *
 *     ATLAS_TEST_CXX=g++-14 ctest --preset debug
 *
 * An unusable value is a hard failure rather than a silent fallback -- asking
 * for a compiler and quietly getting a different one defeats the purpose.
 *
 * @return Name of a working compiler
 * @throws std::runtime_error if ATLAS_TEST_CXX names a compiler that will not
 *         run
 */
inline std::string
find_working_compiler()
{
    static std::string cached_compiler = []() {
        auto can_run = [](std::string const & compiler) {
            std::string test_cmd = compiler + " --version >/dev/null 2>&1";
            return std::system(test_cmd.c_str()) == 0;
        };

        if (auto const * override_cxx = std::getenv("ATLAS_TEST_CXX")) {
            std::string compiler = override_cxx;
            if (not can_run(compiler)) {
                throw std::runtime_error(
                    "ATLAS_TEST_CXX is set to '" + compiler +
                    "', which does not run. Unset it or name a compiler on "
                    "PATH.");
            }
            return compiler;
        }

        // Try common C++ compilers in order
        for (auto const * compiler : {"c++", "g++", "clang++", "cl.exe"}) {
            if (can_run(compiler)) {
                return std::string(compiler);
            }
        }
        return std::string("c++"); // fallback
    }();
    return cached_compiler;
}

} // namespace wjh::atlas::test

#endif // WJH_ATLAS_70AD3818F19147BD802AEA760A3BACF3
