// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_DF79E5E22A2B41289AC1FDAB24AB3378
#define WJH_ATLAS_DF79E5E22A2B41289AC1FDAB24AB3378

namespace wjh::atlas {

/**
 * Main logic for the atlas tool.
 *
 * This function contains all the core logic for the atlas code generator.
 * It can be called from the command-line executable or directly from tests.
 *
 * This function is testable and does not perform any process termination
 * (exit, abort, etc.). Instead, it returns an exit code and throws
 * exceptions for error conditions.
 *
 * @param argc Argument count (standard main() convention)
 * @param argv Argument vector (standard main() convention)
 * @return EXIT_SUCCESS (0) on success, EXIT_FAILURE (1) on error
 *
 * @throws AtlasCommandLineError for command-line argument errors
 * @throws std::exception for other errors (file I/O, parsing, generation)
 *
 * @note The function writes to stdout/stderr for normal operation.
 *       Tests may want to redirect these streams.
 *
 * @see AtlasCommandLine for argument parsing details
 */
[[nodiscard]]
int atlas_main(int argc, char ** argv);

} // namespace wjh::atlas

#endif // WJH_ATLAS_DF79E5E22A2B41289AC1FDAB24AB3378
