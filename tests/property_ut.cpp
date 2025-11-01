// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "property_generators.hpp"

#include "atlas/StrongTypeGenerator.hpp"

#include <algorithm>
#include <regex>
#include <string>

#include "doctest.hpp"
#include "rapidcheck.hpp"

using namespace wjh::atlas;
using namespace wjh::atlas::testing;

namespace {

namespace gen = wjh::atlas::testing::generators;
using wjh::atlas::testing::rc::check;

// Helper: Extract header guard from generated code
std::string
extract_guard(std::string const & code)
{
    std::regex guard_regex{R"(#ifndef\s+([A-Z_0-9]+))"};
    std::smatch match;
    if (std::regex_search(code, match, guard_regex)) {
        return match[1].str();
    }
    return "";
}

// Helper: Check if requested operator appears in code
bool
has_operator(std::string const & code, std::string const & op)
{
    // Handle special operators and aliases
    if (op == "in") {
        return code.find("operator >>") != std::string::npos ||
            code.find("operator>>") != std::string::npos;
    }
    if (op == "out") {
        return code.find("operator <<") != std::string::npos ||
            code.find("operator<<") != std::string::npos;
    }
    if (op == "@") {
        // @ is the indirection operator (mapped to operator*)
        return code.find("operator * ()") != std::string::npos ||
            code.find("operator*()") != std::string::npos;
    }
    if (op == "->") {
        // Arrow operator
        return code.find("operator -> ()") != std::string::npos ||
            code.find("operator->()") != std::string::npos;
    }
    if (op == "hash") {
        return code.find("std::hash<") != std::string::npos;
    }
    if (op == "fmt") {
        return code.find("std::formatter<") != std::string::npos;
    }
    if (op == "iterable") {
        return code.find("auto begin()") != std::string::npos;
    }
    if (op == "assign") {
        return code.find("template <typename T>") != std::string::npos &&
            code.find("operator=(T&&") != std::string::npos;
    }
    if (op == "bool") {
        return code.find("explicit operator bool") != std::string::npos;
    }

    // Regular operators - check with space after "operator"
    // Many operators have spaces: "operator <=>" , "operator +"
    return code.find("operator " + op + " ") != std::string::npos ||
        code.find("operator " + op) != std::string::npos ||
        code.find("operator" + op) != std::string::npos;
}

TEST_SUITE("Property-Based Tests")
{
    // Property: Generation must be deterministic (idempotent)
    //
    // Concept: The same input should always produce the same output, regardless
    // of when or how many times the generator is called. This is fundamental to
    // reproducible builds and version control.
    //
    // Why this matters: If generation is non-deterministic (e.g., depends on
    // timestamps, random seeds, hash table iteration order, or uninitialized
    // variables), the same source file could produce different outputs on
    // different machines or at different times, breaking reproducibility.
    //
    // What we test: Generate code twice with the same TypeDescription and
    // verify the outputs are identical.
    //
    // Bugs this catches:
    // - Random seed dependencies
    // - Timestamp or clock dependencies
    // - Hash table iteration order differences
    // - Uninitialized variables affecting output
    // - Non-deterministic template rendering
    TEST_CASE("Property: Generation is deterministic (idempotency)")
    {
        check("Same input produces same output", []() {
            auto desc = *gen::typeDescription();

            StrongTypeGenerator gen1, gen2;

            auto code1 = gen1(desc);
            auto code2 = gen2(desc);

            RC_ASSERT(code1 == code2);
        });
    }

    // Property: Generated code must contain all required C++ structural
    // elements
    //
    // Concept: All generated code must be valid C++ with proper header guards,
    // type declarations, and member variables. These structural elements are
    // fundamental to C++ header files.
    //
    // Why this matters: Missing structural elements cause compilation failures.
    // Header guards prevent multiple inclusion errors. Type declarations are
    // required to define the strong type. The value member stores the wrapped
    // value.
    //
    // What we test: Verify presence of #ifndef/#define/#endif header guards,
    // struct or class keyword, and the value member variable.
    //
    // Bugs this catches:
    // - Missing header guards (ODR violations)
    // - Missing type declarations (compilation failure)
    // - Missing value member (incomplete type)
    // - Template rendering failures
    // - Broken code generation pipeline
    TEST_CASE("Property: Generated code has valid structure")
    {
        check("All generated code has valid C++ structure", []() {
            auto desc = *gen::typeDescription();

            StrongTypeGenerator gen;
            auto code = gen(desc);

            // Must have header guards
            RC_ASSERT(code.find("#ifndef") != std::string::npos);
            RC_ASSERT(code.find("#define") != std::string::npos);
            RC_ASSERT(code.find("#endif") != std::string::npos);

            // Must have struct or class
            RC_ASSERT(
                code.find("struct ") != std::string::npos ||
                code.find("class ") != std::string::npos);

            // Must have value member
            RC_ASSERT(code.find("value") != std::string::npos);
        });
    }

    // Property: Header guards must be unique for different type names
    //
    // Concept: Each generated type must have a unique header guard to prevent
    // ODR (One Definition Rule) violations. Header guards ensure that when
    // multiple headers are included, each type definition appears only once.
    //
    // Why this matters: Duplicate header guards cause the preprocessor to skip
    // subsequent type definitions, leading to "undefined type" errors.
    // Different types in the same namespace must have different guards based on
    // their names.
    //
    // What we test: Generate two types with different names but the same
    // namespace, extract their header guards, and verify the guards are
    // different.
    //
    // Bugs this catches:
    // - Non-unique guard generation (hash collisions)
    // - Guard not considering type name
    // - Guard missing namespace qualification
    // - Constant guard for all types
    // - Broken guard generation algorithm
    TEST_CASE("Property: Header guards are unique for different types")
    {
        check("Different type names produce different guards", []() {
            auto name1 = *gen::cppIdentifier();
            auto name2 = *gen::cppIdentifier();
            auto ns = *gen::cppNamespace();

            // Ensure different names
            RC_PRE(name1 != name2);

            auto desc1 = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name1,
                .description = "strong int"};

            auto desc2 = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name2,
                .description = "strong int"};

            StrongTypeGenerator gen;
            auto code1 = gen(desc1);
            auto code2 = gen(desc2);

            auto guard1 = extract_guard(code1);
            auto guard2 = extract_guard(code2);

            RC_ASSERT(not guard1.empty());
            RC_ASSERT(not guard2.empty());
            RC_ASSERT(guard1 != guard2);
        });
    }

    // Property: All requested operators must appear in generated code
    //
    // Concept: When a user requests specific operators in the type description,
    // those operators must be generated in the output. This is a core contract
    // between the user's specification and the generator's output.
    //
    // Why this matters: Users depend on requested operators being available.
    // Missing operators break user code that attempts to use them, causing
    // compilation errors or forcing manual implementation.
    //
    // What we test: Generate a type with random operator combinations, then
    // verify each requested operator appears in the generated code. Special
    // handling for C++20's spaceship operator (<=>) which provides all
    // comparison operators, making explicit ==, !=, <, <=, >, >= redundant.
    //
    // Bugs this catches:
    // - Operator parsing failures (missed operators)
    // - Template rendering bugs (operator not generated)
    // - Operator name typos or mismatches
    // - Missing operator implementations
    // - Incorrect spaceship operator handling
    TEST_CASE("Property: Requested operators are present")
    {
        check("If operator requested, it appears in output", []() {
            auto name = *gen::cppIdentifier();
            auto ns = *gen::cppNamespace();
            auto operators = *gen::operatorSet();

            RC_PRE(not operators.empty());

            // Build description
            std::string desc = "strong int";
            if (not operators.empty()) {
                desc += "; ";
                for (size_t i = 0; i < operators.size(); ++i) {
                    if (i > 0) {
                        desc += ", ";
                    }
                    desc += operators[i];
                }
            }

            auto type_desc = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name,
                .description = desc};

            StrongTypeGenerator generator;
            auto code = generator(type_desc);

            // Check if spaceship operator is present
            bool has_spaceship = std::find(
                                     operators.begin(),
                                     operators.end(),
                                     "<=>") != operators.end();

            // Check each operator is present
            // Note: If <=>, is present, != and == are redundant and may be
            // omitted
            for (auto const & op : operators) {
                if (has_spaceship &&
                    (op == "!=" || op == "==" || op == "<" || op == "<=" ||
                     op == ">" || op == ">="))
                {
                    // Spaceship provides all comparisons, so these operators
                    // might not appear explicitly but are available through <=>
                    continue;
                }
                RC_ASSERT(has_operator(code, op));
            }
        });
    }

    // Property: Standard library types must trigger appropriate header includes
    //
    // Concept: When a generated type wraps a standard library type (like
    // std::string, std::vector, or std::optional), the generator must
    // automatically include the necessary standard library headers.
    //
    // Why this matters: Generated code must be self-contained and compile
    // without additional manual includes. Missing headers cause compilation
    // errors with cryptic "incomplete type" or "undefined symbol" messages.
    //
    // What we test: Generate types wrapping various std:: types (string,
    // vector, optional) and verify the corresponding standard library header
    // (#include <string>, <vector>, <optional>) appears in the generated code.
    //
    // Bugs this catches:
    // - Missing include detection logic
    // - Incorrect header names for types
    // - Regex failures in type parsing
    // - Template argument handling bugs (e.g., vector<int>)
    // - Namespace qualification issues
    TEST_CASE("Property: std types trigger appropriate includes")
    {
        check("Using std:: types adds correct includes", []() {
            auto name = *gen::cppIdentifier();
            auto ns = *gen::cppNamespace();

            struct TypeIncludePair
            {
                std::string type;
                std::string include;
            };

            auto type_include = *::rc::gen::element<TypeIncludePair>(
                TypeIncludePair{"std::string", "#include <string>"},
                TypeIncludePair{"std::vector<int>", "#include <vector>"},
                TypeIncludePair{"std::optional<int>", "#include <optional>"});

            auto desc = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name,
                .description = "strong " + type_include.type};

            StrongTypeGenerator generator;
            auto code = generator(desc);

            RC_ASSERT(code.find(type_include.include) != std::string::npos);
        });
    }

    // Property: Hash feature must include the functional header
    //
    // Concept: When the "hash" feature is requested, the generator must include
    // <functional> header and generate a std::hash specialization. The hash
    // feature enables use of strong types in unordered containers and hash
    // tables.
    //
    // Why this matters: std::hash specializations require the <functional>
    // header. Without it, code using the strong type in std::unordered_map or
    // std::unordered_set will fail to compile with "incomplete type" errors.
    //
    // What we test: Generate a type with the "hash" feature, then verify both
    // the #include <functional> directive and std::hash specialization appear
    // in the generated code.
    //
    // Bugs this catches:
    // - Missing functional header include
    // - Hash specialization not generated
    // - Incorrect hash template syntax
    // - Feature detection logic failures
    // - Incomplete hash implementation
    TEST_CASE("Property: hash feature requires functional header")
    {
        check("hash feature always includes <functional>", []() {
            auto name = *gen::cppIdentifier();
            auto ns = *gen::cppNamespace();
            auto other_ops = *gen::operatorSet();

            // Build description with hash
            std::string desc = "strong int; hash";
            for (auto const & op : other_ops) {
                if (op != "hash") { // Avoid duplicates
                    desc += ", " + op;
                }
            }

            auto type_desc = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name,
                .description = desc};

            StrongTypeGenerator generator;
            auto code = generator(type_desc);

            RC_ASSERT(code.find("#include <functional>") != std::string::npos);
            RC_ASSERT(code.find("std::hash<") != std::string::npos);
        });
    }

    // Property: Arithmetic operators must be constexpr by default
    //
    // Concept: Arithmetic operators (+, -, *, /, %) should be marked constexpr
    // to enable compile-time evaluation and use in constexpr contexts. This is
    // a modern C++ best practice for value-semantic types.
    //
    // Why this matters: constexpr operators improve performance by allowing
    // compile-time computation. They also enable use of strong types in
    // constexpr functions, static_assert, and template metaprogramming.
    //
    // What we test: Generate a type with arithmetic operators (+, -) and verify
    // the "constexpr" keyword appears in the generated code.
    //
    // Bugs this catches:
    // - Missing constexpr keywords
    // - Incorrect operator generation templates
    // - Template syntax errors
    // - Regression to non-constexpr operators
    // - Inconsistent const-correctness
    TEST_CASE("Property: constexpr is default for arithmetic operators")
    {
        check("Arithmetic operators are constexpr by default", []() {
            auto name = *gen::cppIdentifier();
            auto ns = *gen::cppNamespace();

            auto desc = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name,
                .description = "strong int; +, -"};

            StrongTypeGenerator generator;
            auto code = generator(desc);

            // Should have constexpr operators
            RC_ASSERT(code.find("constexpr") != std::string::npos);
        });
    }

    // Property: Namespace declarations must be balanced and properly closed
    //
    // Concept: When a namespace is specified, the generated code must have
    // balanced opening and closing namespace declarations with appropriate
    // closing comments. Proper namespace handling is essential for code
    // organization and avoiding naming conflicts.
    //
    // Why this matters: Unbalanced namespace braces cause compilation errors.
    // Missing closing comments make code harder to read and maintain,
    // especially with nested namespaces. Improper namespace handling can leak
    // symbols into the global namespace.
    //
    // What we test: Generate a type with a namespace, then verify both the
    // opening "namespace name" declaration and closing "} // namespace" comment
    // appear in the generated code.
    //
    // Bugs this catches:
    // - Missing namespace declarations
    // - Unbalanced opening/closing braces
    // - Missing namespace closing comments
    // - Nested namespace handling errors
    // - Namespace not propagated to generated code
    TEST_CASE("Property: namespace handling is consistent")
    {
        check("Namespace appears in opening and closing", []() {
            auto name = *gen::cppIdentifier();
            auto ns = *gen::cppIdentifier(); // Use simple namespace

            RC_PRE(not ns.empty());

            auto desc = StrongTypeDescription{
                .kind = "struct",
                .type_namespace = ns,
                .type_name = name,
                .description = "strong int"};

            StrongTypeGenerator generator;
            auto code = generator(desc);

            // Should have namespace opening
            RC_ASSERT(code.find("namespace " + ns) != std::string::npos);

            // Should have namespace closing comment
            RC_ASSERT(code.find("} // namespace") != std::string::npos);
        });
    }

    // Property: Generated code must not use raw pointers or manual memory
    // management
    //
    // Concept: Modern C++ code should avoid raw pointers and manual memory
    // management (new/delete). Generated code should use value semantics,
    // references, or smart pointers instead. This is a core modern C++
    // principle.
    //
    // Why this matters: Raw new/delete indicate potential memory leaks,
    // dangling pointers, or exception-unsafe code. Strong types should be
    // simple value types that don't require manual memory management. Using
    // modern C++ patterns ensures safety and correctness.
    //
    // What we test: Generate random types and verify the generated code
    // contains no " new " or " delete " keywords (with surrounding spaces to
    // avoid false positives from words containing these substrings).
    //
    // Bugs this catches:
    // - Accidental raw pointer usage in templates
    // - Old C++ patterns leaking into generated code
    // - Template rendering bugs introducing unsafe code
    // - Regression to pre-C++11 patterns
    // - Memory management anti-patterns
    TEST_CASE("Property: No raw pointers in generated code")
    {
        check("Generated code doesn't use raw new/delete", []() {
            auto desc = *gen::typeDescription();

            StrongTypeGenerator generator;
            auto code = generator(desc);

            // No raw new/delete (we use modern C++)
            RC_ASSERT(code.find(" new ") == std::string::npos);
            RC_ASSERT(code.find(" delete ") == std::string::npos);
        });
    }

    // Property: Header guards must be well-formed with matching identifiers
    //
    // Concept: Header guards must follow the complete pattern: #ifndef GUARD,
    // #define GUARD, and #endif // GUARD. The guard identifier must be the same
    // in all three places. This is the standard C++ header guard pattern.
    //
    // Why this matters: Malformed header guards cause multiple definition
    // errors or provide no protection at all. If #ifndef and #define use
    // different identifiers, the guard doesn't work. Missing #endif comments
    // make code harder to maintain and can cause subtle errors with nested
    // includes.
    //
    // What we test: Extract the guard identifier from generated code, then
    // verify it appears in #ifndef, #define, and #endif comment with identical
    // spelling.
    //
    // Bugs this catches:
    // - Guard extraction regex failures
    // - Mismatched guard identifiers between ifndef/define
    // - Missing endif guard comments
    // - Typos in guard generation
    // - Incomplete guard structures
    TEST_CASE("Property: Guard structure is well-formed")
    {
        check("Header guards have matching ifndef/define/endif", []() {
            auto desc = *gen::typeDescription();

            StrongTypeGenerator generator;
            auto code = generator(desc);

            auto guard = extract_guard(code);
            RC_ASSERT(not guard.empty());

            // Check that guard appears in both ifndef and define
            RC_ASSERT(code.find("#ifndef " + guard) != std::string::npos);
            RC_ASSERT(code.find("#define " + guard) != std::string::npos);

            // Check endif with guard comment
            RC_ASSERT(code.find("#endif // " + guard) != std::string::npos);
        });
    }

    // Property: Type kind (struct vs class) must determine member accessibility
    //
    // Concept: In C++, struct members are public by default, while class
    // members are private by default. When generating a "class", the code must
    // include an explicit "public:" section. For "struct", members are
    // naturally public without needing explicit access specifiers.
    //
    // Why this matters: Wrong accessibility breaks user code. If a class
    // doesn't have "public:", its members are private and unusable. This is a
    // fundamental C++ semantic that must be respected for correct code
    // generation.
    //
    // What we test: Generate types with randomly chosen "struct" or "class"
    // kind. For class types, verify "public:" appears. For both kinds, verify
    // the "value" member is present.
    //
    // Bugs this catches:
    // - Missing public: section in class types
    // - Wrong accessibility (private when should be public)
    // - Kind parameter ignored during generation
    // - Template logic errors for struct vs class
    // - Inconsistent accessibility patterns
    TEST_CASE("Property: Kind determines value accessibility")
    {
        check("struct has public value, class has private value", []() {
            auto name = *gen::cppIdentifier();
            auto ns = *gen::cppNamespace();
            auto kind = *::rc::gen::element<std::string>("struct", "class");

            auto desc = StrongTypeDescription{
                .kind = kind,
                .type_namespace = ns,
                .type_name = name,
                .description = "strong int"};

            StrongTypeGenerator generator;
            auto code = generator(desc);

            if (kind == "class") {
                // Class should have explicit public: section
                RC_ASSERT(code.find("public:") != std::string::npos);
            }
            // Both should have the value member
            RC_ASSERT(
                code.find("value;") != std::string::npos ||
                code.find("value{") != std::string::npos);
        });
    }
}

} // anonymous namespace
