// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasCommandLine.hpp"
#include "AtlasUtilities.hpp"
#include "ProfileSystem.hpp"

#include <boost/uuid/detail/sha1.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace wjh::atlas { inline namespace v1 {

namespace {

bool
parse_bool(std::string value, std::string const & option_name)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) { return std::tolower(c); });
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    throw AtlasCommandLineError(
        "Invalid value for " + option_name + ": '" + value +
        "'. Expected true/false, 1/0, or yes/no.");
}

// Split comma-separated features and trim whitespace
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

// Normalize description by sorting features for deterministic output
// Handles: type; [forward=...;] operators
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
        throw AtlasCommandLineError(
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
        throw AtlasCommandLineError(
            "Cannot extract template parameter name from enable_if at line " +
            std::to_string(line_number) + " in " + filename +
            ". Expected pattern like: "
            "enable_if=std::is_floating_point<U>::value");
    }

    return param_name;
}

// Validate that a string is a valid C++ identifier
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

// Parse constants string format: "name1:value1; name2:value2"
// Returns map of constant name to value
// Throws on invalid format or duplicate names
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
            throw AtlasCommandLineError(
                "Invalid constant format " + context + ": '" + constant +
                "'. Expected 'name:value' format.");
        }

        std::string name = trim(constant.substr(0, colon_pos));
        std::string value = trim(constant.substr(colon_pos + 1));

        if (name.empty()) {
            throw AtlasCommandLineError(
                "Empty constant name " + context + " in: '" + constant + "'");
        }

        // Validate name is a valid C++ identifier
        if (not is_valid_cpp_identifier(name)) {
            throw AtlasCommandLineError(
                "Invalid constant name " + context + ": '" + name +
                "'. Must be a valid C++ identifier.");
        }

        // Check for duplicates
        if (result.find(name) != result.end()) {
            throw AtlasCommandLineError(
                "Duplicate constant name " + context + ": '" + name + "'");
        }

        result[name] = value;
    }

    return result;
}

// Merge constants from multiple sources, checking for duplicates
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
                throw AtlasCommandLineError(
                    "Duplicate constant name " + context + ": '" + name + "'");
            }
            result[name] = value;
        }
    }

    return result;
}

} // anonymous namespace

AtlasCommandLine::Arguments
AtlasCommandLine::
parse(int argc, char const * const * argv)
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) { // Skip program name
        args.emplace_back(argv[i]);
    }
    return parse(args);
}

AtlasCommandLine::Arguments
AtlasCommandLine::
parse(std::vector<std::string> const & args)
{
    return parse_impl(args);
}

AtlasCommandLine::Arguments
AtlasCommandLine::
parse_impl(std::vector<std::string> const & args)
{
    Arguments result;

    if (args.empty()) {
        throw AtlasCommandLineError(
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
            throw AtlasCommandLineError(
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
            result.upcase_guard = parse_bool(value, "--upcase-guard");
        } else if (key == "input") {
            result.input_file = value;
        } else if (key == "output") {
            result.output_file = value;
        } else if (key == "interactions") {
            result.interactions_mode = parse_bool(value, "--interactions");
        } else if (key == "cpp-standard") {
            try {
                result.cpp_standard = parse_cpp_standard(value);
            } catch (std::invalid_argument const & e) {
                throw AtlasCommandLineError(
                    "Invalid --cpp-standard value: " + std::string(e.what()));
            }
        } else {
            throw AtlasCommandLineError("Unknown argument: --" + key);
        }
    }

    if (not result.help) {
        validate_arguments(result);
    }

    return result;
}

void
AtlasCommandLine::
validate_arguments(Arguments const & args)
{
    // Interactions mode requires an input file
    if (args.interactions_mode && args.input_file.empty()) {
        throw AtlasCommandLineError(
            "Interactions mode (--interactions=true) requires an input file. "
            "Use --input=<file> to specify the interaction file.");
    }

    // If input file is specified, we don't need command line type arguments
    if (not args.input_file.empty()) {
        // Input file mode - no command line type arguments required
        return;
    }

    std::vector<std::string> missing;

    if (args.kind.empty()) {
        missing.push_back("--kind");
    }
    if (args.type_namespace.empty()) {
        missing.push_back("--namespace");
    }
    if (args.type_name.empty()) {
        missing.push_back("--name");
    }
    if (args.description.empty()) {
        missing.push_back("--description");
    }

    if (not missing.empty()) {
        std::ostringstream oss;
        oss << "Missing required arguments: ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << missing[i];
        }
        throw AtlasCommandLineError(oss.str());
    }

    // Validate kind
    if (args.kind != "struct" && args.kind != "class") {
        throw AtlasCommandLineError(
            "Invalid --kind value: '" + args.kind +
            "'. Expected 'struct' or 'class'.");
    }

    // Validate namespace (basic check for valid C++ identifier pattern)
    if (not std::all_of(
            args.type_namespace.begin(),
            args.type_namespace.end(),
            [](char c) { return std::isalnum(c) || c == '_' || c == ':'; }))
    {
        throw AtlasCommandLineError(
            "Invalid --namespace value: '" + args.type_namespace +
            "'. Must contain only alphanumeric characters, underscores, and "
            "colons.");
    }

    // Validate type name (basic check for valid C++ identifier)
    if (args.type_name.empty() || std::isdigit(args.type_name[0]) ||
        not std::all_of(
            args.type_name.begin(),
            args.type_name.end(),
            [](char c) { return std::isalnum(c) || c == '_' || c == ':'; }))
    {
        throw AtlasCommandLineError(
            "Invalid --name value: '" + args.type_name +
            "'. Must be a valid C++ identifier.");
    }
}

StrongTypeDescription
AtlasCommandLine::
to_description(Arguments const & args)
{
    if (args.help) {
        throw AtlasCommandLineError(
            "Cannot convert help request to type description");
    }

    // Merge all constants from command-line flags
    auto constants = merge_constants(
        args.constants,
        "for type '" + args.type_name + "'");

    // Use CLI cpp_standard if specified, otherwise default to 11
    int cpp_standard = (args.cpp_standard > 0) ? args.cpp_standard : 11;

    return StrongTypeDescription{
        .kind = args.kind,
        .type_namespace = args.type_namespace,
        .type_name = args.type_name,
        .description = normalize_description(args.description),
        .default_value = args.default_value,
        .constants = constants,
        .guard_prefix = args.guard_prefix,
        .guard_separator = args.guard_separator,
        .upcase_guard = args.upcase_guard,
        .cpp_standard = cpp_standard,
        .forwarded_memfns = args.forwarded_memfns};
}

AtlasCommandLine::FileGenerationResult
AtlasCommandLine::
parse_input_file(Arguments const & args)
{
    if (args.input_file.empty()) {
        throw AtlasCommandLineError("No input file specified");
    }

    std::ifstream file(args.input_file);
    if (not file) {
        throw AtlasCommandLineError(
            "Cannot open input file: " + args.input_file);
    }

    FileGenerationResult result;
    result.guard_separator = args.guard_separator;
    result.upcase_guard = args.upcase_guard;

    // Profile system for user-defined profiles
    ProfileSystem profile_system;

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
    std::string section_derived_kind;
    std::string section_derived_namespace;
    std::string section_derived_name;

    auto finalize_type = [&]() {
        // Use section-derived kind if current_kind is empty
        std::string effective_kind = current_kind.empty() ? section_derived_kind
                                                          : current_kind;

        // Check if we have started a type definition (by having any of the
        // identifying fields set)
        bool started_type_definition = not section_derived_name.empty() ||
            not current_name.empty() || not section_derived_namespace.empty() ||
            not current_namespace.empty() || not current_description.empty() ||
            not section_derived_kind.empty() || not current_kind.empty();

        if (started_type_definition) {
            // Default to "struct" if kind is not specified
            if (effective_kind.empty()) {
                effective_kind = "struct";
            }

            // Use section-derived name if current_name is empty
            std::string effective_name = current_name.empty()
                ? section_derived_name
                : current_name;

            // Use section-derived namespace if current_namespace is empty,
            // then fall back to global namespace
            std::string effective_namespace = current_namespace.empty()
                ? (section_derived_namespace.empty()
                       ? global_namespace
                       : section_derived_namespace)
                : current_namespace;

            if (effective_namespace.empty() || effective_name.empty() ||
                current_description.empty())
            {
                throw AtlasCommandLineError(
                    "Incomplete type definition near line " +
                    std::to_string(line_number) + " in " + args.input_file);
            }

            // Parse description and expand profile tokens using unified parsing
            std::string expanded_description;
            try {
                auto parsed = parse_specification(current_description);

                // Expand {PROFILE} tokens in operators by merging profile specs
                for (auto const & op : parsed.operators) {
                    // Check if this is a profile reference: {NAME}
                    if (op.length() > 2 && op[0] == '{' &&
                        op[op.length() - 1] == '}')
                    {
                        // Extract profile name
                        std::string profile_name = op.substr(
                            1,
                            op.length() - 2);

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
                expanded_description = parsed.had_strong_keyword
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
            } catch (std::exception const & e) {
                throw AtlasCommandLineError(
                    "Error parsing/expanding description near line " +
                    std::to_string(line_number) + " in " + args.input_file +
                    ": " + e.what());
            }

            // Merge all constants from multiple constants= lines
            auto constants = merge_constants(
                current_constants,
                "for type '" + effective_name + "' near line " +
                    std::to_string(line_number));

            result.types.push_back(StrongTypeDescription{
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
                .forwarded_memfns = current_forward});

            current_kind.clear();
            current_namespace.clear();
            current_name.clear();
            current_description.clear();
            current_default_value.clear();
            current_constants.clear();
            current_forward.clear();
            section_derived_kind.clear();
            section_derived_namespace.clear();
            section_derived_name.clear();
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

            // Extract content between brackets and trim whitespace
            std::string section_content = trim(line.substr(1, line.size() - 2));

            if (section_content.empty()) {
                throw AtlasCommandLineError(
                    "Empty section header at line " +
                    std::to_string(line_number) + " in " + args.input_file);
            }

            // Legacy syntax: [type]
            if (section_content == "type") {
                // Name and namespace will be specified in the section body
                // Clear any previous section-derived values
                section_derived_kind.clear();
                section_derived_namespace.clear();
                section_derived_name.clear();
            } else {
                // New syntax: [TypeName], [ns::TypeName], [struct TypeName],
                // or [class ns::TypeName]

                // Check for optional kind prefix (struct or class)
                if (section_content == "struct" || section_content == "class") {
                    // Just the keyword with no typename - error
                    throw AtlasCommandLineError(
                        "Missing type name in section header at line " +
                        std::to_string(line_number) + " in " + args.input_file);
                } else if (
                    section_content.size() >= 7 &&
                    section_content.substr(0, 7) == "struct ")
                {
                    section_derived_kind = "struct";
                    section_content = trim(section_content.substr(7));
                } else if (
                    section_content.size() >= 6 &&
                    section_content.substr(0, 6) == "class ")
                {
                    section_derived_kind = "class";
                    section_content = trim(section_content.substr(6));
                } else {
                    section_derived_kind.clear();
                }

                // After removing kind prefix, check if anything remains
                if (section_content.empty()) {
                    throw AtlasCommandLineError(
                        "Missing type name in section header at line " +
                        std::to_string(line_number) + " in " + args.input_file);
                }

                // Find last occurrence of ::
                auto last_colon_pos = section_content.rfind("::");

                if (last_colon_pos != std::string::npos) {
                    // Qualified name: [ns::TypeName]
                    section_derived_namespace = trim(
                        section_content.substr(0, last_colon_pos));
                    section_derived_name = trim(
                        section_content.substr(last_colon_pos + 2));

                    // Validate namespace
                    if (not is_valid_cpp_namespace(section_derived_namespace)) {
                        throw AtlasCommandLineError(
                            "Invalid C++ namespace in section header at line " +
                            std::to_string(line_number) + " in " +
                            args.input_file + ": '" +
                            section_derived_namespace + "'");
                    }

                    // Check for trailing :: (namespace with no name)
                    if (section_derived_name.empty()) {
                        throw AtlasCommandLineError(
                            "Missing type name after namespace in section "
                            "header at line " +
                            std::to_string(line_number) + " in " +
                            args.input_file);
                    }
                } else {
                    // Unqualified name: [TypeName]
                    section_derived_namespace.clear();
                    section_derived_name = section_content;
                }

                // Validate type name
                if (not is_valid_cpp_identifier(section_derived_name)) {
                    throw AtlasCommandLineError(
                        "Invalid C++ identifier in section header at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": '" + section_derived_name + "'");
                }
            }

            continue;
        }

        // Parse key=value
        auto equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            throw AtlasCommandLineError(
                "Invalid format at line " + std::to_string(line_number) +
                " in " + args.input_file +
                ": expected 'key=value' or section header like '[type]' or "
                "'[TypeName]'");
        }

        std::string key = trim(line.substr(0, equals_pos));
        std::string value = trim(line.substr(equals_pos + 1));

        // File-level configuration
        if (not in_type_section) {
            if (key == "guard_prefix") {
                result.guard_prefix = value;
            } else if (key == "guard_separator") {
                result.guard_separator = value;
            } else if (key == "upcase_guard") {
                result.upcase_guard = parse_bool(value, "upcase_guard");
            } else if (key == "namespace") {
                global_namespace = value;
            } else if (key == "cpp_standard") {
                // File-level cpp_standard will be applied to all types later
                // Store it in a variable for now
                try {
                    result.file_level_cpp_standard = parse_cpp_standard(value);
                } catch (std::invalid_argument const & e) {
                    throw AtlasCommandLineError(
                        "Invalid cpp_standard at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": " + e.what());
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
                    throw AtlasCommandLineError(
                        "Error parsing/registering profile at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": " + e.what());
                }
            } else {
                throw AtlasCommandLineError(
                    "Unknown configuration key at line " +
                    std::to_string(line_number) + " in " + args.input_file +
                    ": " + key);
            }
        } else { // Type-level configuration
            if (key == "kind") {
                // Check for conflict with section-derived kind
                if (not section_derived_kind.empty() &&
                    section_derived_kind != value)
                {
                    throw AtlasCommandLineError(
                        "Conflicting kind at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": section header specifies '" + section_derived_kind +
                        "' but kind field specifies '" + value + "'");
                }
                current_kind = value;
            } else if (key == "namespace") {
                // Check for conflict with section-derived namespace
                if (not section_derived_namespace.empty() &&
                    section_derived_namespace != value)
                {
                    throw AtlasCommandLineError(
                        "Conflicting namespace at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": section header specifies '" +
                        section_derived_namespace +
                        "' but namespace field "
                        "specifies '" +
                        value + "'");
                }
                current_namespace = value;
            } else if (key == "name") {
                // Check for conflict with section-derived name
                if (not section_derived_name.empty() &&
                    section_derived_name != value)
                {
                    throw AtlasCommandLineError(
                        "Conflicting name at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": section header specifies '" + section_derived_name +
                        "' but name field specifies '" + value + "'");
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
                current_forward.push_back(
                    value); // Accumulate multiple forward= lines
            } else {
                throw AtlasCommandLineError(
                    "Unknown type property at line " +
                    std::to_string(line_number) + " in " + args.input_file +
                    ": " + key);
            }
        }
    }

    // Finalize last type
    finalize_type();

    if (result.types.empty()) {
        throw AtlasCommandLineError(
            "No type definitions found in input file: " + args.input_file);
    }

    // Override with command-line args if provided
    if (not args.guard_prefix.empty()) {
        result.guard_prefix = args.guard_prefix;
    }

    // Override cpp_standard for all types if CLI flag is specified
    if (args.cpp_standard > 0) {
        result.file_level_cpp_standard = args.cpp_standard;
        for (auto & type : result.types) {
            type.cpp_standard = args.cpp_standard;
        }
    }

    return result;
}

std::string
AtlasCommandLine::
get_help_text()
{
    return R"_(Atlas Strong Type Generator

Generate C++ strong type wrappers with configurable operators and features.

USAGE:
    atlas --kind=<kind> --namespace=<namespace> --name=<name>
          --description=<description> [OPTIONS]

    atlas --input=<file> [--output=<file>] [OPTIONS]

REQUIRED ARGUMENTS (command-line mode):
    --kind=<kind>               Type declaration kind: 'struct' or 'class'
    --namespace=<namespace>     C++ namespace for the generated type
    --name=<name>               Name of the generated strong type
    --description=<description> Type description including operators (e.g.,
                                "strong int; +, -, ==, !=")

FILE MODE:
    --input=<file>              Read type descriptions from input file
                                (one or more type definitions)
    --output=<file>             Write generated code to file instead of stdout
    --interactions=<bool>       Parse input file as interaction definitions
                                instead of type definitions (default: false)
                                Values: true/false, 1/0, yes/no

OPTIONAL ARGUMENTS:
    --default-value=<value>     Default value for default constructor
                                (e.g., 42, "hello", std::vector<int>{1,2,3})
    --constants=<consts>        Named constants for the strong type (similar
                                to scoped enum values). Format:
                                "name:value; name2:value2"
                                Can be specified multiple times to accumulate
                                constants.
    --forward=<memfns>          Forward member functions from underlying type.
                                Format: "memfn1,memfn2,memfn3" or
                                "const,memfn1,memfn2" for const-only, or
                                "memfn:alias" for aliasing.
                                Can be specified multiple times to accumulate
                                forwarded member functions.
    --guard-prefix=<prefix>     Custom prefix for header guards
                                (default: namespace-based)
    --guard-separator=<sep>     Separator for header guard components
                                (default: "_")
    --upcase-guard=<bool>       Use uppercase header guards (default: true)
                                Values: true/false, 1/0, yes/no
    --cpp-standard=<std>        Target C++ standard (11, 14, 17, 20, or 23)
                                Generates static_assert to enforce minimum
                                standard at compile time. Overrides file-level
                                and description-level specifications.
                                (default: 11)

    --help, -h                  Show this help message
    --version, -v               Show version information

EXAMPLES:
    # Generate a simple integer wrapper
    atlas --kind=struct --namespace=math --name=Distance \
          --description="strong int; +, -, ==, !="

    # Generate a class with comprehensive operators
    atlas --kind=class --namespace=util --name=Counter \
          --description="strong int; +, -, *, <=>, ++, --, bool, out"

    # Generate a type with named constants
    atlas --kind=struct --namespace=math --name=Status \
          --description="int; ==, !=" \
          --constants="SUCCESS:0; FAILURE:1" --constants="PENDING:2"

    # Generate from input file
    atlas --input=types.txt --output=types.hpp

    # Custom header guard settings
    atlas --kind=struct --namespace=test --name=MyType \
          --description="strong double" \
          --guard-prefix=MYPROJECT --guard-separator=_$_ --upcase-guard=true

    # Generate with C++20 requirement
    atlas --kind=struct --namespace=test --name=UserId \
          --description="strong int; <=>" \
          --cpp-standard=20

INPUT FILE FORMAT:
    The input file uses a simple key=value format with [type] section markers:

    # File-level configuration (optional)
    guard_prefix=MY_TYPES    # optional prefix for header guard
    guard_separator=_        # optional, default: _
    upcase_guard=true        # optional, default: true
    namespace=math           # optional default namespace for all types
    cpp_standard=20          # optional C++ standard (11, 14, 17, 20, 23)

    # Profile definitions (optional, reusable feature bundles)
    profile=NUMERIC; +, -, *, /
    profile=COMPARABLE; ==, !=, <, <=, >, >=

    # Type definitions (multiple formats supported)
    [type]                   # Legacy format
    kind=struct
    namespace=math
    name=Distance
    description=strong int; +, -, ==, !=
    default_value=0
    constants=zero:0; max:1000

    [struct util::Counter]   # Inline syntax: [kind namespace::name]
    description=int; {COMPARABLE}, ++, --, bool, out
    default_value=100
    constants=initial:100

    [test::UserId]           # C++ standard can be specified in description
    description=strong int; <=>, c++20

    Alternative section headers:
    [TypeName]               # Unqualified name
    [ns::TypeName]           # Qualified name without kind (defaults to struct)
    [struct TypeName]        # Explicit kind with unqualified name
    [class ns::TypeName]     # Fully qualified with kind

    All types are generated in a single file with one unified header guard.
    The guard will be: guard_prefix_separator_SHA1 (defaults to ATLAS_ if no
    prefix).

PROFILES:
    Profiles are reusable feature bundles defined at file level:

    profile=NAME; feature1, feature2, ...

    Use profiles in descriptions with {NAME} syntax:
    description=strong int; {NUMERIC}, hash

    Profiles can be composed and features are automatically deduplicated.
    Profiles must be defined before use in type definitions.

CONSTANTS:
    Named constants generate static members similar to scoped enum values:

    constants=name:value; name2:value2

    Multiple constants= lines can be used per type. Example:

    [type]
    name=Status
    description=int; ==, !=
    constants=SUCCESS:0; FAILURE:1
    constants=PENDING:2

    Generates: static constexpr Status SUCCESS = Status(0);
               static constexpr Status FAILURE = Status(1);
               static constexpr Status PENDING = Status(2);
    (or static const if no-constexpr is specified)

OPERATOR REFERENCE:
    Arithmetic:     +, -, *, /, %, u+, u-, u~, &, |, ^, <<, >>
    Comparison:     ==, !=, <, <=, >, >=, <=>
    Special:        ++, --, bool, (), (&), [], @, &of, ->
    Stream:         in, out
    Iteration:      iterable (enables range-based for loops)
    Formatting:     fmt (enables std::format support in C++20)
    Assignment:     assign (template assignment operator)
    Casts:          cast<Type> or explicit_cast<Type> (explicit cast)
                    implicit_cast<Type> (implicit cast, use sparingly)
    Hash:           hash (enables std::hash specialization)
                    no-constexpr-hash (hash without constexpr)
    Subscript:      [] (supports C++23 multidimensional subscripts)
    Custom:         #<header> or #"header" for custom includes
    Modes:          checked, saturating, or wrapping

CONSTRAINTS (Enforce Invariants):
    Constrained types validate values at construction and after operations:

    positive             Value must be > 0
    non_negative         Value must be >= 0
    non_zero             Value must be != 0
    bounded<Min,Max>     Value must be in [Min, Max] (closed interval)
    bounded_range<Min,Max> Value must be in [Min, Max) (half-open)
    non_empty            Container/string must not be empty (deletes default ctor)
    non_null             Pointer must not be null (deletes default ctor)

    Examples:
        "int; positive, +, -, *"                    # Positive integers
        "int; bounded<0,100>, <=>"                  # Percentage (0-100)
        "double; bounded<-273.15,1e7>, +, -"        # Temperature (absolute zero+)
        "std::string; non_empty, ==, !="            # Non-empty strings
        "void*; non_null, ==, !="                   # Non-null pointers
        "uint8_t; bounded<0,100>, +, -, checked"    # Bounded with overflow check

    Constraints throw atlas::ConstraintError on violations:
    - Constructor: "TypeName: value violates constraint: message"
    - Arithmetic: "TypeName: arithmetic result violates constraint (message)"
    - Forwarded functions: "TypeName::function: operation violates constraint (message)"
    Note: Minor formatting inconsistencies exist; don't rely on exact formats.

    For constexpr values, violations cause compilation errors.
    Constraints work with all arithmetic modes (checked/saturating/wrapping).

CONSTEXPR BEHAVIOR:
    By default, all operations are marked constexpr for use in constant
    expressions.

    no-constexpr         Removes constexpr from all operations
    no-constexpr-hash    Removes constexpr only from hash

    Examples:
        "strong int; +, -, hash"              # All constexpr
        "strong std::string; ==, no-constexpr-hash" # Ops constexpr, hash isn't
        "strong std::string; ==, hash, no-constexpr" # Nothing constexpr

For more information, see the Atlas documentation.
)_";
}

InteractionFileDescription
AtlasCommandLine::
parse_interaction_file(std::string const & filename)
{
    std::ifstream file(filename);
    if (not file) {
        throw AtlasCommandLineError(
            "Cannot open interaction file: " + filename);
    }

    InteractionFileDescription result;
    std::string line;
    int line_number = 0;

    // Helper to check if line starts with a prefix
    auto starts_with = [](std::string const & str,
                          std::string const & prefix) -> bool {
        return str.size() >= prefix.size() &&
            str.substr(0, prefix.size()) == prefix;
    };

    // Helper to extract value after '='
    auto extract_after_equals = [](std::string const & str) -> std::string {
        auto pos = str.find('=');
        if (pos == std::string::npos) {
            return "";
        }
        return trim(str.substr(pos + 1));
    };

    // State tracking
    std::string current_namespace;
    std::string current_value_access = "atlas::value";
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
            std::string include = trim(line.substr(8));
            result.includes.push_back(include);
        } else if (line == "include") {
            throw AtlasCommandLineError(
                "Malformed include directive at line " +
                std::to_string(line_number) + " in " + filename +
                ". Expected: include <header> or include \"header\"");
        } else if (starts_with(line, "concept=")) {
            std::string value = extract_after_equals(line);
            if (value.empty()) {
                throw AtlasCommandLineError(
                    "Empty concept definition at line " +
                    std::to_string(line_number) + " in " + filename +
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
                throw AtlasCommandLineError(
                    "Empty template parameter name at line " +
                    std::to_string(line_number) + " in " + filename);
            }

            if (not result.constraints.contains(name)) {
                result.constraints[name] = TypeConstraint{.name = name};
            }
            result.constraints[name].concept_expr = concept_expr;
            pending_concept_name = name;
        } else if (starts_with(line, "enable_if=")) {
            std::string expr = extract_after_equals(line);
            if (expr.empty()) {
                throw AtlasCommandLineError(
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
                std::string param_name = extract_template_param_from_enable_if(
                    expr,
                    line_number,
                    filename);

                if (not result.constraints.contains(param_name)) {
                    result.constraints[param_name] = TypeConstraint{
                        .name = param_name};
                }
                result.constraints[param_name].enable_if_expr = expr;
            }
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
            result.upcase_guard = parse_bool(
                extract_after_equals(line),
                "upcase_guard");
        } else if (starts_with(line, "cpp_standard=")) {
            std::string standard_str = extract_after_equals(line);
            try {
                result.cpp_standard = parse_cpp_standard(standard_str);
            } catch (std::invalid_argument const & e) {
                throw AtlasCommandLineError(
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
            bool symmetric = line.find("<->") != std::string::npos;
            std::string arrow = symmetric ? "<->" : "->";

            auto arrow_pos = line.find(arrow);
            std::string left_side = trim(line.substr(0, arrow_pos));
            std::string result_type = trim(
                line.substr(arrow_pos + arrow.size()));

            // Parse left side: LHS OP RHS
            // Find operator - look for common operators
            std::vector<std::string> ops = {
                "<<",
                ">>",
                "==",
                "!=",
                "<=",
                ">=",
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
                throw AtlasCommandLineError(
                    "Cannot parse interaction at line " +
                    std::to_string(line_number) + " in " + filename + ": " +
                    line);
            }

            if (result_type.empty()) {
                throw AtlasCommandLineError(
                    "Missing result type for interaction at line " +
                    std::to_string(line_number) + " in " + filename + ": " +
                    line);
            }

            // Check if types are constraints (templates)
            bool lhs_is_template = result.constraints.contains(lhs_type);
            bool rhs_is_template = result.constraints.contains(rhs_type);

            InteractionDescription interaction{
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

            result.interactions.push_back(interaction);

            // Clear pending concept name after interaction is parsed
            pending_concept_name.clear();
        } else {
            throw AtlasCommandLineError(
                "Unknown directive at line " + std::to_string(line_number) +
                " in " + filename + ": " + line);
        }
    }

    // Warn if no interactions were defined
    if (result.interactions.empty()) {
        throw AtlasCommandLineError(
            "No interactions found in file: " + filename +
            ". Interaction files must contain at least one interaction "
            "(e.g., 'Type1 * Type2 -> Result').");
    }

    return result;
}

}} // namespace wjh::atlas::v1
