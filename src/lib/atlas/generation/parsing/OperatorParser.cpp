// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "OperatorParser.hpp"

#include <cctype>
#include <stdexcept>

namespace wjh::atlas::generation { inline namespace v1 {

// Helper to strip leading and trailing whitespace from a string_view
static std::string_view
strip(std::string_view sv) noexcept
{
    // Strip leading whitespace
    while (not sv.empty() &&
           std::isspace(static_cast<unsigned char>(sv.front())))
    {
        sv.remove_prefix(1);
    }

    // Strip trailing whitespace
    while (not sv.empty() &&
           std::isspace(static_cast<unsigned char>(sv.back())))
    {
        sv.remove_suffix(1);
    }

    return sv;
}

std::string
OperatorParser::
parse_cast_syntax(std::string_view token, bool & is_implicit)
{
    constexpr std::string_view prefix_implicit = "implicit_cast<";
    constexpr std::string_view prefix_explicit = "explicit_cast<";
    constexpr std::string_view prefix_cast = "cast<";

    // Determine which prefix matches (check longest first to avoid ambiguity)
    std::string_view prefix;
    if (token.starts_with(prefix_implicit)) {
        is_implicit = true;
        prefix = prefix_implicit;
    } else if (token.starts_with(prefix_explicit)) {
        is_implicit = false;
        prefix = prefix_explicit;
    } else if (token.starts_with(prefix_cast)) {
        is_implicit = false;
        prefix = prefix_cast;
    } else {
        // Not a cast operator - return empty string
        return "";
    }

    // Extract type between < and >
    auto start = prefix.length();
    auto end = token.find_last_of('>');
    if (end == std::string_view::npos || end <= start) {
        throw std::invalid_argument(
            "Invalid " + std::string(prefix.substr(0, prefix.length() - 1)) +
            "> syntax: " + std::string(token));
    }

    // Return the type name with whitespace stripped
    return std::string(strip(token.substr(start, end - start)));
}

}} // namespace wjh::atlas::generation::v1
