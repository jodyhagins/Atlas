// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"

#include <boost/uuid/detail/sha1.hpp>

#include <algorithm>
#include <cctype>

namespace wjh::atlas { inline namespace v1 {

std::string
get_sha1(std::string const & s)
{
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(s.data(), s.size());
    boost::uuids::detail::sha1::digest_type hash;
    sha1.get_digest(hash);

    std::string result;
    for (unsigned int x : hash) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%08x", x);
        result += buffer;
    }
    return result;
}

std::string
generate_header_guard(
    std::string const & prefix,
    std::string const & separator,
    std::string const & content_hash,
    bool upcase)
{
    std::string guard_prefix = prefix.empty() ? "ATLAS" : prefix;
    std::string guard = guard_prefix + separator + content_hash;

    if (upcase) {
        std::transform(
            guard.begin(),
            guard.end(),
            guard.begin(),
            [](unsigned char c) { return std::toupper(c); });
    }

    return guard;
}

}} // namespace wjh::atlas::v1
