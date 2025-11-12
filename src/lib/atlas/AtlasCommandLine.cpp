// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasCommandLine.hpp"
#include "AtlasParser.hpp"

#include <algorithm>
#include <sstream>

namespace wjh::atlas {

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
    try {
        auto result = AtlasCliParser::parse_arguments(args);
        if (not result.help && not result.version) {
            validate_arguments(result);
        }
        return result;
    } catch (AtlasParserError const & e) {
        throw AtlasCommandLineError(e.what());
    }
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
    try {
        return AtlasCliParser::arguments_to_description(args);
    } catch (AtlasParserError const & e) {
        throw AtlasCommandLineError(e.what());
    }
}

AtlasCommandLine::FileGenerationResult
AtlasCommandLine::
parse_input_file(Arguments const & args)
{
    try {
        // Delegate to AtlasFileParser
        auto parse_result = AtlasFileParser::parse_type_definitions(
            args.input_file,
            args.guard_prefix,
            args.guard_separator,
            args.upcase_guard,
            args.cpp_standard);

        // Convert FileParseResult to FileGenerationResult
        FileGenerationResult result;
        result.guard_prefix = parse_result.guard_prefix;
        result.guard_separator = parse_result.guard_separator;
        result.upcase_guard = parse_result.upcase_guard;
        result.file_level_cpp_standard = parse_result.file_level_cpp_standard;
        result.types = std::move(parse_result.types);

        return result;
    } catch (AtlasParserError const & e) {
        throw AtlasCommandLineError(e.what());
    }
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
    try {
        // Delegate to AtlasFileParser
        return AtlasFileParser::parse_interactions(filename);
    } catch (AtlasParserError const & e) {
        throw AtlasCommandLineError(e.what());
    }
}

} // namespace wjh::atlas
