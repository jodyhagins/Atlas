# CMake Formatting Guide

## Overview

Instead of using automated tools like `cmake-format` (which have limitations with certain constructs like generator expressions), we use **Claude Code as the CMake formatter** for the Atlas project.

## Why Claude Code as Formatter?

**Benefits:**
- ✅ Understands context and applies nuanced formatting rules
- ✅ Deterministic when given explicit rules
- ✅ Handles complex cases intelligently
- ✅ Correct indentation (+4 from command, keywords stand out with +4 nested args)

## Formatting Rules

All formatting rules are documented in `cmake/.cmake-format-rules.md`. These rules were derived from samples from as existing project.

### Key Principles

1. **Keywords on their own line** - PUBLIC, PRIVATE, INTERFACE get separate lines with +4 indent from command
2. **Arguments under keywords** - Nested +4 from keyword (= +8 from command) for standout effect
3. **Simple statements stay compact** - Don't over-split simple IF conditions
4. **Vertical layout for readability** - Complex commands break across multiple lines

### Quick Examples

**Good (keyword stands out):**
```cmake
target_link_libraries(atlas_lib
    PRIVATE
        Boost::describe
        Boost::json
        Boost::uuid)
```

**Good (generator expressions):**
```cmake
target_include_directories(atlas_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/atlas>)
```

**Bad (keywords don't stand out):**
```cmake
target_link_libraries(atlas_lib
    PRIVATE Boost::describe
            Boost::json)
```

## How to Format CMake Files

### When Needed

Format CMake files when:
- Adding new CMakeLists.txt files
- Making significant changes to existing files
- Before major releases

### Using Claude Code to Format

1. **Ask Claude to format:**
   ```
   Format this CMakeLists.txt file according to the rules in cmake/.cmake-format-rules.md
   ```

2. **Claude will:**
   - Read `cmake/.cmake-format-rules.md` for the style guide
   - Apply rules deterministically (no random variations)
   - Preserve functionality while improving readability

3. **Verify:**
   ```bash
   cd /Users/jhagins/build/atlas/debug
   cmake /Users/jhagins/src/atlas
   ```

## Deterministic Formatting

To ensure consistency, Claude Code follows these principles:
- No creative interpretation of rules
- No random variations between formatting sessions
- Mechanical application of documented rules
- If a pattern matches a rule, apply that rule exactly
- When uncertain, prefer vertical layout with keywords on separate lines

## Future Maintenance

When CMake files need reformatting:
1. Review `cmake/.cmake-format-rules.md` to ensure rules are still current
2. Ask Claude Code to format the file(s)
3. Verify CMake still parses correctly
4. Update this document if new patterns emerge
