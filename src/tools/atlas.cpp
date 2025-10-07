// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasCommandLine.hpp"
#include "AtlasMain.hpp"

#include <cstdlib>

#include <iostream>

/**
 * Main entry point for the atlas command-line tool.
 *
 * This is a thin wrapper around atlas_main() that handles exceptions
 * and converts them to appropriate error messages and exit codes.
 */
int
main(int argc, char ** argv)
{
    using wjh::atlas::AtlasCommandLineError;

    try {
        return wjh::atlas::atlas_main(argc, argv);
    } catch (AtlasCommandLineError const & ex) {
        std::cerr << "Error: " << ex.what()
            << "\n\nUse --help or -h for usage information." << std::endl;
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
    }
    return EXIT_FAILURE;
}
