// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "OperatorParser.hpp"

#include "atlas/AtlasUtilities.hpp"

#include <cctype>
#include <stdexcept>

namespace wjh::atlas::generation {

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

    // The type ends at the '>' matching the prefix's '<' -- same validation
    // as parse_from_atlas_cast_syntax, so a stray '>' or trailing text is a
    // generation-time error instead of invalid C++ in the emitted operator.
    auto name = std::string(prefix.substr(0, prefix.length() - 1));
    std::size_t depth = 1;
    std::size_t end = std::string_view::npos;
    for (std::size_t n = prefix.length(); n < token.size(); ++n) {
        if (token[n] == '<') {
            ++depth;
        } else if (token[n] == '>' && --depth == 0) {
            end = n;
            break;
        }
    }
    if (end == std::string_view::npos) {
        throw std::invalid_argument(
            "Unbalanced angle brackets in " + name +
            "<> syntax: " + std::string(token));
    }
    if (not strip(token.substr(end + 1)).empty()) {
        throw std::invalid_argument(
            "Unexpected text after '>' in " + name +
            "<> syntax: " + std::string(token));
    }

    auto type = strip(token.substr(prefix.length(), end - prefix.length()));
    if (type.empty()) {
        throw std::invalid_argument(
            "Invalid " + name + "<> syntax: " + std::string(token));
    }
    return std::string(type);
}

bool
OperatorParser::
parse_from_atlas_cast_syntax(
    std::string_view token,
    std::vector<std::string> & types)
{
    constexpr std::string_view prefix = "from_atlas_cast<";
    if (not token.starts_with(prefix)) {
        return false;
    }

    // The list ends at the '>' matching the prefix's '<'. Anything else --
    // a '>' that never brings the depth back to zero, or text after the one
    // that does -- would bake the imbalance into the emitted constraint and
    // surface as compile errors far from the .atlas typo.
    std::size_t depth = 1;
    std::size_t end = std::string_view::npos;
    for (std::size_t n = prefix.size(); n < token.size(); ++n) {
        if (token[n] == '<') {
            ++depth;
        } else if (token[n] == '>' && --depth == 0) {
            end = n;
            break;
        }
    }
    if (end == std::string_view::npos) {
        throw std::invalid_argument(
            "Unbalanced angle brackets in from_atlas_cast<> syntax: " +
            std::string(token));
    }
    if (not strip(token.substr(end + 1)).empty()) {
        throw std::invalid_argument(
            "Unexpected text after '>' in from_atlas_cast<> syntax: " +
            std::string(token));
    }

    auto list = token.substr(prefix.size(), end - prefix.size());

    // Split on commas at angle-bracket depth zero, so names like
    // some::Wrapper<int, 4> survive intact.
    for (auto name : split_top_level(list, ',')) {
        if (name.empty()) {
            throw std::invalid_argument(
                "Empty type name in from_atlas_cast<> list: " +
                std::string(token));
        }
        types.emplace_back(name);
    }

    return true;
}

} // namespace wjh::atlas::generation
