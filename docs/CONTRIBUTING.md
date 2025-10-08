# Contributing to Atlas

Thanks for considering contributing! This guide will help you navigate the process without too much suffering.

## Code of Conduct

First things first: don't be a jerk. See [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md) for the extended version.

## Getting Started

1. Fork the repo and branch from `main` (you know the drill)
2. Make sure you have a C++20 compiler (if you're still on C++98, we need to talk)
3. Build it:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cd build && ctest --output-on-failure
   ```
4. If tests pass, you're golden. If not, well... that's what debugging is for.

## Reporting Bugs

Found a bug? Welcome to the club! Open a GitHub issue with:
- Steps to reproduce (bonus points for a minimal example)
- What you expected to happen
- What actually happened (screenshots of your confused face optional)
- Environment details (OS, compiler, version, phase of the moon, etc.)

## Development Guidelines

**Do:**
- Keep changes focused (one thing at a time, like a normal person)
- Update docs when you change behavior (future-you will thank you)
- Follow the existing code style (consistency > personal preference)
- Use the standard library when possible (we're not reinventing `std::vector`)

**Don't:**
- Bundle 47 unrelated changes into one PR
- Break the build (CI will publicly shame you)
- Skip tests (they exist for a reason, probably)

## Submitting PRs

1. Rebase on latest `main` (merge commits make the history cry)
2. Write clear commit messages in imperative mood: "Add feature" not "Added feature" or "Adds feature"
3. Include a summary in your PR: what, why, and how you tested it
4. Respond to review feedback promptly and graciously (we're all learning here)

Remember: code review isn't personal. If someone suggests changes, they're trying to help, not attack your life choices.

Thanks for contributing! May your builds be green and your segfaults be few.

