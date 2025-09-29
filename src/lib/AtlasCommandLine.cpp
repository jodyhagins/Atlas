// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasCommandLine.hpp"

#include <boost/uuid/detail/sha1.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace wjh::atlas { inline namespace v1 {

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
            if (value == "true" || value == "1" || value == "yes") {
                result.upcase_guard = true;
            } else if (value == "false" || value == "0" || value == "no") {
                result.upcase_guard = false;
            } else {
                throw AtlasCommandLineError(
                    "Invalid value for --upcase-guard: '" + value +
                    "'. Expected true/false, 1/0, or yes/no.");
            }
        } else if (key == "input") {
            result.input_file = value;
        } else if (key == "output") {
            result.output_file = value;
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

    // Helper function to trim whitespace
    auto trim = [](std::string const & str) -> std::string {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    };

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
                if (value == "true" || value == "1" || value == "yes") {
                    result.upcase_guard = true;
                } else if (value == "false" || value == "0" || value == "no") {
                    result.upcase_guard = false;
                } else {
                    throw AtlasCommandLineError(
                        "Invalid upcase_guard value at line " +
                        std::to_string(line_number) + " in " + args.input_file +
                        ": expected true/false");
                }
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
make_file_guard(
    std::string const & prefix,
    std::string const & separator,
    bool upcase,
    std::string const & content)
{
    // Compute SHA1 of content
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(content.data(), content.size());
    boost::uuids::detail::sha1::digest_type hash;
    sha1.get_digest(hash);

    std::string sha_str;
    for (unsigned int x : hash) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%08x", x);
        sha_str += buffer;
    }

    // Build guard: prefix + separator + SHA
    // Use "ATLAS" as default prefix if none provided to ensure valid C++
    // identifier
    std::string guard = prefix.empty() ? "ATLAS" : prefix;
    guard += separator;
    guard += sha_str;

    // Uppercase if requested
    if (upcase) {
        std::transform(
            guard.begin(),
            guard.end(),
            guard.begin(),
            [](unsigned char c) { return std::toupper(c); });
    }

    return guard;
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
    Hash:           hash (enables std::hash specialization for unordered containers)
    Subscript:      [] (supports C++23 multidimensional subscripts)
    Custom:         #<header> or #"header" for custom includes

For more information, see the Atlas documentation.
)";
}

}} // namespace wjh::atlas::v1
