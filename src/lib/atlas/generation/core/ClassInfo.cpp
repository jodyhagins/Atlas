// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ClassInfo.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

#include "atlas/AtlasUtilities.hpp"
#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/TypeTokenizer.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace wjh::atlas::generation {

namespace {

// ==================================================
// Helper: Convert vector of to_json-able types to JSON array
// ==================================================
template <typename T>
boost::json::array
vector_to_json(std::vector<T> const & vec)
{
    boost::json::array result;
    result.reserve(vec.size());
    for (auto const & item : vec) {
        result.push_back(item.to_json());
    }
    return result;
}

// Specialization for string vectors - no to_json() needed
template <>
boost::json::array
vector_to_json<std::string>(std::vector<std::string> const & vec)
{
    boost::json::array result;
    result.reserve(vec.size());
    for (auto const & item : vec) {
        result.push_back(boost::json::value(item));
    }
    return result;
}

// ==================================================
// Helper: Convert string map to JSON object
// ==================================================
boost::json::object
map_to_json(std::map<std::string, std::string> const & m)
{
    boost::json::object result;
    for (auto const & [key, value] : m) {
        result[key] = value;
    }
    return result;
}

// ==================================================
// Parsing helper functions
// ==================================================

// Default predicate for strip - checks if character is whitespace
struct IsSpacePred
{
    bool operator () (unsigned char u) const
    {
        return std::isspace(static_cast<int>(u));
    }
};

template <typename PredT = IsSpacePred>
std::string_view
strip(std::string_view sv, PredT pred = PredT{})
{
    auto result = sv;
    while (not result.empty() &&
           pred(static_cast<unsigned char>(result.front())))
    {
        result.remove_prefix(1);
    }
    while (not result.empty() &&
           pred(static_cast<unsigned char>(result.back())))
    {
        result.remove_suffix(1);
    }
    return result;
}

std::vector<std::string_view>
split(std::string_view sv, char sep)
{
    std::vector<std::string_view> components;
    while (not sv.empty()) {
        while (not sv.empty() && std::isspace(sv.front())) {
            sv.remove_prefix(1);
        }
        auto n = std::min(sv.find(sep), sv.size());
        components.push_back(strip(sv.substr(0, n)));
        sv.remove_prefix(std::min(n + 1, sv.size()));
    }
    return components;
}

inline constexpr auto stripns = [](auto sv) {
    return strip(sv, [](unsigned char c) { return c == ':'; });
};

// Check for redundant operators when spaceship is present
void
check_for_redundant_operators(
    bool has_spaceship,
    bool has_equality_ops,
    bool has_relational_ops,
    ClassInfo const & info,
    std::vector<wjh::atlas::StrongTypeGenerator::Warning> * warnings)
{
    if (not has_spaceship || not warnings) {
        return;
    }

    auto type_name = info.class_namespace.empty()
        ? info.full_class_name
        : info.class_namespace + "::" + info.full_class_name;

    if (has_equality_ops) {
        warnings->push_back(
            {.message = "Operator '<=>' makes '==' and '!=' redundant. "
                        "Consider removing '==' and '!=' from the description.",
             .type_name = type_name});
    }

    if (has_relational_ops) {
        warnings->push_back(
            {.message =
                 "Operator '<=>' makes '<', '<=', '>', '>=' redundant. "
                 "Consider removing these operators from the description.",
             .type_name = type_name});
    }
}

// ============================================================================
// Forward Specification Parsing
// ============================================================================

/**
 * Result of parsing a forward specification like "size", "size:length",
 * "substr->Type", or "const"
 */
struct ForwardSpec
{
    std::string memfn_name = ""; // The actual memfn on the wrapped type
    std::string alias_name = ""; // Empty if no alias
    std::string return_type = ""; // Empty if no return type transformation
    bool is_const_marker = false; // True if this is the "const" keyword
};

/**
 * Parse a single forward specification string
 * Supports:
 *   - "memfn"                    : basic forwarding
 *   - "memfn:alias"              : forwarding with alias
 *   - "memfn->ReturnType"        : forwarding with return type transformation
 *   - "memfn:alias->ReturnType"  : forwarding with both alias and return type
 *   - "const"                    : marker for const-only forwarding
 */
ForwardSpec
parse_forward_spec(std::string const & forward_str)
{
    if (forward_str == "const") {
        return ForwardSpec{.is_const_marker = true};
    }

    // Local strip function
    auto strip_local = [](std::string_view sv) {
        while (not sv.empty() &&
               std::isspace(static_cast<unsigned char>(sv.front())))
        {
            sv.remove_prefix(1);
        }
        while (not sv.empty() &&
               std::isspace(static_cast<unsigned char>(sv.back())))
        {
            sv.remove_suffix(1);
        }
        return sv;
    };

    // First, check for return type transformation (->)
    std::string left_part = forward_str;
    std::string return_type;

    auto arrow_pos = forward_str.find("->");
    if (arrow_pos != std::string::npos) {
        // Check for multiple arrows (invalid)
        auto second_arrow = forward_str.find("->", arrow_pos + 2);
        if (second_arrow != std::string::npos) {
            throw std::invalid_argument(
                "Invalid forward return type syntax: '" + forward_str +
                "' (only one -> allowed)");
        }

        left_part = std::string(
            strip_local(std::string_view(forward_str).substr(0, arrow_pos)));
        return_type = std::string(
            strip_local(std::string_view(forward_str).substr(arrow_pos + 2)));

        if (left_part.empty()) {
            throw std::invalid_argument(
                "Invalid forward return type syntax: '" + forward_str +
                "' (missing memfn name before ->)");
        }

        if (return_type.empty()) {
            throw std::invalid_argument(
                "Invalid forward return type syntax: '" + forward_str +
                "' (missing return type after ->)");
        }
    }

    // Now check for alias (:) in the left part
    auto colon_pos = left_part.find(':');
    if (colon_pos != std::string::npos) {
        // Check for multiple colons (invalid)
        auto second_colon = left_part.find(':', colon_pos + 1);
        if (second_colon != std::string::npos) {
            throw std::invalid_argument(
                "Invalid forward alias syntax: '" + forward_str +
                "' (only one colon allowed in memfn:alias format)");
        }

        auto memfn = std::string(
            strip_local(std::string_view(left_part).substr(0, colon_pos)));
        auto alias = std::string(
            strip_local(std::string_view(left_part).substr(colon_pos + 1)));

        if (memfn.empty() || alias.empty()) {
            throw std::invalid_argument(
                "Invalid forward alias syntax: '" + forward_str +
                "' (format should be memfn:alias)");
        }

        return ForwardSpec{
            .memfn_name = memfn,
            .alias_name = alias,
            .return_type = return_type};
    }

    // No alias, just memfn (possibly with return type)
    return ForwardSpec{
        .memfn_name = left_part,
        .alias_name = "",
        .return_type = return_type};
}

// ============================================================================
// End of Forward Specification Parsing
// ============================================================================

/**
 * @brief Parse bounded-style constraint syntax and extract parameters as
 * strings
 * @param token The token string (e.g., "bounded<0,100>" or
 * "bounded_range<0,100>")
 * @return Map with "min" and "max" keys containing literal strings
 * @throws std::invalid_argument if syntax is invalid
 *
 * Note: This parser simply extracts the min/max values as strings.
 * The compiler will validate the values when compiling the generated trait.
 *
 * This function is used for both bounded<min,max> and bounded_range<min,max>
 * constraints, as they share identical parsing logic.
 */
std::map<std::string, std::string>
parse_bounded_params(std::string_view token)
{
    // Expected format: constraint_name<min,max>
    auto start = token.find('<');
    auto end = token.rfind('>');

    if (start == std::string_view::npos || end == std::string_view::npos ||
        end <= start)
    {
        throw std::invalid_argument(
            "Invalid bounded constraint syntax: expected "
            "'constraint<min,max>', got: " +
            std::string(token));
    }

    auto params_str = token.substr(start + 1, end - start - 1);
    auto comma = params_str.find(',');

    if (comma == std::string_view::npos) {
        throw std::invalid_argument(
            "Bounded constraint requires two parameters: constraint<min,max>, "
            "got: " +
            std::string(token));
    }

    std::map<std::string, std::string> result;
    result["min"] = trim(std::string(params_str.substr(0, comma)));
    result["max"] = trim(std::string(params_str.substr(comma + 1)));

    if (result["min"].empty() || result["max"].empty()) {
        throw std::invalid_argument(
            "Bounded constraint parameters cannot be empty: " +
            std::string(token));
    }

    return result;
}

// ============================================================================
// Token Processing State
// ============================================================================

/**
 * @brief State tracking for token processing during parse()
 *
 * Tracks which operator categories have been encountered to enable
 * validation and redundancy detection.
 */
struct TokenProcessingState
{
    bool has_spaceship = false;
    bool has_equality_ops = false;
    bool has_relational_ops = false;
    bool has_checked = false;
    bool has_saturating = false;
    bool has_wrapping = false;
};

// ============================================================================
// Token Processing Helper Functions
// ============================================================================

/**
 * @brief Process arithmetic binary and unary operator tokens
 * @return true if token was recognized and processed
 */
bool
process_arithmetic_operators(ClassInfo & info, std::string_view sv)
{
    if (OperatorParser::is_arithmetic_binary_operator(sv)) {
        if (sv.size() > 1u && sv[1] == '*') {
            sv.remove_suffix(1);
            info.arithmetic_binary_operators.emplace_back(sv);
            info.unary_operators.emplace_back(sv);
        } else {
            info.arithmetic_binary_operators.emplace_back(sv);
        }
        return true;
    }

    if (OperatorParser::is_arithmetic_unary_operator(sv)) {
        if (sv.size() > 1u) {
            sv.remove_prefix(1);
        }
        info.unary_operators.emplace_back(sv);
        return true;
    }

    return false;
}

/**
 * @brief Process logical operator tokens (!, ||, &&, not, or, and)
 * @return true if token was recognized and processed
 */
bool
process_logical_operators(ClassInfo & info, std::string_view sv)
{
    if (sv == "!" || sv == "not") {
        info.logical_not_operator = true;
        return true;
    }

    if (sv == "||" || sv == "or") {
        info.logical_operators.emplace_back("or");
        return true;
    }

    if (sv == "&&" || sv == "and") {
        info.logical_operators.emplace_back("and");
        return true;
    }

    return false;
}

/**
 * @brief Process increment/decrement operator tokens
 * @return true if token was recognized and processed
 */
bool
process_increment_operators(ClassInfo & info, std::string_view sv)
{
    if (sv == "++" || sv == "--") {
        info.increment_operators.emplace_back(sv);
        return true;
    }
    return false;
}

/**
 * @brief Process pointer-like operator tokens (@, &of, ->)
 * @return true if token was recognized and processed
 */
bool
process_pointer_operators(ClassInfo & info, std::string_view sv)
{
    if (sv == "@") {
        info.indirection_operator = true;
        return true;
    }

    if (sv == "&of") {
        info.addressof_operators.emplace_back("&");
        info.includes_vec.push_back("<memory>");
        return true;
    }

    if (sv == "->") {
        info.arrow_operator = true;
        info.includes_vec.push_back("<memory>");
        return true;
    }

    return false;
}

/**
 * @brief Process comparison operator tokens (<=>, ==, !=, <, <=, >, >=)
 * @return true if token was recognized and processed
 */
bool
process_comparison_operators(
    ClassInfo & info,
    std::string_view sv,
    TokenProcessingState & state)
{
    if (sv == "<=>") {
        state.has_spaceship = true;
        info.spaceship_operator = true;
        info.includes_vec.push_back("<compare>");
        return true;
    }

    if (OperatorParser::is_relational_operator(sv)) {
        if (sv == "==" || sv == "!=") {
            state.has_equality_ops = true;
        } else {
            state.has_relational_ops = true;
        }
        info.relational_operators.emplace_back(sv);
        return true;
    }

    return false;
}

/**
 * @brief Process I/O operator tokens (in, out)
 * @return true if token was recognized and processed
 */
bool
process_io_operators(ClassInfo & info, std::string_view sv)
{
    if (sv == "out") {
        info.ostream_operator = true;
        info.includes_vec.push_back("<ostream>");
        return true;
    }

    if (sv == "in") {
        info.istream_operator = true;
        info.includes_vec.push_back("<istream>");
        return true;
    }

    return false;
}

/**
 * @brief Process conversion operator tokens (bool, cast<T>, etc.)
 * @return true if token was recognized and processed
 */
bool
process_conversion_operators(ClassInfo & info, std::string_view sv)
{
    if (sv == "bool") {
        info.bool_operator = true;
        return true;
    }

    // Try to parse as cast operator
    bool is_implicit = false;
    std::string cast_type = OperatorParser::parse_cast_syntax(sv, is_implicit);

    if (not cast_type.empty()) {
        if (is_implicit) {
            info.desc.implicit_casts.push_back(std::move(cast_type));
        } else {
            info.desc.explicit_casts.push_back(std::move(cast_type));
        }
        return true;
    }

    return false;
}

/**
 * @brief Process callable and container operator tokens ((), (&), [])
 * @return true if token was recognized and processed
 */
bool
process_callable_operators(ClassInfo & info, std::string_view sv)
{
    if (sv == "()") {
        info.nullary = true;
        return true;
    }

    if (sv == "(&)") {
        info.callable = true;
        info.includes_vec.push_back("<utility>");
        info.includes_vec.push_back("<functional>");
        return true;
    }

    if (sv == "[]") {
        info.subscript_operator = true;
        return true;
    }

    return false;
}

/**
 * @brief Process specialization tokens (hash, no-constexpr-hash, fmt)
 * @return true if token was recognized and processed
 */
bool
process_specializations(ClassInfo & info, std::string_view sv)
{
    if (sv == "hash") {
        info.hash_specialization = true;
        info.includes_vec.push_back("<functional>");
        return true;
    }

    if (sv == "no-constexpr-hash") {
        info.hash_specialization = true;
        info.hash_const_expr = "";
        info.includes_vec.push_back("<functional>");
        return true;
    }

    if (sv == "fmt") {
        info.desc.generate_formatter = true;
        info.includes_vec.push_back("<format>");
        info.include_guards["<format>"] =
            "defined(__cpp_lib_format) && __cpp_lib_format >= 202110L";
        return true;
    }

    return false;
}

/**
 * @brief Process feature flag tokens (iterable, assign, no-constexpr)
 * @return true if token was recognized and processed
 */
bool
process_feature_flags(ClassInfo & info, std::string_view sv)
{
    if (sv == "iterable") {
        info.desc.generate_iterators = true;
        return true;
    }

    if (sv == "assign") {
        info.desc.generate_template_assignment = true;
        info.includes_vec.push_back("<concepts>");
        info.include_guards["<concepts>"] =
            "defined(__cpp_concepts) && __cpp_concepts >= 201907L";
        return true;
    }

    if (sv == "no-constexpr") {
        info.const_expr = "";
        info.hash_const_expr = "";
        return true;
    }

    return false;
}

/**
 * @brief Process arithmetic mode tokens (checked, saturating, wrapping)
 * @return true if token was recognized and processed
 */
bool
process_arithmetic_mode_tokens(
    ClassInfo & info,
    std::string_view sv,
    TokenProcessingState & state)
{
    if (sv == "checked") {
        state.has_checked = true;
        info.arithmetic_mode = ArithmeticMode::Checked;
        info.includes_vec.push_back("<limits>");
        info.includes_vec.push_back("<stdexcept>");
        info.includes_vec.push_back("<cmath>");
        return true;
    }

    if (sv == "saturating") {
        state.has_saturating = true;
        info.arithmetic_mode = ArithmeticMode::Saturating;
        info.includes_vec.push_back("<limits>");
        info.includes_vec.push_back("<cmath>");
        return true;
    }

    if (sv == "wrapping") {
        state.has_wrapping = true;
        info.arithmetic_mode = ArithmeticMode::Wrapping;
        return true;
    }

    return false;
}

/**
 * @brief Process simple constraint tokens (positive, non_negative, etc.)
 * @return true if token was recognized and processed
 */
bool
process_simple_constraints(ClassInfo & info, std::string_view sv)
{
    if (sv == "positive") {
        info.has_constraint = true;
        info.constraint_type = "positive";
        info.constraint_message = "value must be positive (> 0)";
        return true;
    }

    if (sv == "non_negative") {
        info.has_constraint = true;
        info.constraint_type = "non_negative";
        info.constraint_message = "value must be non-negative (>= 0)";
        return true;
    }

    if (sv == "non_zero") {
        info.has_constraint = true;
        info.constraint_type = "non_zero";
        info.constraint_message = "value must be non-zero (!= 0)";
        return true;
    }

    if (sv == "non_empty") {
        info.has_constraint = true;
        info.constraint_type = "non_empty";
        info.constraint_message = "value must not be empty";
        info.delete_default_constructor = true;
        return true;
    }

    if (sv == "non_null") {
        info.has_constraint = true;
        info.constraint_type = "non_null";
        info.constraint_message = "pointer must not be null";
        info.delete_default_constructor = true;
        return true;
    }

    return false;
}

/**
 * @brief Process bounded constraint tokens (bounded<min,max>,
 * bounded_range<min,max>)
 * @return true if token was recognized and processed
 */
bool
process_bounded_constraints(ClassInfo & info, std::string_view sv)
{
    if (sv.starts_with("bounded<") || sv.starts_with("bounded <") ||
        sv.starts_with("bounded_range<") || sv.starts_with("bounded_range <"))
    {
        info.has_constraint = true;
        info.is_bounded = true;

        // Determine constraint type and bracket style
        bool is_half_open = sv.starts_with("bounded_range");
        info.constraint_type = is_half_open ? "bounded_range" : "bounded";

        info.constraint_params = parse_bounded_params(sv);

        // Store min/max for template generation
        info.bounded_min = info.constraint_params["min"];
        info.bounded_max = info.constraint_params["max"];

        // Build human-readable message
        auto escaped = [](std::string const s) {
            std::string result;
            for (auto c : s) {
                if (c == '"') {
                    result += '\\';
                }
                result += c;
            }
            return result;
        };

        info.constraint_message = "value must be in [" +
            escaped(info.bounded_min) + ", " + escaped(info.bounded_max) +
            (is_half_open ? ")" : "]");

        return true;
    }

    return false;
}

/**
 * @brief Process include directives (#<header>)
 * @return true if token was recognized and processed
 */
bool
process_include_directive(ClassInfo & info, std::string_view sv)
{
    if (sv.starts_with('#')) {
        auto str = std::string(strip(sv.substr(1)));
        for (auto & c : str) {
            if (c == '\'') {
                c = '"';
            }
        }
        info.includes_vec.push_back(std::move(str));
        return true;
    }

    return false;
}

/**
 * @brief Process C++ standard specification tokens (c++20, C++23, etc.)
 * @return true if token was recognized and processed
 */
bool
process_cpp_standard(ClassInfo & info, std::string_view sv)
{
    if (sv.starts_with("c++") || sv.starts_with("C++")) {
        try {
            info.cpp_standard = parse_cpp_standard(sv);
            info.desc.cpp_standard = info.cpp_standard;
        } catch (std::invalid_argument const & e) {
            throw std::invalid_argument(
                "Invalid C++ standard in description: " +
                std::string(e.what()));
        }
        return true;
    }

    return false;
}

/**
 * @brief Process a single operator/feature token
 * @return true if token was recognized and processed
 *
 * This is the main dispatcher that tries all token processing functions
 * in sequence until one recognizes the token.
 */
bool
process_single_token(
    ClassInfo & info,
    std::string_view sv,
    TokenProcessingState & state)
{
    // Try each category of token processors
    if (process_arithmetic_operators(info, sv)) {
        return true;
    }
    if (process_logical_operators(info, sv)) {
        return true;
    }
    if (process_increment_operators(info, sv)) {
        return true;
    }
    if (process_pointer_operators(info, sv)) {
        return true;
    }
    if (process_comparison_operators(info, sv, state)) {
        return true;
    }
    if (process_io_operators(info, sv)) {
        return true;
    }
    if (process_conversion_operators(info, sv)) {
        return true;
    }
    if (process_callable_operators(info, sv)) {
        return true;
    }
    if (process_specializations(info, sv)) {
        return true;
    }
    if (process_feature_flags(info, sv)) {
        return true;
    }
    if (process_arithmetic_mode_tokens(info, sv, state)) {
        return true;
    }
    if (process_simple_constraints(info, sv)) {
        return true;
    }
    if (process_bounded_constraints(info, sv)) {
        return true;
    }
    if (process_include_directive(info, sv)) {
        return true;
    }
    if (process_cpp_standard(info, sv)) {
        return true;
    }

    return false; // Token not recognized
}

// ============================================================================
// End of Token Processing Helper Functions
// ============================================================================

// ============================================================================
// Post-Processing and Finalization Functions
// ============================================================================

/**
 * @brief Validate that arithmetic modes are mutually exclusive
 */
void
validate_arithmetic_modes(TokenProcessingState const & state)
{
    if (state.has_checked + state.has_saturating + state.has_wrapping > 1) {
        throw std::invalid_argument("Cannot specify multiple arithmetic modes "
                                    "(checked, saturating, wrapping)");
    }
}

/**
 * @brief Set constraint template arguments based on constraint type
 */
void
finalize_constraint_config(ClassInfo & info)
{
    if (info.has_constraint && not info.constraint_type.empty()) {
        if (info.constraint_type == "bounded" ||
            info.constraint_type == "bounded_range")
        {
            // For bounded constraints, use trait-based design
            info.constraint_template_args = "<atlas_bounds>";
        } else {
            // Other constraints just need the type
            info.constraint_template_args = "<" + info.underlying_type + ">";
        }
    }
}

/**
 * @brief Handle spaceship operator interactions with equality/relational
 * operators
 */
void
finalize_spaceship_operators(
    ClassInfo & info,
    TokenProcessingState const & state)
{
    if (not state.has_spaceship) {
        return;
    }

    // If spaceship is alone (no other relational operators), auto-generate
    // defaulted operator==
    if (not state.has_equality_ops && not state.has_relational_ops) {
        info.defaulted_equality_operator = true;
    }
    // If spaceship is with equality operators (== or !=), use defaulted
    // operator== instead of hand-written
    else if (state.has_equality_ops)
    {
        info.defaulted_equality_operator = true;
        // Remove == and != from relational_operators since we'll use
        // defaulted version
        info.relational_operators.erase(
            std::remove_if(
                info.relational_operators.begin(),
                info.relational_operators.end(),
                [](auto const & op) { return op.op == "==" || op.op == "!="; }),
            info.relational_operators.end());
    }
}

/**
 * @brief Process default value from StrongTypeDescription
 */
void
finalize_default_values(
    ClassInfo & info,
    wjh::atlas::StrongTypeDescription const & desc)
{
    if (not desc.default_value.empty()) {
        info.has_default_value = true;

        // Check if default_value references a constant name
        // If so, use the constant's expanded value instead
        std::string resolved_value = desc.default_value;
        for (auto const & constant : info.constants) {
            if (constant.name == desc.default_value) {
                resolved_value = constant.value;
                break;
            }
        }

        info.default_initializer = "{" + resolved_value + "}";
    }
}

/**
 * @brief Deduce, sort, and uniquify header includes
 */
void
finalize_includes(ClassInfo & info)
{
    // Deduce standard library headers from underlying type
    auto deduced_headers = deduce_headers_from_type(info.underlying_type);
    for (auto const & header : deduced_headers) {
        info.includes_vec.emplace_back(header);
    }

    // Also deduce headers from constant values and default values
    // (e.g., std::numeric_limits<int>::max() needs <limits>)
    for (auto const & constant : info.constants) {
        auto const_headers = deduce_headers_from_type(constant.value);
        for (auto const & header : const_headers) {
            info.includes_vec.emplace_back(header);
        }
    }

    if (not info.desc.default_value.empty()) {
        auto default_headers = deduce_headers_from_type(
            info.desc.default_value);
        for (auto const & header : default_headers) {
            info.includes_vec.emplace_back(header);
        }
    }

    // Add standard includes that are always needed
    info.includes_vec.push_back("<type_traits>");
    info.includes_vec.push_back("<utility>");

    // Sort and uniquify
    std::sort(info.includes_vec.begin(), info.includes_vec.end());
    info.includes_vec.erase(
        std::unique(info.includes_vec.begin(), info.includes_vec.end()),
        info.includes_vec.end());

    // Remove <version> if present
    info.includes_vec.erase(
        std::find(
            info.includes_vec.begin(),
            info.includes_vec.end(),
            "<version>"),
        info.includes_vec.end());
}

/**
 * @brief Propagate arithmetic mode to all arithmetic operators
 */
void
propagate_arithmetic_mode(ClassInfo & info)
{
    for (auto & op : info.arithmetic_binary_operators) {
        op.mode = info.arithmetic_mode;
    }
    for (auto & op : info.unary_operators) {
        op.mode = info.arithmetic_mode;
    }
}

/**
 * @brief Sort all operator vectors
 */
void
sort_operator_vectors(ClassInfo & info)
{
    for (auto * c :
         {&info.arithmetic_binary_operators,
          &info.unary_operators,
          &info.addressof_operators,
          &info.relational_operators,
          &info.logical_operators,
          &info.increment_operators})
    {
        std::sort(c->begin(), c->end());
    }
}

/**
 * @brief Set boolean flags based on vector contents
 */
void
set_boolean_flags(ClassInfo & info)
{
    info.has_relational_operators = not info.relational_operators.empty();
    info.has_explicit_casts = not info.explicit_cast_operators.empty();
    info.has_implicit_casts = not info.implicit_cast_operators.empty();
    info.has_forwarded_memfns = not info.forwarded_memfns.empty();
}

void
set_member_variable_name(ClassInfo & info)
{
    info.value_member_name = "value";
}

/**
 * @brief Build fully qualified name for specializations and checked arithmetic
 */
void
set_qualified_name(ClassInfo & info)
{
    if (info.hash_specialization || info.desc.generate_formatter ||
        not info.desc.constants.empty() ||
        info.arithmetic_mode == ArithmeticMode::Checked)
    {
        if (not info.class_namespace.empty()) {
            info.full_qualified_name = info.class_namespace +
                "::" + info.full_class_name;
        } else {
            info.full_qualified_name = info.full_class_name;
        }
    }
}

/**
 * @brief Enable optional features based on description flags
 */
void
enable_optional_features(ClassInfo & info)
{
    if (info.desc.generate_iterators) {
        info.iterator_support_member = true;
    }

    if (info.desc.generate_formatter) {
        info.formatter_specialization = true;
    }

    if (info.desc.generate_template_assignment) {
        info.template_assignment_operator = true;
    }
}

/**
 * @brief Expand special constant values (min/max) to std::numeric_limits
 * @param value The constant value to potentially expand
 * @param underlying_type The underlying type for the strong type
 * @param includes Vector to add <limits> to if expansion occurs
 * @return Expanded value or original if not a special value
 */
std::string
expand_special_constant_value(
    std::string_view value,
    std::string const & underlying_type,
    std::vector<std::string> & includes)
{
    if (value == "min" || value == "MIN") {
        includes.push_back("<limits>");
        return "std::numeric_limits<" + underlying_type + ">::min()";
    } else if (value == "max" || value == "MAX") {
        includes.push_back("<limits>");
        return "std::numeric_limits<" + underlying_type + ">::max()";
    } else {
        return std::string(value);
    }
}

/**
 * @brief Populate cast operator and constant vectors from description
 */
void
populate_casts_and_constants(ClassInfo & info)
{
    // Populate cast operators (filter out explicit casts that are also
    // implicit)
    for (auto const & cast_type : info.desc.explicit_casts) {
        if (std::find(
                info.desc.implicit_casts.begin(),
                info.desc.implicit_casts.end(),
                cast_type) == info.desc.implicit_casts.end())
        {
            info.explicit_cast_operators.emplace_back(cast_type);
        }
    }

    for (auto const & cast_type : info.desc.implicit_casts) {
        info.implicit_cast_operators.emplace_back(cast_type);
    }

    // Populate constants
    for (auto const & [name, value] : info.desc.constants) {
        std::string expanded_value = expand_special_constant_value(
            value,
            info.underlying_type,
            info.includes_vec);
        info.constants.emplace_back(name, expanded_value);

        if (name == "nil_value") {
            info.nil_value_is_constant = true;
        }
    }
}

/**
 * @brief Set const_qualifier based on const_expr setting
 */
void
set_const_qualifier(ClassInfo & info)
{
    if (info.const_expr.empty()) {
        info.const_qualifier = "const ";
    } else {
        info.const_qualifier = "constexpr ";
    }
}

// ============================================================================
// End of Post-Processing and Finalization Functions
// ============================================================================

// Expand nested namespaces for C++11 compatibility
// Input: "foo::bar::baz"
// Returns: opening/closing namespace split on multiple lines
std::pair<std::string, std::string>
expand_namespace(std::string const & ns)
{
    if (ns.empty()) {
        return {"", ""};
    }

    // Split namespace by "::"
    std::vector<std::string> parts;
    std::string_view sv = ns;
    while (not sv.empty()) {
        auto pos = sv.find("::");
        if (pos == std::string_view::npos) {
            parts.emplace_back(sv);
            break;
        }
        parts.emplace_back(sv.substr(0, pos));
        sv.remove_prefix(pos + 2);
    }

    // Build opening
    std::string opening;
    for (auto const & part : parts) {
        opening += "namespace ";
        opening += part;
        opening += " {\n";
    }

    // Build closing (in reverse order)
    std::string closing;
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        closing += "} // namespace ";
        closing += *it;
        closing += "\n";
    }

    return {opening, closing};
}

// Process forwarded memfns from description into ClassInfo
void
process_forwarded_memfns(ClassInfo & info)
{
    bool const_only = false;

    // Process each forward specification string from desc.forwarded_memfns
    // These can be either:
    // 1. Single items like "size", "size:length", "const" (from
    // parse_specification)
    // 2. Comma-separated lists like "size,empty,clear" (from separate forward=
    // lines)
    for (auto const & forward_str_raw : info.desc.forwarded_memfns) {
        // Validate that forward_str is not empty
        auto trimmed = strip(forward_str_raw);
        if (trimmed.empty()) {
            throw std::invalid_argument(
                "Empty forward= specification (forward= must be followed by "
                "memfn names)");
        }

        // The forward string might contain commas (from forward= lines in the
        // file) So we need to split by comma first IMPORTANT: Must store
        // trimmed as a string so the string_views from split() remain valid
        std::string trimmed_str(trimmed);
        for (auto memfn_spec_view : split(trimmed_str, ',')) {
            auto memfn_spec = std::string(strip(memfn_spec_view));

            if (memfn_spec.empty()) {
                continue; // Skip empty tokens
            }

            // Parse using our unified ForwardSpec parser
            ForwardSpec spec = parse_forward_spec(memfn_spec);

            // Check if it's the "const" marker
            if (spec.is_const_marker) {
                const_only = true;
                continue;
            }

            // It's a normal forward specification - create ForwardedMemfn
            ForwardedMemfn fm;
            fm.memfn_name = spec.memfn_name;
            fm.alias_name = spec.alias_name;
            fm.return_type = spec.return_type;
            fm.const_only = const_only;

            // Set template rendering flags based on const_only
            // Note: cpp23_or_later is not used anymore - we use feature test
            // macros instead
            if (fm.const_only) {
                // For const-only, just const (no ref qualifier) suffices
                fm.generate_const_no_ref = true;
                fm.generate_const_lvalue = false;
                fm.generate_const_rvalue = false;
                fm.generate_nonconst_lvalue = false;
                fm.generate_nonconst_rvalue = false;
            } else {
                // Generate all 4 ref-qualified overloads
                fm.generate_const_no_ref = false;
                fm.generate_const_lvalue = true;
                fm.generate_const_rvalue = true;
                fm.generate_nonconst_lvalue = true;
                fm.generate_nonconst_rvalue = true;
            }

            info.forwarded_memfns.push_back(std::move(fm));
        }
    }

    std::sort(
        info.forwarded_memfns.begin(),
        info.forwarded_memfns.end(),
        [](auto const & x, auto const & y) {
            auto const x_str = x.alias_name.empty() ? x.memfn_name
                                                    : x.alias_name;
            auto const y_str = y.alias_name.empty() ? y.memfn_name
                                                    : y.alias_name;
            return x_str < y_str;
        });
}

} // anonymous namespace

// ==================================================
// to_json() implementations
// ==================================================

boost::json::object
Operator::
to_json() const
{
    return {{"op", op}, {"mode", static_cast<int>(mode)}};
}

boost::json::object
CastOperator::
to_json() const
{
    return {{"cast_type", cast_type}};
}

boost::json::object
Constant::
to_json() const
{
    return {{"name", name}, {"value", value}};
}

boost::json::object
ForwardedMemfn::
to_json() const
{
    return {
        {"memfn_name", memfn_name},
        {"alias_name", alias_name},
        {"return_type", return_type},
        {"const_only", const_only},
        {"cpp23_or_later", cpp23_or_later},
        {"generate_const_no_ref", generate_const_no_ref},
        {"generate_const_lvalue", generate_const_lvalue},
        {"generate_const_rvalue", generate_const_rvalue},
        {"generate_nonconst_lvalue", generate_nonconst_lvalue},
        {"generate_nonconst_rvalue", generate_nonconst_rvalue}};
}

// ==================================================
// ClassInfo::to_json implementation
// The central serialization function for template rendering
// ==================================================
boost::json::object
ClassInfo::
to_json() const
{
    boost::json::object result;

    // Namespace and naming
    result["class_namespace"] = class_namespace;
    result["namespace_open"] = namespace_open;
    result["namespace_close"] = namespace_close;
    result["full_class_name"] = full_class_name;
    result["class_name"] = class_name;
    result["underlying_type"] = underlying_type;
    result["full_qualified_name"] = full_qualified_name;

    // Arithmetic operators - delegate to_json to vector elements
    result["arithmetic_binary_operators"] = vector_to_json(
        arithmetic_binary_operators);
    result["unary_operators"] = vector_to_json(unary_operators);

    // Pointer-like operators
    result["indirection_operator"] = indirection_operator;
    result["addressof_operators"] = vector_to_json(addressof_operators);
    result["arrow_operator"] = arrow_operator;

    // Comparison operators
    result["spaceship_operator"] = spaceship_operator;
    result["defaulted_equality_operator"] = defaulted_equality_operator;
    result["relational_operators"] = vector_to_json(relational_operators);
    result["has_relational_operators"] = has_relational_operators;

    // Increment/decrement
    result["increment_operators"] = vector_to_json(increment_operators);

    // Stream operators
    result["ostream_operator"] = ostream_operator;
    result["istream_operator"] = istream_operator;

    // Boolean conversion
    result["bool_operator"] = bool_operator;

    // Function-like operators
    result["nullary"] = nullary;
    result["callable"] = callable;

    // Access control
    result["public_specifier"] = public_specifier;

    // Logical operators
    result["logical_not_operator"] = logical_not_operator;
    result["logical_operators"] = vector_to_json(logical_operators);

    // Include management
    result["includes_vec"] = vector_to_json(includes_vec);
    result["include_guards"] = map_to_json(include_guards);

    // Specialization support
    result["hash_specialization"] = hash_specialization;
    result["formatter_specialization"] = formatter_specialization;

    // Container-like operators
    result["subscript_operator"] = subscript_operator;

    // Default value support
    result["has_default_value"] = has_default_value;
    result["default_initializer"] = default_initializer;

    // constexpr support
    result["const_expr"] = const_expr;
    result["hash_const_expr"] = hash_const_expr;

    // Member variable name
    result["value"] = value_member_name;

    // Iterator support
    result["iterator_support_member"] = iterator_support_member;

    // Template assignment
    result["template_assignment_operator"] = template_assignment_operator;

    // Cast operators
    result["explicit_cast_operators"] = vector_to_json(explicit_cast_operators);
    result["implicit_cast_operators"] = vector_to_json(implicit_cast_operators);
    result["has_explicit_casts"] = has_explicit_casts;
    result["has_implicit_casts"] = has_implicit_casts;

    // Named constants
    result["constants"] = vector_to_json(constants);

    // Forwarded member functions
    result["forwarded_memfns"] = vector_to_json(forwarded_memfns);
    result["has_forwarded_memfns"] = has_forwarded_memfns;

    // Additional qualifiers
    result["const_qualifier"] = const_qualifier;

    // C++ standard level
    result["cpp_standard"] = cpp_standard;

    // Arithmetic mode
    result["arithmetic_mode"] = static_cast<int>(arithmetic_mode);

    // Original description - manually serialize the fields
    // We can't use boost::json::value_from here because boost/json/src.hpp
    // can only be included in one translation unit (StrongTypeGenerator.cpp)
    boost::json::object desc_obj;
    desc_obj["kind"] = desc.kind;
    desc_obj["type_namespace"] = desc.type_namespace;
    desc_obj["type_name"] = desc.type_name;
    desc_obj["description"] = desc.description;
    desc_obj["default_value"] = desc.default_value;
    desc_obj["guard_prefix"] = desc.guard_prefix;
    desc_obj["guard_separator"] = desc.guard_separator;
    desc_obj["upcase_guard"] = desc.upcase_guard;
    desc_obj["generate_iterators"] = desc.generate_iterators;
    desc_obj["generate_formatter"] = desc.generate_formatter;
    desc_obj["cpp_standard"] = desc.cpp_standard;
    result["desc"] = desc_obj;

    // Constraint validation
    result["has_constraint"] = has_constraint;
    result["constraint_type"] = constraint_type;
    result["constraint_params"] = map_to_json(constraint_params);
    result["constraint_message"] = constraint_message;
    result["constraint_template_args"] = constraint_template_args;
    result["is_bounded"] = is_bounded;
    result["bounded_min"] = bounded_min;
    result["bounded_max"] = bounded_max;
    result["delete_default_constructor"] = delete_default_constructor;

    return result;
}

// ==================================================
// ClassInfo::parse implementation
// Parse a StrongTypeDescription into ClassInfo
// ==================================================

ClassInfo
ClassInfo::
parse(
    wjh::atlas::StrongTypeDescription const & desc,
    std::vector<wjh::atlas::StrongTypeGenerator::Warning> * warnings)
{
    ClassInfo info;
    info.desc = desc;
    info.cpp_standard = desc.cpp_standard;
    info.class_namespace = stripns(desc.type_namespace);

    // Expand nested namespaces for C++11 compatibility
    auto [ns_open, ns_close] = expand_namespace(info.class_namespace);
    info.namespace_open = ns_open;
    info.namespace_close = ns_close;

    info.full_class_name = stripns(desc.type_name);
    info.class_name = [&] {
        if (auto n = info.full_class_name.rfind(':');
            n < info.full_class_name.size())
        {
            return info.full_class_name.substr(n + 1);
        }
        return info.full_class_name;
    }();

    if (desc.kind == "class") {
        info.public_specifier = "public:";
    } else if (desc.kind != "struct") {
        throw std::invalid_argument("kind must be either class or struct");
    }

    // Parse the description string into operators and underlying type
    ParsedSpecification parsed_spec = parse_specification(desc.description);
    info.underlying_type = parsed_spec.first_part;

    // Collect forwarded member function specifications
    for (auto const & forward : parsed_spec.forwards) {
        info.desc.forwarded_memfns.push_back(forward);
    }

    // Process all operator tokens
    TokenProcessingState state;
    for (auto const & op_str : parsed_spec.operators) {
        std::string_view sv(op_str);
        if (sv.empty()) {
            continue;
        }

        if (not process_single_token(info, sv, state)) {
            throw std::invalid_argument(
                "Unrecognized operator or option in description: '" +
                std::string(sv) + "'");
        }
    }

    // Post-processing and finalization
    validate_arithmetic_modes(state);
    finalize_constraint_config(info);
    check_for_redundant_operators(
        state.has_spaceship,
        state.has_equality_ops,
        state.has_relational_ops,
        info,
        warnings);
    populate_casts_and_constants(info);
    finalize_spaceship_operators(info, state);
    finalize_default_values(info, desc);
    finalize_includes(info);
    propagate_arithmetic_mode(info);
    sort_operator_vectors(info);
    set_boolean_flags(info);
    set_member_variable_name(info);
    set_qualified_name(info);
    enable_optional_features(info);
    process_forwarded_memfns(info);
    set_const_qualifier(info);

    return info;
}

} // namespace wjh::atlas::generation
