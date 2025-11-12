// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6
#define WJH_ATLAS_A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace wjh::atlas {

/**
 * Maps C++ type tokens to their required standard library headers.
 *
 * This class maintains lookup tables for:
 * - Exact type matches (e.g., `std::string` -> `<string>`)
 * - Integral type families (e.g., `int8_t` -> `<cstdint>`)
 * - Namespace prefixes (e.g., `std::chrono::*` -> `<chrono>`)
 *
 * The mapping uses a priority-ordered strategy:
 * 1. Exact token match
 * 2. Integral type family (suffix-based detection)
 * 3. Namespace prefix match
 * 4. No match (user-defined type)
 */
class HeaderMapper
{
public:
    /**
     * Construct a HeaderMapper with default type-to-header mappings.
     */
    explicit HeaderMapper();

    /**
     * Get the headers required for a single token.
     *
     * Examples:
     * - "std::string"              -> ["<string>"]
     * - "std::vector"              -> ["<vector>"]
     * - "std::int64_t"             -> ["<cstdint>"]
     * - "int8_t"                   -> ["<cstdint>"]
     * - "std::chrono::nanoseconds" -> ["<chrono>"] (namespace prefix match)
     * - "MyCustomType"             -> [] (unknown type)
     *
     * @param token A namespace-qualified token (e.g., "std::string")
     * @return A vector of required headers (may be empty)
     */
    std::vector<std::string> get_headers(std::string_view token) const;

private:
    std::map<std::string_view, std::string_view> exact_matches_;
    std::vector<std::pair<std::string_view, std::string_view>>
        namespace_prefixes_;

    void init_exact_matches();
    void init_namespace_prefixes();
    std::vector<std::string> check_integral_type(std::string_view) const;
    std::vector<std::string> check_namespace_prefix(std::string_view) const;
};

} // namespace wjh::atlas

#endif // WJH_ATLAS_A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6
