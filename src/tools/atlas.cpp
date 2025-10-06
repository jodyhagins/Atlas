// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasCommandLine.hpp"
#include "AtlasUtilities.hpp"
#include "InteractionGenerator.hpp"
#include "StrongTypeGenerator.hpp"
#include "version.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include <iostream>

#ifdef _WIN32
    #include <io.h>
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

int
main(int argc, char ** argv)
{
    namespace atlas = wjh::atlas;
    try {
        auto args = atlas::AtlasCommandLine::parse(argc, argv);
        if (args.help) {
            std::cout << atlas::AtlasCommandLine::get_help_text() << std::endl;
            return EXIT_SUCCESS;
        }

        if (args.version) {
            std::cout << "Atlas Strong Type Generator v"
                << atlas::codegen::version_string << std::endl;
            return EXIT_SUCCESS;
        }

        std::string output;

        // File input mode - generate either types or interactions
        if (not args.input_file.empty()) {
            if (args.interactions_mode) {
                // Parse as interaction file and generate interactions
                auto interaction_desc =
                    atlas::AtlasCommandLine::parse_interaction_file(
                        args.input_file);
                output = atlas::generate_interactions(interaction_desc);
            } else {
                // Parse as type file and generate strong types
                auto file_result = atlas::AtlasCommandLine::parse_input_file(
                    args);
                output = atlas::generate_strong_types_file(
                    file_result.types,
                    file_result.guard_prefix,
                    file_result.guard_separator,
                    file_result.upcase_guard);
            }
        } else { // Command-line mode - single type with individual guard
            auto description = atlas::AtlasCommandLine::to_description(args);
            atlas::StrongTypeGenerator generator;
            output = generator(description);

            // Output warnings to stderr
            auto const & warnings = generator.get_warnings();
            if (not warnings.empty()) {
                bool use_color = atlas::supports_color(fileno(stderr));

                std::cerr << "\n";
                if (use_color) {
                    std::cerr << atlas::color::red
                        << "Warnings:" << atlas::color::reset << "\n";
                    for (auto const & w : warnings) {
                        std::cerr << "  " << atlas::color::yellow << w.type_name
                            << ": " << w.message << atlas::color::reset << "\n";
                    }
                } else {
                    std::cerr << "Warnings:\n";
                    for (auto const & w : warnings) {
                        std::cerr << "  " << w.type_name << ": " << w.message
                            << "\n";
                    }
                }
                std::cerr << std::endl;
            }
        }

        // Write output
        if (not args.output_file.empty()) {
            std::ofstream outfile(args.output_file);
            if (not outfile) {
                throw std::runtime_error(
                    "Cannot open output file: " + args.output_file);
            }
            outfile << output;
            if (not outfile) {
                throw std::runtime_error(
                    "Error writing to output file: " + args.output_file);
            }
        } else {
            std::cout << output << std::endl;
        }

        return EXIT_SUCCESS;
    } catch (atlas::AtlasCommandLineError const & ex) {
        std::cerr << "Error: " << ex.what()
            << "\n\nUse --help or -h for usage information." << std::endl;
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
    }
    return EXIT_FAILURE;
}
