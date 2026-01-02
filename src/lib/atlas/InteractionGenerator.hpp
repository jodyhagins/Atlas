// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A4073A493A5C451B8E8D6DDF07C4596A
#define WJH_ATLAS_A4073A493A5C451B8E8D6DDF07C4596A

#include <map>
#include <string>
#include <vector>

namespace wjh::atlas {

/**
 * @brief Type constraint specification for template parameters
 *
 * Represents a constraint on a template type parameter that can be expressed
 * using either C++20 concepts or C++17 SFINAE, or both. When both are
 * provided, the generator will use feature detection macros to emit the
 * appropriate version.
 */
struct TypeConstraint
{
    /**
     * Unique name/identifier for this constraint (e.g., "std::floating_point")
     */
    std::string name = "";

    /**
     * C++20 concept expression (e.g., "std::floating_point")
     * Used in: template<std::floating_point T>
     */
    std::string concept_expr = "";

    /**
     * C++17 SFINAE expression (e.g., "std::is_floating_point_v<T>")
     * Used in: template<typename T,
     * std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
     */
    std::string enable_if_expr = "";

    /**
     * Whether a C++20 concept expression was provided
     */
    bool has_concept() const { return not concept_expr.empty(); }

    /**
     * Whether a C++17 SFINAE expression was provided
     */
    bool has_enable_if() const { return not enable_if_expr.empty(); }
};

/**
 * @brief Description of a binary operator interaction between types
 *
 * Describes how two types interact via a binary operator, producing a result
 * type. The interaction can be symmetric (commutative) or asymmetric.
 */
struct InteractionDescription
{
    /**
     * The operator symbol: "+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>"
     */
    std::string op_symbol;

    /**
     * Left-hand side type name or type constraint name
     * Examples: "Distance", "std::floating_point"
     */
    std::string lhs_type;

    /**
     * Right-hand side type name or type constraint name
     * Examples: "Time", "std::integral"
     */
    std::string rhs_type;

    /**
     * Result type name
     * Examples: "Velocity", "double"
     */
    std::string result_type;

    /**
     * Whether the operation is symmetric (commutative)
     * true: generates both LHS OP RHS and RHS OP LHS
     * false: generates only LHS OP RHS
     */
    bool symmetric = false;

    /**
     * Whether LHS is a type constraint (template parameter)
     */
    bool lhs_is_template = false;

    /**
     * Whether RHS is a type constraint (template parameter)
     */
    bool rhs_is_template = false;

    /**
     * Whether to generate constexpr qualifier
     */
    bool is_constexpr = true;

    /**
     * Namespace for this interaction
     */
    std::string interaction_namespace = "";

    /**
     * How to access underlying value for LHS type
     * Examples: "atlas::to_underlying", ".value", "get_value"
     * If empty, falls back to value_access
     */
    std::string lhs_value_access = "";

    /**
     * How to access underlying value for RHS type
     * Examples: "atlas::to_underlying", ".value", "get_value"
     * If empty, falls back to value_access
     */
    std::string rhs_value_access = "";

    /**
     * Default way to access underlying value of types
     * Used when lhs_value_access or rhs_value_access is not specified
     * Examples: "atlas::to_underlying", ".value", "get_value"
     * If empty, uses atlas::to_underlying
     */
    std::string value_access = "";
};

/**
 * @brief Complete description of an interaction file
 *
 * Contains all information needed to generate operator interactions between
 * types, including includes, type constraints, and the interactions themselves.
 */
struct InteractionFileDescription
{
    /**
     * Include directives to emit at the top of the generated file
     * Examples: "Distance.hpp", "<concepts>", "<atlas/value.hpp>"
     */
    std::vector<std::string> includes = {};

    /**
     * Map of constraint name to constraint definition
     * Key: constraint name (e.g., "std::floating_point")
     * Value: TypeConstraint with concept and/or enable_if expressions
     */
    std::map<std::string, TypeConstraint> constraints = {};

    /**
     * List of all operator interactions to generate
     */
    std::vector<InteractionDescription> interactions = {};

    /**
     * Prefix for the header guard (empty = use "ATLAS")
     */
    std::string guard_prefix = "";

    /**
     * Separator between guard prefix and hash
     */
    std::string guard_separator = "_";

    /**
     * Whether to uppercase the header guard
     */
    bool upcase_guard = true;

    /**
     * Target C++ standard for generated code (11, 14, 17, 20, or 23).
     * Defaults to C++11 for maximum compatibility.
     * Generates a static_assert to ensure the code is compiled with the
     * correct standard.
     */
    int cpp_standard = 11;
};

/**
 * @brief Generator for operator interactions between types
 *
 * Generates free function operators that define how different types interact
 * through binary operators. Supports both C++20 concepts and C++17 SFINAE
 * with automatic feature detection.
 */
struct InteractionGenerator
{
    /**
     * Generate code for operator interactions.
     *
     * @param desc Complete interaction file description
     * @return A string containing the complete C++ header with all operator
     *         definitions, includes, and header guards
     *
     * @note The generated header guard is based on the guard_prefix and
     *       a SHA1 digest of the generated code.
     *
     * @note The generator assumes that the specified value_access will
     *       successfully extract the underlying value from strong types.
     *       For built-in types, the value is used directly.
     */
    std::string operator () (InteractionFileDescription const & desc) const;
};

inline constexpr auto generate_interactions = InteractionGenerator{};

} // namespace wjh::atlas

#endif // WJH_ATLAS_A4073A493A5C451B8E8D6DDF07C4596A
