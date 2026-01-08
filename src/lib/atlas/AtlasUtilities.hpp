// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_8651ABC1F7E740D3960747B1195C51A7
#define WJH_ATLAS_8651ABC1F7E740D3960747B1195C51A7

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace wjh::atlas {

/**
 * @brief Unified structure for parsed specifications (descriptions and
 * profiles)
 *
 * This structure can represent either:
 * - A description: first_part is the type name (e.g., "std::string")
 * - A profile: first_part is the profile name (e.g., "STRING_LIKE")
 */
struct ParsedSpecification
{
    std::string first_part; // Type name (for descriptions) or profile name (for
                            // profiles)
    std::vector<std::string>
        forwards; // Raw forward specifications ("size", "size:length", "const")
                  // - ORDER MATTERS!
    std::set<std::string> operators; // All operator/feature tokens
    bool had_strong_keyword =
        false; // True if original spec had "strong" prefix

    /**
     * @brief Merge another ParsedSpecification into this one
     *
     * Used to combine profiles with descriptions. The description's first_part
     * takes precedence, while forwards and operators are merged (unioned).
     */
    void merge(ParsedSpecification const & other);
};

/**
 * @brief Parse a specification string (description or profile definition)
 *
 * Format: "first_part; [forward=memfns;] operators"
 *
 * Examples:
 *   "std::string; forward=size,empty; ==, !="
 *   "STRING_LIKE; forward=size,empty,clear; ==, !=, hash"
 *   "int; +, -, *"
 */
ParsedSpecification parse_specification(std::string_view spec);

/**
 * @brief Generate a header guard name
 *
 * @param prefix Guard prefix (empty = "ATLAS")
 * @param separator Separator between prefix and hash
 * @param content_hash SHA1 hash of content
 * @param upcase Whether to uppercase the entire guard
 * @return Complete header guard identifier
 */
std::string generate_header_guard(
    std::string const & prefix,
    std::string const & separator,
    std::string const & content_hash,
    bool upcase);

/**
 * @brief Trim whitespace from both ends of a string
 *
 * @param str Input string to trim
 * @return Trimmed string with leading and trailing whitespace removed
 */
std::string trim(std::string const & str);

/**
 * @brief Options for controlling what code is included in the preamble
 */
struct PreambleOptions
{
    bool include_arrow_operator_traits = false;
    bool include_dereference_operator_traits = false;
    bool include_checked_helpers = false;
    bool include_saturating_helpers = false;
    bool include_constraints = false;
    bool include_nilable_support = false;
    bool include_hash_drill = false;
    bool include_ostream_drill = false;
    bool include_istream_drill = false;
    bool include_format_drill = false;

    // Auto-generation options: enable automatic support for all atlas types
    bool auto_hash = false;
    bool auto_ostream = false;
    bool auto_istream = false;
    bool auto_format = false;
};

/**
 * @brief Get the list of header includes required by the preamble
 *
 * @param options Controls which optional preamble features are included
 * @return Vector of header file names (e.g., "<type_traits>", "<utility>")
 */
std::vector<std::string> get_preamble_includes(
    PreambleOptions const & options = {});

/**
 * @brief The code necessary in every generated file.
 */
std::string preamble(PreambleOptions options = {});

/**
 * @brief Parse C++ standard specification from string
 *
 * Accepts formats: "20", "c++20", "C++20"
 * Valid values: 11, 14, 17, 20, 23
 *
 * @param val String representation of C++ standard
 * @return Numeric C++ standard value (11, 14, 17, 20, or 23)
 * @throws std::invalid_argument if format is invalid or value is unsupported
 */
int parse_cpp_standard(std::string_view val);

/**
 * @brief Generate static assertion for C++ standard requirement
 *
 * Generates a static_assert that verifies the code is compiled with at least
 * the required C++ standard. For C++11 (minimum), returns empty string.
 *
 * @param standard Required C++ standard (11, 14, 17, 20, or 23)
 * @return Static assertion code, or empty string for C++11
 */
std::string generate_cpp_standard_assertion(int standard);

/**
 * @brief Check if a file descriptor supports ANSI color codes
 *
 * @param fd File descriptor to check (e.g., fileno(stderr))
 * @return true if the file descriptor is a color-capable TTY
 */
bool supports_color(int fd);

/**
 * @brief ANSI color codes for terminal output
 */
namespace color {
constexpr char const * red = "\033[31m";
constexpr char const * yellow = "\033[33m";
constexpr char const * reset = "\033[0m";
} // namespace color

} // namespace wjh::atlas

#endif // WJH_ATLAS_8651ABC1F7E740D3960747B1195C51A7
