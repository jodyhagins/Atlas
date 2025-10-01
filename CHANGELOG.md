# Changelog

All notable changes to this project will be documented here. This format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) and the project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- **Cross-Type Interaction Generation**: New `InteractionGenerator` library for generating operator interactions between different types.
  - Support for asymmetric operators (`->`) and symmetric/commutative operators (`<->`)
  - Template-based interactions with C++20 concepts and C++17 SFINAE
  - Customizable value extraction via `atlas::value` implementation
  - Self-contained generated headers with embedded `atlas::value` utility
  - `--interactions=true` CLI flag for interactions mode
  - Comprehensive interaction file format with directives for includes, namespaces, concepts, and constexpr control
  - 19 comprehensive tests including property-based tests with RapidCheck
- `AtlasUtilities` library with shared SHA1 and header guard utilities (refactored from `StrongTypeGenerator`)
- Robust error handling for interaction files with descriptive error messages including line numbers
- Initial CLI (`atlas`) for generating strong type headers.
- `StrongTypeGenerator` library with extensible operator support.
- Command-line parsing and validation via `AtlasCommandLine`.
- Documentation covering usage, DSL reference, and header guard design.
- `no-constexpr` option to disable constexpr for all operations.
- `no-constexpr-hash` option to selectively disable constexpr for hash only.
- FetchContent example demonstrating CMake integration.
- `test_install.sh` script for local validation of installation and find_package.
- Comprehensive CI with GCC/Clang on Ubuntu and macOS.
- Code coverage reporting via codecov.

### Changed
- N/A

### Fixed
- N/A
