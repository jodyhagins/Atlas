// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_B8F3D4E7A1C54E2B9D6F8A2C3E5F7B9D
#define WJH_ATLAS_B8F3D4E7A1C54E2B9D6F8A2C3E5F7B9D

#include "AtlasCommandLine.hpp"
#include "InteractionGenerator.hpp"
#include "StrongTypeGenerator.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace wjh::atlas {

/**
 * Base exception for all parser-related errors.
 */
class AtlasParserError
: public std::runtime_error
{
public:
    explicit AtlasParserError(std::string const & message)
    : std::runtime_error(message)
    { }
};

/**
 * Result structure for parsing operations.
 *
 * Contains a vector of strong type descriptions along with metadata
 * needed for generation (header guards, C++ standard, etc.).
 */
struct ParseResult
{
    std::vector<StrongTypeDescription> types;
    std::string guard_prefix;
    std::string guard_separator = "_";
    bool upcase_guard = true;
    int cpp_standard = 11;
};

/**
 * Result structure for parsing type definition files.
 *
 * Contains type descriptions and file-level configuration.
 */
struct FileParseResult
{
    std::string guard_prefix;
    std::string guard_separator = "_";
    bool upcase_guard = true;
    int file_level_cpp_standard = 11;
    std::vector<StrongTypeDescription> types;

    // Auto-generation options
    bool auto_hash = false;
    bool auto_ostream = false;
    bool auto_istream = false;
    bool auto_format = false;
};

/**
 * Common parsing utilities used by all parser components.
 */
namespace parser_utils {

/**
 * Parse a boolean value from string.
 *
 * Accepts: "true", "yes", "1" (case-insensitive) -> true
 *          "false", "no", "0" (case-insensitive) -> false
 *
 * @param value String to parse (passed by value, will be modified)
 * @param option_name Name of the option (for error messages)
 * @return Boolean value
 * @throw AtlasParserError if value is not a valid boolean string
 */
bool parse_bool(std::string value, std::string const & option_name);

/**
 * Parse constants from a string in format "name:value; name:value".
 *
 * @param constants_string String containing constants definitions
 * @param context Context string for error messages
 * @return Map of constant names to values
 * @throw AtlasParserError on invalid format or duplicate names
 */
std::map<std::string, std::string> parse_constants_string(
    std::string const & constants_string,
    std::string const & context);

/**
 * Merge multiple constant definitions into a single map.
 *
 * @param constants_strings Vector of constant strings to merge
 * @param context Context string for error messages
 * @return Unified map of all constants
 * @throw AtlasParserError on duplicate names
 */
std::map<std::string, std::string> merge_constants(
    std::vector<std::string> const & constants_strings,
    std::string const & context);

/**
 * Normalize a type description by sorting operators and removing duplicates.
 *
 * Handles format: type; [forward=...;] operators
 *
 * @param description Raw description string
 * @return Normalized description with sorted operators
 */
std::string normalize_description(std::string const & description);

} // namespace parser_utils

/**
 * File parser for Atlas type definition and interaction files.
 *
 * Extracts file parsing logic from AtlasCommandLine into reusable
 * static member functions.
 */
class AtlasFileParser
{
public:
    /**
     * Parse type definition file.
     *
     * @param filename Input file path
     * @param guard_prefix Header guard prefix (can be overridden by file)
     * @param guard_separator Header guard separator
     * @param upcase_guard Whether to uppercase header guard
     * @param cli_cpp_standard C++ standard from CLI (overrides file)
     * @return Parsed file result with type descriptions
     * @throw AtlasParserError on parsing errors
     */
    static FileParseResult parse_type_definitions(
        std::string const & filename,
        std::string const & guard_prefix,
        std::string const & guard_separator,
        bool upcase_guard,
        int cli_cpp_standard);

    /**
     * Parse interaction definition file.
     *
     * @param filename Input file path
     * @return Interaction file description
     * @throw AtlasParserError on parsing errors
     */
    static InteractionFileDescription parse_interactions(
        std::string const & filename);
};

/**
 * CLI parser for Atlas command-line arguments.
 *
 * Extracts CLI parsing logic from AtlasCommandLine into reusable
 * static member functions.
 */
class AtlasCliParser
{
public:
    /**
     * Parse command-line arguments.
     *
     * @param args Vector of argument strings
     * @return Arguments structure with parsed values
     * @throw AtlasParserError on invalid arguments
     */
    static AtlasCommandLine::Arguments parse_arguments(
        std::vector<std::string> const & args);

    /**
     * Convert Arguments to StrongTypeDescription.
     *
     * @param args Parsed arguments
     * @return Strong type description ready for generation
     * @throw AtlasParserError on conversion errors
     */
    static StrongTypeDescription arguments_to_description(
        AtlasCommandLine::Arguments const & args);
};

} // namespace wjh::atlas

#endif // WJH_ATLAS_B8F3D4E7A1C54E2B9D6F8A2C3E5F7B9D
