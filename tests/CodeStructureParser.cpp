// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "CodeStructureParser.hpp"

#include <algorithm>
#include <sstream>

namespace wjh::atlas::testing {

CodeStructure
CodeStructureParser::
parse(std::string const & code)
{
    CodeStructure result;

    result.guard_name = extract_guard(code);
    result.includes = extract_includes(code);
    extract_type_info(code, result);
    extract_operators(code, result);
    extract_hash_info(code, result);

    return result;
}

std::string
CodeStructureParser::
extract_guard(std::string const & code)
{
    std::regex guard_regex{R"(#ifndef\s+([A-Z_a-z0-9]+))"};
    std::smatch match;
    if (std::regex_search(code, match, guard_regex)) {
        return match[1].str();
    }
    return "";
}

std::vector<std::string>
CodeStructureParser::
extract_includes(std::string const & code)
{
    std::vector<std::string> includes;
    std::regex include_regex{R"(#include\s+[<"]([^>"]+)[>"])"};

    auto begin = std::sregex_iterator(code.begin(), code.end(), include_regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        includes.push_back((*it)[0].str());
    }

    return includes;
}

void
CodeStructureParser::
extract_type_info(std::string const & code, CodeStructure & result)
{
    // Find struct/class declaration
    std::regex type_regex{
        R"((struct|class)\s+([A-Za-z_:][A-Za-z_0-9:]*)\s*(?::|))"};
    std::smatch match;

    // Search in the type-specific portion (after the marker)
    auto marker_pos = code.find(
        "/// These are the droids you are looking for!");
    std::string search_area = code;
    if (marker_pos != std::string::npos) {
        search_area = code.substr(marker_pos);
    }

    if (std::regex_search(search_area, match, type_regex)) {
        result.kind = match[1].str();
        result.type_name = match[2].str();
    }

    // Extract namespace - handle both C++11 nested and C++17 inline syntax
    result.namespace_name = extract_namespace_name(search_area);

    // Build fully qualified name
    if (result.namespace_name.empty()) {
        result.full_qualified_name = result.type_name;
    } else {
        result.full_qualified_name = result.namespace_name +
            "::" + result.type_name;
    }

    // Extract member variable - use simple string search for now
    auto value_pos = search_area.find(" value");
    if (value_pos != std::string::npos) {
        // Find the type before " value"
        auto line_start = search_area.rfind('\n', value_pos);
        if (line_start == std::string::npos) {
            line_start = 0;
        }
        auto type_decl = search_area.substr(line_start, value_pos - line_start);

        // Simple extraction: find last word before " value"
        auto last_space = type_decl.find_last_not_of(" \t\n");
        if (last_space != std::string::npos) {
            type_decl = type_decl.substr(0, last_space + 1);
        }
        // Remove leading whitespace
        auto first_non_space = type_decl.find_first_not_of(" \t\n");
        if (first_non_space != std::string::npos) {
            result.member_type = type_decl.substr(first_non_space);
        }
        result.member_name = "value";

        // Check for default value
        auto brace_open = search_area.find("value{", value_pos);
        if (brace_open != std::string::npos) {
            // Find matching closing brace, handling nested braces
            int depth = 1;
            size_t pos = brace_open + 6; // Start after "value{"
            while (pos < search_area.size() && depth > 0) {
                if (search_area[pos] == '{') {
                    ++depth;
                } else if (search_area[pos] == '}') {
                    --depth;
                }
                ++pos;
            }
            if (depth == 0) {
                result.member_default_value = search_area.substr(
                    brace_open + 6, // strlen("value{")
                    pos - 1 - (brace_open + 6)); // pos-1 is the closing brace
            }
        }
    }

    // Check for public: specifier
    result.has_public_specifier =
        (search_area.find("public:") != std::string::npos);
    result.has_private_specifier =
        (search_area.find("private:") != std::string::npos);

    // Check for constexpr constructor
    result.has_constexpr_constructor =
        (search_area.find("constexpr explicit " + result.type_name) !=
         std::string::npos);
}

void
CodeStructureParser::
extract_operators(std::string const & code, CodeStructure & result)
{
    // Simple approach: search for "operator X" patterns
    // IMPORTANT: Longer patterns must come before shorter ones!
    std::vector<std::string> op_symbols = {
        // Three-way comparison first
        "<=>",
        // Compound assignments (must come before binary ops)
        "+=",
        "-=",
        "*=",
        "/=",
        "%=",
        "&=",
        "|=",
        "^=",
        "<<=",
        ">>=",
        // Comparison operators (longer first)
        "==",
        "!=",
        "<=",
        ">=",
        "<",
        ">",
        // Shift operators
        "<<",
        ">>",
        // Increment/decrement
        "++",
        "--",
        // Arithmetic
        "+",
        "-",
        "*",
        "/",
        "%",
        // Bitwise
        "&",
        "|",
        "^",
        "~",
        // Pointer and member access
        "->",
        // Subscript and call
        "[]",
        "()",
        // Conversion operators
        "bool"};

    for (auto const & op_sym : op_symbols) {
        std::string pattern = "operator " + op_sym;
        size_t pos = 0;
        while ((pos = code.find(pattern, pos)) != std::string::npos) {
            // Make sure this is a complete operator match
            // (not part of a longer operator like << in <<=)
            size_t next_pos = pos + pattern.length();
            if (next_pos < code.length()) {
                char next_char = code[next_pos];
                // If next char could extend the operator, skip this match
                if (next_char == '=' || next_char == '+' || next_char == '-' ||
                    next_char == '<' || next_char == '>')
                {
                    pos = next_pos;
                    continue;
                }
                // Must be followed by whitespace or '(' for a valid operator
                // declaration
                if (next_char != ' ' && next_char != '\t' && next_char != '(' &&
                    next_char != '\n' && next_char != '\r')
                {
                    pos = next_pos;
                    continue;
                }
            }

            CodeStructure::Operator op;
            op.name = pattern;

            // Strategy: The declaration line comes AFTER the doc comment.
            // Pattern: "... } /**doc comment*/ friend constexpr operator X"
            // So we look for the most recent "*/" before this operator.

            size_t decl_start = 0;
            size_t search_start = (pos > 500) ? (pos - 500) : 0;

            // Find the end of the most recent comment block
            size_t comment_end = code.rfind("*/", pos);

            if (comment_end != std::string::npos &&
                comment_end >= search_start && comment_end < pos)
            {
                // Start right after the comment
                decl_start = comment_end + 2;
            } else {
                // No comment found, look for closing brace of previous function
                size_t after_brace = code.rfind('}', pos);
                if (after_brace != std::string::npos &&
                    after_brace >= search_start && after_brace < pos)
                {
                    decl_start = after_brace + 1;
                } else {
                    // Fallback: reasonable context
                    decl_start = search_start;
                }
            }

            // Get context from declaration start to operator keyword
            auto context = code.substr(decl_start, pos - decl_start);

            // Look for "friend" that appears on the same declaration (before
            // "operator") Must be a whole word, not part of another identifier
            op.is_friend = false;
            size_t friend_pos = context.find("friend");
            while (friend_pos != std::string::npos) {
                // Check it's a whole word
                bool is_word_start =
                    (friend_pos == 0 ||
                     not std::isalnum(
                         static_cast<unsigned char>(context[friend_pos - 1])) &&
                         context[friend_pos - 1] != '_');
                bool is_word_end =
                    (friend_pos + 6 >= context.length() ||
                     not std::isalnum(
                         static_cast<unsigned char>(context[friend_pos + 6])) &&
                         context[friend_pos + 6] != '_');

                if (is_word_start && is_word_end) {
                    op.is_friend = true;
                    break;
                }
                friend_pos = context.find("friend", friend_pos + 1);
            }

            // Similarly for constexpr
            op.is_constexpr = false;
            size_t constexpr_pos = context.find("constexpr");
            while (constexpr_pos != std::string::npos) {
                bool is_word_start =
                    (constexpr_pos == 0 ||
                     not std::isalnum(static_cast<unsigned char>(
                         context[constexpr_pos - 1])) &&
                         context[constexpr_pos - 1] != '_');
                bool is_word_end =
                    (constexpr_pos + 9 >= context.length() ||
                     not std::isalnum(static_cast<unsigned char>(
                         context[constexpr_pos + 9])) &&
                         context[constexpr_pos + 9] != '_');

                if (is_word_start && is_word_end) {
                    op.is_constexpr = true;
                    break;
                }
                constexpr_pos = context.find("constexpr", constexpr_pos + 1);
            }

            // Check for "= default" (look forward from operator to find it)
            op.is_default = false;
            size_t forward_end = std::min(pos + 200, code.length());
            auto forward_context = code.substr(pos, forward_end - pos);
            if (forward_context.find("= default") != std::string::npos) {
                op.is_default = true;
            }

            result.operators.push_back(op);
            pos += pattern.length();
        }
    }
}

void
CodeStructureParser::
extract_hash_info(std::string const & code, CodeStructure & result)
{
    // Check for hash specialization
    std::regex hash_regex{R"(struct\s+std::hash\<([^\>]+)\>)"};
    std::smatch match;
    if (std::regex_search(code, match, hash_regex)) {
        result.has_hash_specialization = true;

        // Check if hash operator is constexpr
        std::string hash_section = code.substr(match.position());
        result.hash_is_constexpr =
            (hash_section.find("constexpr std::size_t operator ()") !=
             std::string::npos);
    }
}

std::string
CodeStructureParser::
extract_namespace_name(std::string const & search_area)
{
    // Handle both C++11 nested and C++17 inline syntax
    // C++11: namespace foo {\nnamespace bar {
    // C++17: namespace foo::bar {
    std::regex ns_regex{R"(namespace\s+([A-Za-z_:][A-Za-z_0-9:]*)\s*\{)"};
    std::smatch match;
    std::vector<std::string> namespaces;

    // Find all namespace declarations before the struct/class
    auto ns_search = search_area;
    while (std::regex_search(ns_search, match, ns_regex)) {
        std::string ns_name = match[1].str();

        // Check if this namespace contains :: (C++17 style)
        if (ns_name.find("::") != std::string::npos) {
            // Already qualified, use it directly
            return ns_name;
        }

        // C++11 style - collect namespace names
        namespaces.push_back(ns_name);

        // Check if the next thing after the { is another namespace or the
        // struct/class
        auto next_pos = match.position() + match.length();
        if (next_pos >= ns_search.size()) {
            break;
        }
        ns_search = ns_search.substr(next_pos);

        // If we find struct/class after this namespace, we're done
        if (ns_search.find("struct ") != std::string::npos ||
            ns_search.find("class ") != std::string::npos)
        {
            // Check if struct/class comes before another namespace
            auto struct_pos = ns_search.find("struct ");
            auto class_pos = ns_search.find("class ");
            auto next_ns_pos = ns_search.find("namespace ");

            auto type_pos = std::min(
                struct_pos != std::string::npos ? struct_pos
                                                : std::string::npos,
                class_pos != std::string::npos ? class_pos : std::string::npos);

            if (next_ns_pos != std::string::npos &&
                (type_pos == std::string::npos || next_ns_pos < type_pos))
            {
                // Another namespace comes first, continue collecting
                continue;
            } else {
                // Type declaration comes first, stop here
                break;
            }
        }
    }

    // Reconstruct namespace from collected parts (C++11 nested style)
    if (namespaces.empty()) {
        return "";
    }

    // Build namespace_name from collected parts
    std::string result = namespaces[0];
    for (size_t i = 1; i < namespaces.size(); ++i) {
        result += "::" + namespaces[i];
    }
    return result;
}

} // namespace wjh::atlas::testing
