// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_D41A5B3E8F2C4A9B1F7E3C6A9D2B5E8F
#define WJH_ATLAS_D41A5B3E8F2C4A9B1F7E3C6A9D2B5E8F

#include <string>
#include <string_view>
#include <vector>

namespace wjh::atlas { inline namespace v1 {

/**
 * Extract all identifiers and namespace-qualified tokens from a type
 * expression.
 *
 * This function parses a C++ type string and returns a list of
 * namespace-qualified tokens. Template brackets, commas, and spaces
 * are treated as token separators and are not included in results.
 *
 * Examples:
 * - "std::string"                     -> ["std::string"]
 * - "int"                             -> ["int"]
 * - "std::map<std::string, int>"      -> ["std::map", "std::string", "int"]
 * - "std::vector<std::vector<int>>"   -> ["std::vector", "std::vector", "int"]
 * - "std :: string"                   -> ["std::string"] (spaces ignored)
 * - ""                                -> []
 *
 * @param type_str The type string to tokenize
 * @return A vector of namespace-qualified token strings
 *
 * @note Tokens are extracted preserving namespace qualification (`::`).
 *       Non-identifier characters like `<`, `>`, `,`, and spaces are
 *       skipped during tokenization.
 */
std::vector<std::string> tokenize_type(std::string_view type_str);

/**
 * Deduce all required standard library headers for a given type
 * expression.
 *
 * This function combines tokenization and header mapping to identify
 * every header needed for a type. It handles:
 * - Exact type matches (e.g., `std::string` -> `<string>`)
 * - Integral type families (e.g., `int8_t` -> `<cstdint>`)
 * - Namespace prefixes that imply headers (e.g., `std::chrono::` -> `<chrono>`)
 *
 * Examples:
 * - "std::string"                          : ["<string>"]
 * - "int8_t"                               : ["<cstdint>"]
 * - "std::map<std::string, std::int64_t>"  : ["<cstdint>", "<map>", "<string>"]
 * - "std::chrono::nanoseconds"             : ["<chrono>"]
 * - "std::vector<std::vector<int>>"        : ["<vector>"]
 *
 * @param type_str The type string to analyze
 * @return A sorted, deduplicated vector of required headers
 *
 * @note Headers are returned in sorted order with duplicates removed.
 *       User-defined types are not matched and do not produce headers.
 */
std::vector<std::string> deduce_headers_from_type(std::string_view type_str);

}} // namespace wjh::atlas::v1

#endif // WJH_ATLAS_D41A5B3E8F2C4A9B1F7E3C6A9D2B5E8F
