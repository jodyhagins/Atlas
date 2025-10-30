// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "TokenToHeaderMapper.hpp"
#include "TypeTokenizer.hpp"

#include <algorithm>
#include <cctype>

namespace wjh::atlas { inline namespace v1 {

std::vector<std::string>
tokenize_type(std::string_view type_str)
{
    std::vector<std::string> tokens;
    std::string current_token;

    for (std::size_t i = 0; i < type_str.size(); ++i) {
        char c = type_str[i];

        if (std::isspace(c)) {
            // Check if next non-space character is ':'
            std::size_t j = i + 1;
            while (j < type_str.size() && std::isspace(type_str[j])) {
                ++j;
            }
            if (j < type_str.size() && type_str[j] == ':') {
                // Next is `:`, skip these spaces (part of namespace separator)
                i = j - 1; // Will be incremented by loop
                continue;
            }

            // Otherwise, spaces are token separators
            if (not current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            // Skip all consecutive spaces
            while (i + 1 < type_str.size() && std::isspace(type_str[i + 1])) {
                ++i;
            }
        } else if (std::isalnum(c) || c == '_') {
            // Accumulate alphanumeric and underscore characters
            current_token += c;
        } else if (
            c == ':' && i + 1 < type_str.size() && type_str[i + 1] == ':')
        {
            // Namespace separator - append to current token
            current_token += "::";
            ++i; // Skip the next ':'

            // Skip any spaces after '::' so we continue building the token
            while (i + 1 < type_str.size() && std::isspace(type_str[i + 1])) {
                ++i;
            }
        } else {
            // Other non-identifier character (< > , etc.)
            // Flush current token if not empty
            if (not current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
            // Skip this character
        }
    }

    // Flush any remaining token
    if (not current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

std::vector<std::string>
deduce_headers_from_type(std::string_view type_str)
{
    HeaderMapper mapper;
    auto tokens = tokenize_type(type_str);

    std::vector<std::string> headers;

    for (auto const & token : tokens) {
        auto token_headers = mapper.get_headers(token);
        headers.insert(
            headers.end(),
            token_headers.begin(),
            token_headers.end());
    }

    // Deduplicate and sort
    std::sort(headers.begin(), headers.end());
    headers.erase(std::unique(headers.begin(), headers.end()), headers.end());

    return headers;
}

}} // namespace wjh::atlas::v1
