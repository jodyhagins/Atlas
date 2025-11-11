// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_2A78E6C83F92454FA0B5BC113E11CF1A
#define WJH_ATLAS_2A78E6C83F92454FA0B5BC113E11CF1A

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

#include <map>
#include <string>
#include <vector>

// Forward declarations
namespace boost::json {
class object;
class value;
}

namespace wjh::atlas { inline namespace v1 {
struct StrongTypeGenerator;
}}

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * @brief Represents a binary or unary operator for code generation
 *
 * Simple value type holding an operator token (e.g., "+", "-", "*")
 * with optional arithmetic mode specification.
 * Used in ClassInfo to track which operators should be generated.
 */
struct Operator
{
    std::string op;
    ArithmeticMode mode = ArithmeticMode::Default;

    explicit Operator(
        std::string_view op_,
        ArithmeticMode mode_ = ArithmeticMode::Default)
    : op(op_)
    , mode(mode_)
    { }

    friend bool operator < (Operator const & x, Operator const & y)
    {
        return x.op < y.op;
    }

    [[nodiscard]]
    boost::json::object to_json() const;
};

/**
 * @brief Represents a cast operator for code generation
 *
 * Holds the target type for cast operations.
 * Used to generate both explicit and implicit cast operators.
 */
struct CastOperator
{
    std::string cast_type;

    explicit CastOperator(std::string cast_type_)
    : cast_type(std::move(cast_type_))
    { }

    /**
     * @brief Convert this CastOperator to a JSON object
     */
    [[nodiscard]]
    boost::json::object to_json() const;
};

/**
 * @brief Represents a named constant for code generation
 *
 * Used to generate static constant members in the strong type class.
 * Example: { "zero", "0" } generates: static constexpr Type zero{0};
 */
struct Constant
{
    std::string name;
    std::string value;

    Constant(std::string name_, std::string value_)
    : name(std::move(name_))
    , value(std::move(value_))
    { }

    /**
     * @brief Convert this Constant to a JSON object
     */
    [[nodiscard]]
    boost::json::object to_json() const;
};

/**
 * @brief Represents a forwarded member function for code generation
 *
 * Specifies a member function that should be forwarded to the underlying
 * value, with optional aliasing and return type transformation.
 *
 * Design philosophy:
 * - Support const-only forwarding for immutable operations
 * - Allow different ref-qualifiers based on C++ standard
 * - Enable return type transformation (e.g., wrapping result in strong type)
 */
struct ForwardedMemfn
{
    std::string memfn_name; // Original member function name
    std::string alias_name; // Empty if no alias
    std::string return_type; // Empty if no return type transformation
    bool const_only = false; // Only generate const-qualified versions

    // Flags for template rendering - control which overloads are generated
    bool cpp23_or_later = false;
    bool generate_const_no_ref = false; // Just const (no ref qualifier)
    bool generate_const_lvalue = true; // const &
    bool generate_const_rvalue = true; // const &&
    bool generate_nonconst_lvalue = true; // &
    bool generate_nonconst_rvalue = true; // &&

    ForwardedMemfn() = default;

    ForwardedMemfn(
        std::string memfn_name_,
        std::string alias_name_ = "",
        bool const_only_ = false,
        std::string return_type_ = "")
    : memfn_name(std::move(memfn_name_))
    , alias_name(std::move(alias_name_))
    , return_type(std::move(return_type_))
    , const_only(const_only_)
    { }

    /**
     * @brief Convert this ForwardedMemfn to a JSON object
     */
    [[nodiscard]]
    boost::json::object to_json() const;
};

/**
 * @brief Complete metadata for generating a strong type class
 *
 * This is the central data structure that drives template-based code
 * generation. It aggregates all information needed to produce a complete strong
 * type wrapper:
 * - Naming and namespacing
 * - Operator support (arithmetic, relational, logical, etc.)
 * - Type conversions and casts
 * - Member function forwarding
 * - Constraint validation
 * - I/O and hashing support
 *
 * Design philosophy:
 * - Header-only compatible: no BOOST_DESCRIBE dependency
 * - Flat structure: directly accessible by mustache templates
 * - Bool flags: easy conditionals in templates
 * - Vector collections: natural iteration in templates
 *
 * Usage:
 * This struct is populated by StrongTypeGenerator and passed to boost::mustache
 * for template rendering. Each field corresponds to a template variable or
 * section.
 */
struct ClassInfo
{
    // Namespace and naming
    std::string class_namespace = {};
    std::string namespace_open = {};
    std::string namespace_close = {};
    std::string full_class_name = {};
    std::string class_name = {};
    std::string underlying_type = {};

    // Arithmetic operators
    std::vector<Operator> arithmetic_binary_operators = {};
    std::vector<Operator> unary_operators = {};

    // Pointer-like operators
    bool indirection_operator = false;
    std::vector<Operator> addressof_operators = {};
    bool arrow_operator = false;

    // Comparison operators
    bool spaceship_operator = false;
    bool defaulted_equality_operator = false;
    std::vector<Operator> relational_operators = {};
    bool has_relational_operators = false;

    // Increment/decrement
    std::vector<Operator> increment_operators = {};

    // Stream operators
    bool ostream_operator = false;
    bool istream_operator = false;

    // Boolean conversion
    bool bool_operator = false;

    // Function-like operators
    bool nullary = false;
    bool callable = false;

    // Access control
    std::string public_specifier = {};

    // Logical operators
    bool logical_not_operator = false;
    std::vector<Operator> logical_operators = {};

    // Include management
    std::vector<std::string> includes_vec = {};
    std::map<std::string, std::string> include_guards = {};

    // Specialization support
    bool hash_specialization = false;
    bool formatter_specialization = false;

    // Fully qualified name for specializations
    std::string full_qualified_name = {};

    // Container-like operators
    bool subscript_operator = false;

    // Default value support
    bool has_default_value = false;
    std::string default_initializer = "{}";

    // constexpr support
    std::string const_expr = "constexpr ";
    std::string hash_const_expr = "constexpr ";

    // Member variable name (allows customization for nullable types)
    std::string value_member_name = "value";

    // Iterator support
    bool iterator_support_member = false;

    // Template assignment
    bool template_assignment_operator = false;

    // Cast operators
    std::vector<CastOperator> explicit_cast_operators = {};
    std::vector<CastOperator> implicit_cast_operators = {};
    bool has_explicit_casts = false;
    bool has_implicit_casts = false;

    // Named constants
    std::vector<Constant> constants = {};

    // Forwarded member functions
    std::vector<ForwardedMemfn> forwarded_memfns = {};
    bool has_forwarded_memfns = false;

    // Additional qualifiers
    std::string const_qualifier = "constexpr ";

    // C++ standard level
    int cpp_standard = 11;

    // Arithmetic mode (checked, saturating, etc.)
    ArithmeticMode arithmetic_mode = ArithmeticMode::Default;

    // Original description (contains metadata)
    wjh::atlas::v1::StrongTypeDescription desc = {};

    // Constraint validation
    bool has_constraint = false;
    std::string constraint_type = {};
    std::map<std::string, std::string> constraint_params = {};
    std::string constraint_message = {};
    std::string constraint_template_args = {};
    bool is_bounded = false;
    std::string bounded_min = {};
    std::string bounded_max = {};
    bool delete_default_constructor = false;
    bool nil_value_is_constant = false;

    [[nodiscard]]
    boost::json::object to_json() const;

    /**
     * @brief Parse a StrongTypeDescription into a ClassInfo
     * @param desc The strong type description to parse
     * @param warnings Optional vector to collect warning messages
     * @return Parsed ClassInfo ready for template rendering
     */
    static ClassInfo parse(
        wjh::atlas::v1::StrongTypeDescription const & desc,
        std::vector<wjh::atlas::v1::StrongTypeGenerator::Warning> * warnings =
            nullptr);
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_2A78E6C83F92454FA0B5BC113E11CF1A
