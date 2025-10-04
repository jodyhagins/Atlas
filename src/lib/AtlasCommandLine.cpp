// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasCommandLine.hpp"
#include "AtlasUtilities.hpp"

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

    return StrongTypeDescription{
        .kind = args.kind,
        .type_namespace = args.type_namespace,
        .type_name = args.type_name,
        .description = args.description,
        .default_value = args.default_value,
        .guard_prefix = args.guard_prefix,
        .guard_separator = args.guard_separator,
        .upcase_guard = args.upcase_guard};
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

    std::string line;
    int line_number = 0;
    bool in_type_section = false;

    // Current type being parsed
    std::string current_kind;
    std::string current_namespace;
    std::string current_name;
    std::string current_description;
    std::string current_default_value;

    auto finalize_type = [&]() {
        if (not current_kind.empty()) {
            if (current_namespace.empty() || current_name.empty() ||
                current_description.empty())
            {
                throw AtlasCommandLineError(
                    "Incomplete type definition near line " +
                    std::to_string(line_number) + " in " + args.input_file);
            }

            result.types.push_back(StrongTypeDescription{
                .kind = current_kind,
                .type_namespace = current_namespace,
                .type_name = current_name,
                .description = current_description,
                .default_value = current_default_value,
                .guard_prefix = result.guard_prefix,
                .guard_separator = result.guard_separator,
                .upcase_guard = result.upcase_guard});

            current_kind.clear();
            current_namespace.clear();
            current_name.clear();
            current_description.clear();
            current_default_value.clear();
        }
    };

    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check for [type] section
        if (line == "[type]") {
            finalize_type();
            in_type_section = true;
            continue;
        }

        // Parse key=value
        auto equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            throw AtlasCommandLineError(
                "Invalid format at line " + std::to_string(line_number) +
                " in " + args.input_file +
                ": expected 'key=value' or '[type]'");
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
            } else {
                throw AtlasCommandLineError(
                    "Unknown configuration key at line " +
                    std::to_string(line_number) + " in " + args.input_file +
                    ": " + key);
            }
        } else { // Type-level configuration
            if (key == "kind") {
                current_kind = value;
            } else if (key == "namespace") {
                current_namespace = value;
            } else if (key == "name") {
                current_name = value;
            } else if (key == "description") {
                current_description = value;
            } else if (key == "default_value") {
                current_default_value = value;
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

    return result;
}

std::string
AtlasCommandLine::
get_help_text()
{
    return R"(Atlas Strong Type Generator

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
    --guard-prefix=<prefix>     Custom prefix for header guards
                                (default: namespace-based)
    --guard-separator=<sep>     Separator for header guard components
                                (default: "_")
    --upcase-guard=<bool>       Use uppercase header guards (default: true)
                                Values: true/false, 1/0, yes/no

    --help, -h                  Show this help message

EXAMPLES:
    # Generate a simple integer wrapper
    atlas --kind=struct --namespace=math --name=Distance \
          --description="strong int; +, -, ==, !="

    # Generate a class with comprehensive operators
    atlas --kind=class --namespace=util --name=Counter \
          --description="strong int; +, -, *, <=>, ++, --, bool, out"

    # Generate from input file
    atlas --input=types.txt --output=types.hpp

    # Custom header guard settings
    atlas --kind=struct --namespace=test --name=MyType \
          --description="strong double" \
          --guard-prefix=MYPROJECT --guard-separator=_$_ --upcase-guard=true

INPUT FILE FORMAT:
    The input file uses a simple key=value format with [type] section markers:

    # File-level configuration (optional)
    guard_prefix=MY_TYPES    # optional prefix for header guard
    guard_separator=_        # optional, default: _
    upcase_guard=true        # optional, default: true

    # Type definitions
    [type]
    kind=struct
    namespace=math
    name=Distance
    description=strong int; +, -, ==, !=
    default_value=0

    [type]
    kind=class
    namespace=util
    name=Counter
    description=strong int; ++, --, bool, out
    default_value=100

    All types are generated in a single file with one unified header guard.
    The guard will be: guard_prefix_separator_SHA1 (defaults to ATLAS_ if no prefix).

OPERATOR REFERENCE:
    Arithmetic:     +, -, *, /, %, u+, u-, u~, &, |, ^, <<, >>
    Comparison:     ==, !=, <, <=, >, >=, <=>
    Special:        ++, --, bool, (), (&), [], @, &of, ->
    Stream:         in, out
    Hash:           hash (enables std::hash specialization)
                    no-constexpr-hash (hash without constexpr)
    Subscript:      [] (supports C++23 multidimensional subscripts)
    Custom:         #<header> or #"header" for custom includes

CONSTEXPR BEHAVIOR:
    By default, all operations are marked constexpr for use in constant expressions.

    no-constexpr         Removes constexpr from all operations
    no-constexpr-hash    Removes constexpr only from hash

    Examples:
        "strong int; +, -, hash"              # All constexpr
        "strong std::string; ==, no-constexpr-hash" # Ops are constexpr, hash isn't
        "strong std::string; ==, hash, no-constexpr" # Nothing constexpr

For more information, see the Atlas documentation.
)";
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
                    ". Expected: concept=TypeName or concept=TypeName : "
                    "expression");
            }
            // Parse: name : concept_expr
            auto colon_pos = value.find(':');
            std::string name = trim(
                colon_pos == std::string::npos ? value
                                               : value.substr(0, colon_pos));
            std::string concept_expr = colon_pos == std::string::npos
                ? name
                : trim(value.substr(colon_pos + 1));

            if (name.empty()) {
                throw AtlasCommandLineError(
                    "Empty concept name at line " +
                    std::to_string(line_number) + " in " + filename);
            }

            if (not result.constraints.contains(name)) {
                result.constraints[name] = TypeConstraint{.name = name};
            }
            result.constraints[name].concept_expr = concept_expr;
            pending_concept_name = name;
        } else if (starts_with(line, "enable_if=")) {
            std::string expr = extract_after_equals(line);
            if (not pending_concept_name.empty()) {
                result.constraints[pending_concept_name].enable_if_expr = expr;
                pending_concept_name.clear();
            }
            // If no pending concept, this enable_if applies to the last concept
            else if (not result.constraints.empty())
            {
                result.constraints.rbegin()->second.enable_if_expr = expr;
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
