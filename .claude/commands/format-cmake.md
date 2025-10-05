---
description: Format CMake files according to project rules
---

Format the CMake file(s) at `{{arg1}}` according to the rules in `.cmake-format-rules.md`.

**Instructions:**

1. If `{{arg1}}` is a file path ending in `.txt` or `CMakeLists.txt`:
   - Read the file
   - Apply formatting rules from `.cmake-format-rules.md` deterministically
   - Write the formatted content back

2. If `{{arg1}}` is a directory path:
   - Find all `CMakeLists.txt` files in the directory and subdirectories
   - Format each file according to the rules
   - Report which files were formatted

3. Key formatting rules to apply:
   - Multi-line command arguments: indent +4 from command
   - Keywords (PUBLIC, PRIVATE, etc.): indent +4 from command on their own line
   - Arguments under keywords: indent +4 from keyword (= +8 from command)
   - Simple IF statements stay on one line when they fit
   - No random variations - apply rules mechanically and consistently

4. After formatting:
   - Verify CMake can still parse the file(s) by running: `cmake -P <file>` or similar check
   - Report any syntax errors

Apply all rules from `.cmake-format-rules.md` exactly as documented, with no creative interpretation.
