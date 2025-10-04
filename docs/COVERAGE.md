# Code Coverage Guide

## Quick Start (Recommended)

### Option 1: Using CMake targets (easiest)

```bash
# Configure with coverage enabled
cmake -B build/coverage -G Ninja -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug

# Build and run coverage
cmake --build build/coverage
cmake --build build/coverage --target coverage

# View report (if lcov is installed)
open build/coverage/coverage_html/index.html  # macOS
# or
xdg-open build/coverage/coverage_html/index.html  # Linux
```

### Option 2: Using the convenience script

```bash
scripts/run_coverage.sh
```

This script will automatically:
- Configure a coverage build
- Compile the project
- Run all tests
- Generate an HTML coverage report
- Open it in your browser (if available)

## Installing lcov (for HTML reports)

**macOS:**
```bash
brew install lcov
```

**Ubuntu/Debian:**
```bash
sudo apt-get install lcov
```

**Fedora/RHEL:**
```bash
sudo dnf install lcov
```

## Manual Coverage Analysis

If you already have a build with coverage flags:

```bash
cd build/atlas/debug
ctest --output-on-failure

# Generate coverage data
find . -name "*.gcda" | while read f; do gcov "$f"; done

# View .gcov files for line-by-line coverage
less src/lib/StrongTypeGenerator.cpp.gcov
```

## Understanding the Report

- **Green lines**: Covered by tests
- **Red lines**: Not covered by tests
- **Gray lines**: Non-executable (comments, declarations)

Coverage percentages:
- **Line coverage**: % of executable lines run by tests
- **Function coverage**: % of functions called by tests
- **Branch coverage**: % of conditional branches taken

## CI/CD Integration

The coverage target can be integrated into CI pipelines:

```bash
cmake -B build -DENABLE_COVERAGE=ON
cmake --build build
cmake --build build --target coverage
```

## Interpreting Results

Focus on:
1. **Core logic** (StrongTypeGenerator, InteractionGenerator) - aim for >90%
2. **Edge cases** - error handling paths
3. **Public API** - all user-facing functions

Low coverage areas to investigate:
- Error handling paths not triggered by tests
- Defensive code that shouldn't normally execute
- Platform-specific code paths
