// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_E2A965BF728E4AF4830FA2458F1477A5
#define WJH_ATLAS_E2A965BF728E4AF4830FA2458F1477A5

#include <string>

namespace wjh::atlas { inline namespace v1 {

/**
 * @brief Generate SHA1 hash of a string
 *
 * @param s Input string to hash
 * @return Hexadecimal string representation of the SHA1 hash
 */
std::string get_sha1(std::string const & s);

}} // namespace wjh::atlas::v1

#endif // WJH_ATLAS_E2A965BF728E4AF4830FA2458F1477A5
