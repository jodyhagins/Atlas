// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#include "AtlasParser.hpp"
#include "AtlasUtilities.hpp"
#include "ProfileSystem.hpp"
#include "TemplateSystem.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace wjh::atlas {

namespace {

// Helper: split comma-separated features and trim whitespace
std::vector<std::string>
split_features(std::string const & features_str)
{
    std::vector<std::string> features;
    std::stringstream ss(features_str);
    std::string feature;
    while (std::getline(ss, feature, ',')) {
        feature = trim(feature);
        if (not feature.empty()) {
            features.push_back(feature);
        }
    }
    return features;
}

// Helper: validate that a string is a valid C++ identifier
bool
is_valid_cpp_identifier(std::string const & id)
{
    if (id.empty()) {
        return false;
    }

    // First character must be letter or underscore
    if (not std::isalpha(static_cast<unsigned char>(id[0])) && id[0] != '_') {
        return false;
    }

    // Remaining characters must be alphanumeric or underscore
    for (size_t i = 1; i < id.size(); ++i) {
        if (not std::isalnum(static_cast<unsigned char>(id[i])) && id[i] != '_')
        {
            return false;
        }
    }

    return true;
}

// Validate that a string is a valid C++ namespace (may contain ::)
bool
is_valid_cpp_namespace(std::string const & ns)
{
    if (ns.empty()) {
        return true; // Empty namespace is valid (means global)
    }

    // Split by :: and validate each part
    std::string current_part;
    for (size_t i = 0; i < ns.size(); ++i) {
        if (i + 1 < ns.size() && ns[i] == ':' && ns[i + 1] == ':') {
            // Found separator
            if (not is_valid_cpp_identifier(current_part)) {
                return false;
            }
            current_part.clear();
            ++i; // Skip second ':'
        } else {
            current_part += ns[i];
        }
    }

    // Validate the last part
    return is_valid_cpp_identifier(current_part);
}

// Extract template parameter name from enable_if expression
// e.g., "std::is_floating_point<U>::value" -> "U"
std::string
extract_template_param_from_enable_if(
    std::string const & expr,
    int line_number,
    std::string const & filename)
{
    auto open_angle = expr.find('<');
    auto close_angle = expr.find('>');

    if (open_angle == std::string::npos || close_angle == std::string::npos ||
        close_angle <= open_angle)
    {
        throw AtlasParserError(
            "Cannot extract template parameter name from enable_if at line " +
            std::to_string(line_number) + " in " + filename +
            ". Expected pattern like: "
            "enable_if=std::is_floating_point<U>::value");
    }

    std::string param_name = trim(
        expr.substr(open_angle + 1, close_angle - open_angle - 1));

    // Handle nested templates by taking first identifier before comma
    // For "std::is_same<T, int>::value", extract "T"
    auto comma_pos = param_name.find(',');
    if (comma_pos != std::string::npos) {
        param_name = trim(param_name.substr(0, comma_pos));
    }

    if (param_name.empty()) {
        throw AtlasParserError(
            "Cannot extract template parameter name from enable_if at line " +
            std::to_string(line_number) + " in " + filename +
            ". Expected pattern like: "
            "enable_if=std::is_floating_point<U>::value");
    }

    return param_name;
}

// ====================================================================
// Helper functions for parse_type_definitions
// ====================================================================

struct SectionHeaderInfo
{
    std::string kind;
    std::string type_namespace;
    std::string name;

    // Template definition: [template Name Params...]
    bool is_template_definition = false;
    std::vector<std::string> template_params; // Parameter names

    // Template instantiation: [use TemplateName Args...]
    bool is_template_instantiation = false;
    std::string template_name; // Template to instantiate
    std::vector<std::string> template_args; // Argument values
};

// Open and validate input file for parsing
std::ifstream
open_type_definitions_file(std::string const & filename)
{
    if (filename.empty()) {
        throw AtlasParserError("No input file specified");
    }

    std::ifstream file(filename);
    if (not file) {
        throw AtlasParserError("Cannot open input file: " + filename);
    }

    return file;
}

// Parse section header like [type], [TypeName], [ns::TypeName], [struct Type]
SectionHeaderInfo
parse_section_header(
    std::string const & line,
    int line_number,
    std::string const & filename)
{
    SectionHeaderInfo info;

    // Extract content between brackets and trim whitespace
    std::string section_content = trim(line.substr(1, line.size() - 2));

    if (section_content.empty()) {
        throw AtlasParserError(
            "Empty section header at line " + std::to_string(line_number) +
            " in " + filename);
    }

    // Legacy syntax: [type]
    if (section_content == "type") {
        // Name and namespace will be specified in the section body
        return info;
    }

    // Template definition: [template Name Params...]
    if (section_content.size() >= 9 &&
        section_content.substr(0, 9) == "template ")
    {
        info.is_template_definition = true;

        // Parse: template Name Param1 Param2 ...
        std::string rest = trim(section_content.substr(9));
        if (rest.empty()) {
            throw AtlasParserError(
                "Missing template name in section header at line " +
                std::to_string(line_number) + " in " + filename);
        }

        // Split by whitespace
        std::vector<std::string> tokens;
        std::istringstream iss(rest);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            throw AtlasParserError(
                "Missing template name in section header at line " +
                std::to_string(line_number) + " in " + filename);
        }

        info.name = tokens[0]; // Template name

        if (not is_valid_cpp_identifier(info.name)) {
            throw AtlasParserError(
                "Invalid template name in section header at line " +
                std::to_string(line_number) + " in " + filename + ": '" +
                info.name + "'");
        }

        if (tokens.size() < 2) {
            throw AtlasParserError(
                "Template '" + info.name +
                "' must have at least one parameter at line " +
                std::to_string(line_number) + " in " + filename);
        }

        // Remaining tokens are parameters
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (not is_valid_cpp_identifier(tokens[i])) {
                throw AtlasParserError(
                    "Invalid parameter name '" + tokens[i] +
                    "' in template header at line " +
                    std::to_string(line_number) + " in " + filename);
            }
            info.template_params.push_back(tokens[i]);
        }

        return info;
    }

    // Template instantiation: [use TemplateName Args...]
    if (section_content.size() >= 4 && section_content.substr(0, 4) == "use ") {
        info.is_template_instantiation = true;

        // Parse: use TemplateName Arg1 Arg2 ...
        std::string rest = trim(section_content.substr(4));
        if (rest.empty()) {
            throw AtlasParserError(
                "Missing template name in 'use' section header at line " +
                std::to_string(line_number) + " in " + filename);
        }

        // Split by whitespace
        std::vector<std::string> tokens;
        std::istringstream iss(rest);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            throw AtlasParserError(
                "Missing template name in 'use' section header at line " +
                std::to_string(line_number) + " in " + filename);
        }

        info.template_name = tokens[0]; // Template name to instantiate

        if (not is_valid_cpp_identifier(info.template_name)) {
            throw AtlasParserError(
                "Invalid template name in 'use' section header at line " +
                std::to_string(line_number) + " in " + filename + ": '" +
                info.template_name + "'");
        }

        // Look for 'as' keyword for custom name
        // Syntax: [use Template Arg1 Arg2 as CustomName]
        size_t as_pos = tokens.size();
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] == "as") {
                as_pos = i;
                break;
            }
        }

        if (as_pos < tokens.size()) {
            // Arguments are tokens[1..as_pos)
            for (size_t i = 1; i < as_pos; ++i) {
                info.template_args.push_back(tokens[i]);
            }
            // Name is the token after 'as'
            if (as_pos + 1 < tokens.size()) {
                info.name = tokens[as_pos + 1];
                if (not is_valid_cpp_identifier(info.name)) {
                    throw AtlasParserError(
                        "Invalid type name after 'as' in 'use' section header "
                        "at line " +
                        std::to_string(line_number) + " in " + filename +
                        ": '" + info.name + "'");
                }
                // Warn if extra tokens after name
                if (as_pos + 2 < tokens.size()) {
                    throw AtlasParserError(
                        "Unexpected tokens after name in 'use' section header "
                        "at line " +
                        std::to_string(line_number) + " in " + filename);
                }
            } else {
                throw AtlasParserError(
                    "Missing name after 'as' in 'use' section header at line " +
                    std::to_string(line_number) + " in " + filename);
            }
        } else {
            // No 'as' - all remaining tokens are arguments
            for (size_t i = 1; i < tokens.size(); ++i) {
                info.template_args.push_back(tokens[i]);
            }
        }

        return info;
    }

    // New syntax: [TypeName], [ns::TypeName], [struct TypeName],
    // or [class ns::TypeName]

    // Check for optional kind prefix (struct or class)
    if (section_content == "struct" || section_content == "class") {
        // Just the keyword with no typename - error
        throw AtlasParserError(
            "Missing type name in section header at line " +
            std::to_string(line_number) + " in " + filename);
    } else if (
        section_content.size() >= 7 &&
        section_content.substr(0, 7) == "struct ")
    {
        info.kind = "struct";
        section_content = trim(section_content.substr(7));
    } else if (
        section_content.size() >= 6 && section_content.substr(0, 6) == "class ")
    {
        info.kind = "class";
        section_content = trim(section_content.substr(6));
    }

    // After removing kind prefix, check if anything remains
    if (section_content.empty()) {
        throw AtlasParserError(
            "Missing type name in section header at line " +
            std::to_string(line_number) + " in " + filename);
    }

    // Find last occurrence of ::
    auto last_colon_pos = section_content.rfind("::");

    if (last_colon_pos != std::string::npos) {
        // Qualified name: [ns::TypeName]
        info.type_namespace = trim(section_content.substr(0, last_colon_pos));
        info.name = trim(section_content.substr(last_colon_pos + 2));

        // Validate namespace
        if (not is_valid_cpp_namespace(info.type_namespace)) {
            throw AtlasParserError(
                "Invalid C++ namespace in section header at line " +
                std::to_string(line_number) + " in " + filename + ": '" +
                info.type_namespace + "'");
        }

        // Check for trailing :: (namespace with no name)
        if (info.name.empty()) {
            throw AtlasParserError(
                "Missing type name after namespace in section "
                "header at line " +
                std::to_string(line_number) + " in " + filename);
        }
    } else {
        // Unqualified name: [TypeName]
        info.name = section_content;
    }

    // Validate type name
    if (not is_valid_cpp_identifier(info.name)) {
        throw AtlasParserError(
            "Invalid C++ identifier in section header at line " +
            std::to_string(line_number) + " in " + filename + ": '" +
            info.name + "'");
    }

    return info;
}

// Parse and expand profile tokens in description
std::string
expand_profile_tokens(
    std::string const & description,
    ProfileSystem & profile_system,
    int line_number,
    std::string const & filename)
{
    try {
        auto parsed = parse_specification(description);

        // Expand {PROFILE} tokens in operators by merging profile specs
        for (auto const & op : parsed.operators) {
            // Check if this is a profile reference: {NAME}
            if (op.length() > 2 && op[0] == '{' && op[op.length() - 1] == '}') {
                // Extract profile name
                std::string profile_name = op.substr(1, op.length() - 2);

                // Look up and merge profile
                auto const & profile_spec = profile_system.get_profile(
                    profile_name);
                parsed.merge(profile_spec);
            }
        }

        // Remove {PROFILE} tokens from operators after merging
        std::set<std::string> final_operators;
        for (auto const & op : parsed.operators) {
            if (not (
                    op.length() > 2 && op[0] == '{' &&
                    op[op.length() - 1] == '}'))
            {
                final_operators.insert(op);
            }
        }

        // Reconstruct description: [strong] type; [forward=...;]
        // operators Only add "strong" if it was in the original
        std::string expanded_description = parsed.had_strong_keyword
            ? "strong " + parsed.first_part + ";"
            : parsed.first_part + ";";
        if (not parsed.forwards.empty()) {
            expanded_description += " forward=";
            for (size_t i = 0; i < parsed.forwards.size(); ++i) {
                if (i > 0) {
                    expanded_description += ",";
                }
                expanded_description += parsed.forwards[i];
            }
            expanded_description += ";";
        }
        if (not final_operators.empty()) {
            expanded_description += " ";
            bool first = true;
            // Sort operators for deterministic output
            std::vector<std::string> sorted_ops(
                final_operators.begin(),
                final_operators.end());
            std::sort(sorted_ops.begin(), sorted_ops.end());
            for (auto const & op : sorted_ops) {
                if (not first) {
                    expanded_description += ", ";
                }
                first = false;
                expanded_description += op;
            }
        }

        return expanded_description;
    } catch (std::exception const & e) {
        throw AtlasParserError(
            "Error parsing/expanding description near line " +
            std::to_string(line_number) + " in " + filename + ": " + e.what());
    }
}

// Parse file-level configuration key-value pair
void
parse_file_level_config(
    std::string const & key,
    std::string const & value,
    int line_number,
    std::string const & filename,
    FileParseResult & result,
    std::string & global_namespace,
    ProfileSystem & profile_system)
{
    if (key == "guard_prefix") {
        result.guard_prefix = value;
    } else if (key == "guard_separator") {
        result.guard_separator = value;
    } else if (key == "upcase_guard") {
        result.upcase_guard = parser_utils::parse_bool(value, "upcase_guard");
    } else if (key == "namespace") {
        global_namespace = value;
    } else if (key == "cpp_standard") {
        // File-level cpp_standard will be applied to all types later
        // Store it in a variable for now
        try {
            result.file_level_cpp_standard = parse_cpp_standard(value);
        } catch (std::invalid_argument const & e) {
            throw AtlasParserError(
                "Invalid cpp_standard at line " + std::to_string(line_number) +
                " in " + filename + ": " + e.what());
        }
    } else if (key == "profile") {
        // Parse profile=NAME; features...
        // value contains everything after the = sign (e.g.,
        // "STRING_LIKE; forward=size,empty; ==, !=")
        try {
            // Use parse_specification to parse the entire profile
            // definition
            auto parsed = parse_specification(value);
            // The first_part is the profile name
            profile_system.register_profile(parsed.first_part, parsed);
        } catch (std::exception const & e) {
            throw AtlasParserError(
                "Error parsing/registering profile at line " +
                std::to_string(line_number) + " in " + filename + ": " +
                e.what());
        }
    } else {
        throw AtlasParserError(
            "Unknown configuration key at line " + std::to_string(line_number) +
            " in " + filename + ": " + key);
    }
}

// Parse type-level configuration key-value pair
void
parse_type_level_config(
    std::string const & key,
    std::string const & value,
    int line_number,
    std::string const & filename,
    std::string & current_kind,
    std::string & current_namespace,
    std::string & current_name,
    std::string & current_description,
    std::string & current_default_value,
    std::vector<std::string> & current_constants,
    std::vector<std::string> & current_forward,
    SectionHeaderInfo const & section_info)
{
    if (key == "kind") {
        // Check for conflict with section-derived kind
        if (not section_info.kind.empty() && section_info.kind != value) {
            throw AtlasParserError(
                "Conflicting kind at line " + std::to_string(line_number) +
                " in " + filename + ": section header specifies '" +
                section_info.kind + "' but kind field specifies '" + value +
                "'");
        }
        current_kind = value;
    } else if (key == "namespace") {
        // Check for conflict with section-derived namespace
        if (not section_info.type_namespace.empty() &&
            section_info.type_namespace != value)
        {
            throw AtlasParserError(
                "Conflicting namespace at line " + std::to_string(line_number) +
                " in " + filename + ": section header specifies '" +
                section_info.type_namespace +
                "' but namespace field "
                "specifies '" +
                value + "'");
        }
        current_namespace = value;
    } else if (key == "name") {
        // Check for conflict with section-derived name
        if (not section_info.name.empty() && section_info.name != value) {
            throw AtlasParserError(
                "Conflicting name at line " + std::to_string(line_number) +
                " in " + filename + ": section header specifies '" +
                section_info.name + "' but name field specifies '" + value +
                "'");
        }
        current_name = value;
    } else if (key == "description") {
        current_description = value;
    } else if (key == "default_value") {
        current_default_value = value;
    } else if (key == "constants") {
        current_constants.push_back(
            value); // Accumulate multiple constants= lines
    } else if (key == "forward") {
        current_forward.push_back(value); // Accumulate multiple forward= lines
    } else {
        throw AtlasParserError(
            "Unknown type property at line " + std::to_string(line_number) +
            " in " + filename + ": " + key);
    }
}

// Build StrongTypeDescription from accumulated state
StrongTypeDescription
build_type_description(
    std::string const & current_kind,
    std::string const & current_namespace,
    std::string const & current_name,
    std::string const & current_description,
    std::string const & current_default_value,
    std::vector<std::string> const & current_constants,
    std::vector<std::string> const & current_forward,
    SectionHeaderInfo const & section_info,
    std::string const & global_namespace,
    ProfileSystem & profile_system,
    int line_number,
    std::string const & filename,
    FileParseResult const & result)
{
    // Use section-derived kind if current_kind is empty
    std::string effective_kind = current_kind.empty() ? section_info.kind
                                                      : current_kind;

    // Default to "struct" if kind is not specified
    if (effective_kind.empty()) {
        effective_kind = "struct";
    }

    // Use section-derived name if current_name is empty
    std::string effective_name = current_name.empty() ? section_info.name
                                                      : current_name;

    // Use section-derived namespace if current_namespace is empty,
    // then fall back to global namespace
    std::string effective_namespace = current_namespace.empty()
        ? (section_info.type_namespace.empty() ? global_namespace
                                               : section_info.type_namespace)
        : current_namespace;

    if (effective_namespace.empty() || effective_name.empty() ||
        current_description.empty())
    {
        throw AtlasParserError(
            "Incomplete type definition near line " +
            std::to_string(line_number) + " in " + filename);
    }

    // Parse description and expand profile tokens
    std::string expanded_description = expand_profile_tokens(
        current_description,
        profile_system,
        line_number,
        filename);

    // Merge all constants from multiple constants= lines
    auto constants = parser_utils::merge_constants(
        current_constants,
        "for type '" + effective_name + "' near line " +
            std::to_string(line_number));

    return StrongTypeDescription{
        .kind = effective_kind,
        .type_namespace = effective_namespace,
        .type_name = effective_name,
        .description = expanded_description,
        .default_value = current_default_value,
        .constants = constants,
        .guard_prefix = result.guard_prefix,
        .guard_separator = result.guard_separator,
        .upcase_guard = result.upcase_guard,
        .cpp_standard = result.file_level_cpp_standard,
        .forwarded_memfns = current_forward};
}

// Check if any type definition has been started
bool
has_started_type_definition(
    std::string const & current_kind,
    std::string const & current_namespace,
    std::string const & current_name,
    std::string const & current_description,
    SectionHeaderInfo const & section_info)
{
    return not section_info.name.empty() || not current_name.empty() ||
        not section_info.type_namespace.empty() ||
        not current_namespace.empty() || not current_description.empty() ||
        not section_info.kind.empty() || not current_kind.empty();
}

// ====================================================================
// Helper functions for parse_interactions
// ====================================================================

// Open and validate interactions file
std::ifstream
open_interactions_file(std::string const & filename)
{
    std::ifstream file(filename);
    if (not file) {
        throw AtlasParserError("Cannot open interaction file: " + filename);
    }
    return file;
}

// Check if line starts with a prefix
bool
starts_with(std::string const & str, std::string const & prefix)
{
    return str.size() >= prefix.size() &&
        str.substr(0, prefix.size()) == prefix;
}

// Extract value after '=' in a key=value line
std::string
extract_after_equals(std::string const & str)
{
    auto pos = str.find('=');
    if (pos == std::string::npos) {
        return "";
    }
    return trim(str.substr(pos + 1));
}

// Parse include directive
void
parse_include_directive(
    std::string const & line,
    int line_number,
    std::string const & filename,
    InteractionFileDescription & result)
{
    if (line == "include") {
        throw AtlasParserError(
            "Malformed include directive at line " +
            std::to_string(line_number) + " in " + filename +
            ". Expected: include <header> or include \"header\"");
    }

    std::string include = trim(line.substr(8));
    result.includes.push_back(include);
}

// Parse concept directive
void
parse_concept_directive(
    std::string const & line,
    int line_number,
    std::string const & filename,
    InteractionFileDescription & result,
    std::string & pending_concept_name)
{
    std::string value = extract_after_equals(line);
    if (value.empty()) {
        throw AtlasParserError(
            "Empty concept definition at line " + std::to_string(line_number) +
            " in " + filename +
            ". Expected: concept=<concept_expr> <param_name>");
    }

    // Space-separated syntax: "std::integral T"
    // The template parameter name is the last whitespace-separated
    // token
    auto last_space = value.rfind(' ');
    std::string name;
    std::string concept_expr;

    if (last_space != std::string::npos) {
        concept_expr = trim(value.substr(0, last_space));
        name = trim(value.substr(last_space + 1));
    } else {
        // No space - assume parameter name is "T"
        concept_expr = trim(value);
        name = "T";
    }

    if (name.empty()) {
        throw AtlasParserError(
            "Empty template parameter name at line " +
            std::to_string(line_number) + " in " + filename);
    }

    if (not result.constraints.contains(name)) {
        result.constraints[name] = TypeConstraint{.name = name};
    }
    result.constraints[name].concept_expr = concept_expr;
    pending_concept_name = name;
}

// Parse enable_if directive
void
parse_enable_if_directive(
    std::string const & line,
    int line_number,
    std::string const & filename,
    InteractionFileDescription & result,
    std::string & pending_concept_name)
{
    std::string expr = extract_after_equals(line);
    if (expr.empty()) {
        throw AtlasParserError(
            "Empty enable_if expression at line " +
            std::to_string(line_number) + " in " + filename +
            ". Expected: enable_if=<expression>");
    }

    if (not pending_concept_name.empty()) {
        // This enable_if belongs to the most recent concept
        result.constraints[pending_concept_name].enable_if_expr = expr;
        pending_concept_name.clear();
    } else {
        // No pending concept - extract parameter name from enable_if
        std::string param_name =
            extract_template_param_from_enable_if(expr, line_number, filename);

        if (not result.constraints.contains(param_name)) {
            result.constraints[param_name] = TypeConstraint{.name = param_name};
        }
        result.constraints[param_name].enable_if_expr = expr;
    }
}

// Parse interaction line: LHS OP RHS -> RESULT or LHS OP RHS <-> RESULT
InteractionDescription
parse_interaction_line(
    std::string const & line,
    int line_number,
    std::string const & filename,
    InteractionFileDescription const & result,
    std::string const & current_namespace,
    std::string const & current_value_access,
    std::string const & current_lhs_value_access,
    std::string const & current_rhs_value_access,
    bool current_constexpr)
{
    bool symmetric = line.find("<->") != std::string::npos;
    std::string arrow = symmetric ? "<->" : "->";

    auto arrow_pos = line.find(arrow);
    std::string left_side = trim(line.substr(0, arrow_pos));
    std::string result_type = trim(line.substr(arrow_pos + arrow.size()));

    // Parse left side: LHS OP RHS
    // Find operator - look for common operators
    std::vector<std::string> ops = {
        "<<",
        ">>",
        "==",
        "!=",
        "<=",
        ">=",
        "<=>",
        "&&",
        "||",
        "+",
        "-",
        "*",
        "/",
        "%",
        "&",
        "|",
        "^",
        "<",
        ">"};

    std::string lhs_type, rhs_type, op_symbol;
    for (auto const & op : ops) {
        auto op_pos = left_side.find(" " + op + " ");
        if (op_pos != std::string::npos) {
            lhs_type = trim(left_side.substr(0, op_pos));
            rhs_type = trim(left_side.substr(op_pos + op.size() + 2));
            op_symbol = op;
            break;
        }
    }

    if (lhs_type.empty() || rhs_type.empty() || op_symbol.empty()) {
        throw AtlasParserError(
            "Cannot parse interaction at line " + std::to_string(line_number) +
            " in " + filename + ": " + line);
    }

    if (result_type.empty()) {
        throw AtlasParserError(
            "Missing result type for interaction at line " +
            std::to_string(line_number) + " in " + filename + ": " + line);
    }

    // Check if types are constraints (templates)
    bool lhs_is_template = result.constraints.contains(lhs_type);
    bool rhs_is_template = result.constraints.contains(rhs_type);

    return InteractionDescription{
        .op_symbol = op_symbol,
        .lhs_type = lhs_type,
        .rhs_type = rhs_type,
        .result_type = result_type,
        .symmetric = symmetric,
        .lhs_is_template = lhs_is_template,
        .rhs_is_template = rhs_is_template,
        .is_constexpr = current_constexpr,
        .interaction_namespace = current_namespace,
        .lhs_value_access = current_lhs_value_access,
        .rhs_value_access = current_rhs_value_access,
        .value_access = current_value_access};
}

} // anonymous namespace

namespace parser_utils {

bool
parse_bool(std::string value, std::string const & option_name)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    throw AtlasParserError(
        "Invalid value for " + option_name + ": '" + value +
        "'. Expected true/false, 1/0, or yes/no.");
}

std::map<std::string, std::string>
parse_constants_string(
    std::string const & constants_str,
    std::string const & context)
{
    std::map<std::string, std::string> result;

    if (constants_str.empty()) {
        return result;
    }

    // Split by semicolon
    std::stringstream ss(constants_str);
    std::string constant;
    while (std::getline(ss, constant, ';')) {
        constant = trim(constant);
        if (constant.empty()) {
            continue;
        }

        // Split by first colon
        auto colon_pos = constant.find(':');
        if (colon_pos == std::string::npos) {
            throw AtlasParserError(
                "Invalid constant format " + context + ": '" + constant +
                "'. Expected 'name:value' format.");
        }

        std::string name = trim(constant.substr(0, colon_pos));
        std::string value = trim(constant.substr(colon_pos + 1));

        if (name.empty()) {
            throw AtlasParserError(
                "Empty constant name " + context + " in: '" + constant + "'");
        }

        // Validate name is a valid C++ identifier
        if (not is_valid_cpp_identifier(name)) {
            throw AtlasParserError(
                "Invalid constant name " + context + ": '" + name +
                "'. Must be a valid C++ identifier.");
        }

        // Check for duplicates
        if (result.find(name) != result.end()) {
            throw AtlasParserError(
                "Duplicate constant name " + context + ": '" + name + "'");
        }

        result[name] = value;
    }

    return result;
}

std::map<std::string, std::string>
merge_constants(
    std::vector<std::string> const & constants_strings,
    std::string const & context)
{
    std::map<std::string, std::string> result;

    for (auto const & constants_str : constants_strings) {
        auto parsed = parse_constants_string(constants_str, context);
        for (auto const & [name, value] : parsed) {
            if (result.find(name) != result.end()) {
                throw AtlasParserError(
                    "Duplicate constant name " + context + ": '" + name + "'");
            }
            result[name] = value;
        }
    }

    return result;
}

std::string
normalize_description(std::string const & description)
{
    auto semicolon_pos = description.find(';');
    if (semicolon_pos == std::string::npos) {
        return description;
    }

    std::string type_part = description.substr(0, semicolon_pos + 1);
    std::string rest = trim(description.substr(semicolon_pos + 1));

    if (rest.empty()) {
        return type_part;
    }

    // Check if there's a forward= section
    std::string forward_part;
    std::string features_str = rest;

    auto next_semicolon = rest.find(';');
    if (next_semicolon != std::string::npos) {
        auto first_segment = trim(rest.substr(0, next_semicolon));
        if (first_segment.find("forward=") == 0) {
            // This is a forward= section
            forward_part = first_segment + ";";
            features_str = trim(rest.substr(next_semicolon + 1));
        }
    }

    // Sort operator features only (not forward=)
    if (features_str.empty()) {
        return type_part + " " + forward_part;
    }

    auto features = split_features(features_str);
    std::sort(features.begin(), features.end());

    std::string result = type_part + " ";
    if (not forward_part.empty()) {
        result += forward_part + " ";
    }
    for (size_t i = 0; i < features.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += features[i];
    }
    return result;
}

} // namespace parser_utils

// AtlasFileParser implementation

FileParseResult
AtlasFileParser::
parse_type_definitions(
    std::string const & filename,
    std::string const & guard_prefix,
    std::string const & guard_separator,
    bool upcase_guard,
    int cli_cpp_standard)
{
    auto file = open_type_definitions_file(filename);

    FileParseResult result;
    result.guard_separator = guard_separator;
    result.upcase_guard = upcase_guard;

    // Profile system for user-defined profiles
    ProfileSystem profile_system;

    // Template system for user-defined type templates
    TemplateSystem template_system;

    std::string line;
    int line_number = 0;
    bool in_type_section = false;

    // Global namespace that applies to all types (unless overridden)
    std::string global_namespace;

    // Current type being parsed
    std::string current_kind;
    std::string current_namespace;
    std::string current_name;
    std::string current_description;
    std::string current_default_value;
    std::vector<std::string>
        current_constants; // Accumulate multiple constants= lines
    std::vector<std::string>
        current_forward; // Accumulate multiple forward= lines

    // Section-derived kind, namespace and name from [struct ns::Type] syntax
    SectionHeaderInfo section_info;

    auto finalize_type = [&]() {
        // Handle template definitions: register instead of adding to types
        if (section_info.is_template_definition) {
            // Require description for templates
            if (current_description.empty()) {
                throw AtlasParserError(
                    "Template '" + section_info.name +
                    "' missing required description near line " +
                    std::to_string(line_number) + " in " + filename);
            }

            TypeTemplate tmpl;
            tmpl.name = section_info.name;
            tmpl.parameters = section_info.template_params;
            tmpl.kind = current_kind.empty() ? "struct" : current_kind;
            tmpl.type_namespace = current_namespace;
            tmpl.description = current_description;
            tmpl.default_value = current_default_value;
            tmpl.constants = current_constants;
            tmpl.forwards = current_forward;

            template_system.register_template(tmpl, profile_system);

            current_kind.clear();
            current_namespace.clear();
            current_name.clear();
            current_description.clear();
            current_default_value.clear();
            current_constants.clear();
            current_forward.clear();
            section_info = SectionHeaderInfo{};
            return;
        }

        // Handle template instantiations
        if (section_info.is_template_instantiation) {
            auto const & tmpl = template_system.get_template(
                section_info.template_name);

            // Check argument count
            if (section_info.template_args.size() != tmpl.parameters.size()) {
                throw AtlasParserError(
                    "Template '" + section_info.template_name + "' expects " +
                    std::to_string(tmpl.parameters.size()) +
                    " argument(s) but got " +
                    std::to_string(section_info.template_args.size()) +
                    " near line " + std::to_string(line_number) + " in " +
                    filename);
            }

            // Substitute parameters in template fields
            std::string expanded_desc = substitute_template_params(
                tmpl.description,
                tmpl.parameters,
                section_info.template_args);

            std::string expanded_default = substitute_template_params(
                tmpl.default_value,
                tmpl.parameters,
                section_info.template_args);

            std::vector<std::string> expanded_constants;
            for (auto const & c : tmpl.constants) {
                expanded_constants.push_back(substitute_template_params(
                    c,
                    tmpl.parameters,
                    section_info.template_args));
            }

            std::vector<std::string> expanded_forwards;
            for (auto const & f : tmpl.forwards) {
                expanded_forwards.push_back(substitute_template_params(
                    f,
                    tmpl.parameters,
                    section_info.template_args));
            }

            // Add any instance-level constants (additive)
            for (auto const & c : current_constants) {
                expanded_constants.push_back(c);
            }

            // Override kind if specified in instance
            std::string effective_kind = current_kind.empty() ? tmpl.kind
                                                              : current_kind;

            // Override namespace if specified, else use template's, else global
            std::string effective_namespace = current_namespace.empty()
                ? (tmpl.type_namespace.empty() ? global_namespace
                                               : tmpl.type_namespace)
                : current_namespace;

            // Override default_value if specified in instance
            std::string effective_default = current_default_value.empty()
                ? expanded_default
                : current_default_value;

            // Generate a name from template_name + args (e.g., Optional_int)
            std::string generated_name = section_info.template_name;
            for (auto const & arg : section_info.template_args) {
                // Replace :: with _ for qualified names
                std::string safe_arg = arg;
                size_t pos = 0;
                while ((pos = safe_arg.find("::", pos)) != std::string::npos) {
                    safe_arg.replace(pos, 2, "_");
                    pos += 1;
                }
                generated_name += "_" + safe_arg;
            }

            // Use provided name if specified (header 'as' or body 'name='),
            // otherwise use generated name
            std::string effective_name = not section_info.name.empty()
                ? section_info.name
                : (current_name.empty() ? generated_name : current_name);

            // Build the type description using the expanded values
            SectionHeaderInfo synthetic_info;
            synthetic_info.kind = effective_kind;
            synthetic_info.type_namespace = effective_namespace;
            synthetic_info.name = effective_name;

            auto type_desc = build_type_description(
                effective_kind,
                effective_namespace,
                effective_name,
                expanded_desc,
                effective_default,
                expanded_constants,
                expanded_forwards,
                synthetic_info,
                global_namespace,
                profile_system,
                line_number,
                filename,
                result);

            result.types.push_back(type_desc);

            current_kind.clear();
            current_namespace.clear();
            current_name.clear();
            current_description.clear();
            current_default_value.clear();
            current_constants.clear();
            current_forward.clear();
            section_info = SectionHeaderInfo{};
            return;
        }

        // Check if we have started a regular type definition
        bool started = has_started_type_definition(
            current_kind,
            current_namespace,
            current_name,
            current_description,
            section_info);

        if (started) {
            auto type_desc = build_type_description(
                current_kind,
                current_namespace,
                current_name,
                current_description,
                current_default_value,
                current_constants,
                current_forward,
                section_info,
                global_namespace,
                profile_system,
                line_number,
                filename,
                result);

            result.types.push_back(type_desc);

            current_kind.clear();
            current_namespace.clear();
            current_name.clear();
            current_description.clear();
            current_default_value.clear();
            current_constants.clear();
            current_forward.clear();
            section_info = SectionHeaderInfo{};
        }
    };

    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check for section header: [type] or [TypeName] or [ns::TypeName]
        if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
            finalize_type();
            in_type_section = true;
            section_info = parse_section_header(line, line_number, filename);
            continue;
        }

        // Parse key=value
        auto equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            throw AtlasParserError(
                "Invalid format at line " + std::to_string(line_number) +
                " in " + filename +
                ": expected 'key=value' or section header like '[type]' or "
                "'[TypeName]'");
        }

        std::string key = trim(line.substr(0, equals_pos));
        std::string value = trim(line.substr(equals_pos + 1));

        // File-level configuration
        if (not in_type_section) {
            parse_file_level_config(
                key,
                value,
                line_number,
                filename,
                result,
                global_namespace,
                profile_system);
        } else { // Type-level configuration
            parse_type_level_config(
                key,
                value,
                line_number,
                filename,
                current_kind,
                current_namespace,
                current_name,
                current_description,
                current_default_value,
                current_constants,
                current_forward,
                section_info);
        }
    }

    // Finalize last type
    finalize_type();

    if (result.types.empty()) {
        throw AtlasParserError(
            "No type definitions found in input file: " + filename);
    }

    // Override with guard_prefix if provided
    if (not guard_prefix.empty()) {
        result.guard_prefix = guard_prefix;
    }

    // Override cpp_standard for all types if CLI flag is specified
    if (cli_cpp_standard > 0) {
        result.file_level_cpp_standard = cli_cpp_standard;
        for (auto & type : result.types) {
            type.cpp_standard = cli_cpp_standard;
        }
    }

    return result;
}

InteractionFileDescription
AtlasFileParser::
parse_interactions(std::string const & filename)
{
    auto file = open_interactions_file(filename);

    InteractionFileDescription result;
    std::string line;
    int line_number = 0;

    // State tracking
    std::string current_namespace;
    std::string current_value_access = "atlas::undress";
    std::string current_lhs_value_access; // Empty = use current_value_access
    std::string current_rhs_value_access; // Empty = use current_value_access
    bool current_constexpr = true;
    std::string pending_concept_name;

    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse directives
        if (starts_with(line, "include ")) {
            parse_include_directive(line, line_number, filename, result);
        } else if (starts_with(line, "concept=")) {
            parse_concept_directive(
                line,
                line_number,
                filename,
                result,
                pending_concept_name);
        } else if (starts_with(line, "enable_if=")) {
            parse_enable_if_directive(
                line,
                line_number,
                filename,
                result,
                pending_concept_name);
        } else if (starts_with(line, "namespace=")) {
            current_namespace = extract_after_equals(line);
        } else if (starts_with(line, "value_access=")) {
            current_value_access = extract_after_equals(line);
        } else if (starts_with(line, "lhs_value_access=")) {
            current_lhs_value_access = extract_after_equals(line);
        } else if (starts_with(line, "rhs_value_access=")) {
            current_rhs_value_access = extract_after_equals(line);
        } else if (starts_with(line, "guard_prefix=")) {
            result.guard_prefix = extract_after_equals(line);
        } else if (starts_with(line, "guard_separator=")) {
            result.guard_separator = extract_after_equals(line);
        } else if (starts_with(line, "upcase_guard=")) {
            result.upcase_guard = parser_utils::parse_bool(
                extract_after_equals(line),
                "upcase_guard");
        } else if (starts_with(line, "cpp_standard=")) {
            std::string standard_str = extract_after_equals(line);
            try {
                result.cpp_standard = parse_cpp_standard(standard_str);
            } catch (std::invalid_argument const & e) {
                throw AtlasParserError(
                    "Invalid cpp_standard at line " +
                    std::to_string(line_number) + " in " + filename + ": " +
                    e.what());
            }
        } else if (line == "constexpr") {
            current_constexpr = true;
        } else if (line == "no-constexpr") {
            current_constexpr = false;
        }
        // Parse interactions: LHS OP RHS -> RESULT or LHS OP RHS <-> RESULT
        else if (
            line.find("->") != std::string::npos ||
            line.find("<->") != std::string::npos)
        {
            auto interaction = parse_interaction_line(
                line,
                line_number,
                filename,
                result,
                current_namespace,
                current_value_access,
                current_lhs_value_access,
                current_rhs_value_access,
                current_constexpr);

            result.interactions.push_back(interaction);

            // Clear pending concept name after interaction is parsed
            pending_concept_name.clear();
        } else {
            throw AtlasParserError(
                "Unknown directive at line " + std::to_string(line_number) +
                " in " + filename + ": " + line);
        }
    }

    // Warn if no interactions were defined
    if (result.interactions.empty()) {
        throw AtlasParserError(
            "No interactions found in file: " + filename +
            ". Interaction files must contain at least one interaction "
            "(e.g., 'Type1 * Type2 -> Result').");
    }

    return result;
}

// AtlasCliParser implementation

AtlasCommandLine::Arguments
AtlasCliParser::
parse_arguments(std::vector<std::string> const & args)
{
    AtlasCommandLine::Arguments result;

    if (args.empty()) {
        throw AtlasParserError(
            "No arguments provided. Use --help for usage information.");
    }

    for (auto const & arg : args) {
        if (arg == "--help" || arg == "-h") {
            result.help = true;
            return result; // Early return for help
        }

        if (arg == "--version" || arg == "-v") {
            result.version = true;
            return result; // Early return for version
        }

        auto equals_pos = arg.find('=');
        if (equals_pos == std::string::npos || not arg.starts_with("--")) {
            throw AtlasParserError(
                "Invalid argument format: '" + arg +
                "'. Expected --key=value format.");
        }

        std::string key = arg.substr(2, equals_pos - 2); // Skip "--"
        std::string value = arg.substr(equals_pos + 1);

        if (key == "kind") {
            result.kind = value;
        } else if (key == "namespace") {
            result.type_namespace = value;
        } else if (key == "name") {
            result.type_name = value;
        } else if (key == "description") {
            result.description = value;
        } else if (key == "default-value") {
            result.default_value = value;
        } else if (key == "constants") {
            result.constants.push_back(
                value); // Accumulate multiple --constants flags
        } else if (key == "forward") {
            result.forwarded_memfns.push_back(
                value); // Accumulate multiple --forward flags
        } else if (key == "guard-prefix") {
            result.guard_prefix = value;
        } else if (key == "guard-separator") {
            result.guard_separator = value;
        } else if (key == "upcase-guard") {
            result.upcase_guard = parser_utils::parse_bool(
                value,
                "--upcase-guard");
        } else if (key == "input") {
            result.input_file = value;
        } else if (key == "output") {
            result.output_file = value;
        } else if (key == "interactions") {
            result.interactions_mode = parser_utils::parse_bool(
                value,
                "--interactions");
        } else if (key == "cpp-standard") {
            try {
                result.cpp_standard = parse_cpp_standard(value);
            } catch (std::invalid_argument const & e) {
                throw AtlasParserError(
                    "Invalid --cpp-standard value: " + std::string(e.what()));
            }
        } else {
            throw AtlasParserError("Unknown argument: --" + key);
        }
    }

    if (not result.help) {
        // Validate arguments using AtlasCommandLine's validation
        // (validation will be called from AtlasCommandLine::parse_impl)
    }

    return result;
}

StrongTypeDescription
AtlasCliParser::
arguments_to_description(AtlasCommandLine::Arguments const & args)
{
    if (args.help) {
        throw AtlasParserError(
            "Cannot convert help request to type description");
    }

    // Merge all constants from command-line flags
    auto constants = parser_utils::merge_constants(
        args.constants,
        "for type '" + args.type_name + "'");

    // Use CLI cpp_standard if specified, otherwise default to 11
    int cpp_standard = (args.cpp_standard > 0) ? args.cpp_standard : 11;

    return StrongTypeDescription{
        .kind = args.kind,
        .type_namespace = args.type_namespace,
        .type_name = args.type_name,
        .description = parser_utils::normalize_description(args.description),
        .default_value = args.default_value,
        .constants = constants,
        .guard_prefix = args.guard_prefix,
        .guard_separator = args.guard_separator,
        .upcase_guard = args.upcase_guard,
        .cpp_standard = cpp_standard,
        .forwarded_memfns = args.forwarded_memfns};
}

} // namespace wjh::atlas
