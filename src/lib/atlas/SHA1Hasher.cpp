// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "SHA1Hasher.hpp"

#include <boost/uuid/detail/sha1.hpp>

#include <cstdio>
#include <string>

namespace wjh::atlas {

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

} // namespace wjh::atlas
