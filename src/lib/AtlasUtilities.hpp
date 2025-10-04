// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_8651ABC1F7E740D3960747B1195C51A7
#define WJH_ATLAS_8651ABC1F7E740D3960747B1195C51A7

#include <string>

namespace wjh::atlas { inline namespace v1 {

/**
 * @brief Generate SHA1 hash of a string
 *
 * @param s Input string to hash
 * @return Hexadecimal string representation of the SHA1 hash
 */
std::string get_sha1(std::string const & s);

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
 * @brief The C++ class definition for atlas::strong_type_tag.
 */
std::string strong_type_tag_definition();

/**
 * @brief The code necessary in every generated file.
 */
std::string preamble();

}} // namespace wjh::atlas::v1

#endif // WJH_ATLAS_8651ABC1F7E740D3960747B1195C51A7
