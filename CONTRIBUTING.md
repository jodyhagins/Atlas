# Contributing to Atlas

Thank you for your interest in improving Atlas! This guide explains how to report issues, propose changes, and submit pull requests.

## Code of Conduct

Participation in this project is governed by the Code of Conduct found in `CODE_OF_CONDUCT.md`. Please read it to understand expected behavior.

## Getting Started

1. Fork the repository and create your branch from `main`.
2. Ensure you have a C++20 toolchain.
3. Build the CLI with cmake (see `README.md`).
4. Run tests before submitting changes:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cd build && ctest --output-on-failure
   ```

## Issues and Feature Requests

- Use GitHub issues to report bugs or request features.
- When filing a bug, include steps to reproduce, expected behavior, actual behavior, and environment details (OS, compiler, Boost version).

## Development Guidelines

- Keep changes focused; avoid bundling unrelated updates.
- Update documentation (`README.md`, `docs/`) when behavior or options change.
- Add entries to `CHANGELOG.md` under the “Unreleased” section.
- Follow existing code style; use consistent formatting and meaningful variable names.
- Prefer standard library facilities over bespoke utilities when possible.

## Commit and PR Workflow

1. Rebase your branch on the latest `main` before submitting a PR.
2. Write clear commit messages; use the imperative mood (e.g., “Add option for ...”).
3. Include a summary, motivation, and testing details in your pull request description.
4. Expect review; respond to feedback promptly and courteously.

We appreciate your contributions!

