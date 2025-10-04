#!/bin/bash
# Wrapper script to use llvm-cov in gcov-compatible mode
# This provides better branch coverage data than Apple's gcov emulation
exec xcrun llvm-cov gcov "$@"
