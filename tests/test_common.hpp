// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_70AD3818F19147BD802AEA760A3BACF3
#define WJH_ATLAS_70AD3818F19147BD802AEA760A3BACF3

#include <cstdlib>
#include <string>

namespace wjh::atlas::test {

/**
 * Find a working C++ compiler by trying common compiler names.
 * Result is cached after first call for performance.
 *
 * @return Name of a working compiler (c++, g++, clang++, or cl.exe)
 */
inline std::string
find_working_compiler()
{
    static std::string cached_compiler = []() {
        // Try common C++ compilers in order
        for (auto const * compiler : {"c++", "g++", "clang++", "cl.exe"}) {
            std::string test_cmd = std::string(compiler) +
                " --version >/dev/null 2>&1";
            if (std::system(test_cmd.c_str()) == 0) {
                return std::string(compiler);
            }
        }
        return std::string("c++"); // fallback
    }();
    return cached_compiler;
}

} // namespace wjh::atlas::test

#endif // WJH_ATLAS_70AD3818F19147BD802AEA760A3BACF3
