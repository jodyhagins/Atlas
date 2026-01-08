// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_D12E103EDCC54717A84227722361BE22
#define WJH_ATLAS_D12E103EDCC54717A84227722361BE22

#include "InteractionGenerator.hpp"
#include "StrongTypeGenerator.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace wjh::atlas {

class AtlasCommandLineError
: public std::runtime_error
{
public:
    explicit AtlasCommandLineError(std::string const & message)
    : std::runtime_error(message)
    { }
};

class AtlasCommandLine
{
public:
    struct Arguments
    {
        std::string kind;
        std::string type_namespace;
        std::string type_name;
        std::string description;
        std::string default_value;
        std::vector<std::string> constants;
        std::vector<std::string> forwarded_memfns;
        std::string guard_prefix;
        std::string guard_separator = "_";
        bool upcase_guard = true;
        bool help = false;
        bool version = false;
        std::string input_file;
        std::string output_file;
        bool interactions_mode = false;
        int cpp_standard = 0; // 0 means not specified on CLI

        // Auto-generation options (for single-type mode)
        bool auto_hash = false;
        bool auto_ostream = false;
        bool auto_istream = false;
        bool auto_format = false;
    };

    struct FileGenerationResult
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

    // Parse command line arguments
    static Arguments parse(int argc, char const * const * argv);

    // Parse from vector of strings (useful for testing)
    static Arguments parse(std::vector<std::string> const & args);

    // Convert Arguments to StrongTypeDescription
    static StrongTypeDescription to_description(Arguments const & args);

    // Parse input file and return type descriptions
    static FileGenerationResult parse_input_file(Arguments const & args);

    // Parse interaction file and return interaction descriptions
    static InteractionFileDescription parse_interaction_file(
        std::string const & filename);

    // Get help text
    static std::string get_help_text();

private:
    static Arguments parse_impl(std::vector<std::string> const & args);
    static void validate_arguments(Arguments const & args);
};

} // namespace wjh::atlas

#endif // WJH_ATLAS_D12E103EDCC54717A84227722361BE22
