// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_4F59B4312A2E4CF9BE42CEE05C67CEC3
#define WJH_ATLAS_4F59B4312A2E4CF9BE42CEE05C67CEC3

#include <map>
#include <string>
#include <vector>

namespace wjh::atlas::generation {
struct ClassInfo;
}

namespace wjh::atlas {

struct StrongTypeDescription
{
    /**
     * Whether the type is generated as a struct or class.  If a struct, then
     * the wrapped value will be public.  If a class, then the wrapped value
     * will be private.
     */
    std::string kind = "struct";

    /**
     * The fully qualified namespace of the strong type, type_name.
     */
    std::string type_namespace;

    /**
     * The name of the strong type to be generated, minus the namespace.  It may
     * include parent class scopes.
     *
     * For example, consider namespace a::b { struct A { struct B { }; }; }
     * with a strong type declared inside B, with the name C.  The namespace
     * would be "a::b" and the type_name would be "A::B::C"
     */
    std::string type_name;

    /**
     * A text description of the strong type that is to be generated.
     *
     * There are LOTS of options.  Usually, you will use only a few of them, but
     * they are provided to allow flexibility, which will hopefully encourage
     * the use of strong types in more places.
     *
     * The input is expected to be in a particular format of 'strong <type>;
     * <opt>, ..., <opt>'.
     *
     * All generated functions are marked constexpr where possible, enabling
     * compile-time evaluation. This includes constructors, cast operators,
     * comparison operators, arithmetic operators, logical operators,
     * increment/decrement, access operators, call operators (including callable
     * with invocable), and hash functions. Only stream operators (in/out) are
     * NOT marked constexpr.
     *
     * The <type> argument is the wrapped type, e.g. 'unsigned' or 'std::string'
     * or 'some_template<with, parameters>'.
     *
     * The <opt> values indicate what supplemental support is to be generated
     * for the strong type.  Unrecognized values will cause an exception to be
     * thrown.
     *
     * These <opt> values are recognized.
     *
     * Each binary arithmetic operator ("+", "-", "*", "/", "%", "&", "|", "^",
     * "<<", ">>") will generate two friend functions, one to apply the operator
     * on the wrapped type, and the other to implement the corresponding
     * assignment operator.
     *
     * Each unary arithmetic operator (+, -, and ~) will return a strong type,
     * with the corresponding operator applied to the wrapped type.  Note,
     * however, that the option names are slightly different to differentiate
     * from their binary counterparts ("u+", "u-", "u~", "~").  The bitwise not
     * operator has a "u"-version for consistency with the others, even though
     * it is not really required.
     *
     * Two special options ("+*", "-*") are shorthand for "+, u+" and "-, u-"
     * and will generate both the binary and unary operators.
     *
     * The spaceship operator ("<=>") will generate a default implementation of
     * the three-way-comparison operator.
     *
     * Each of the other six comparison operators ("==", "!=", "<=", ">=", "<",
     * ">") will apply that operator on the wrapped type.
     *
     * The binary and unary logical operators ("!", "not", "||", "or", "&&",
     * "or") are applied to the wrapped type.  General guidelines are to not
     * overload || and &&, but the interface does not prohibit such use.
     *
     * The pre/post increment/decrement ("++", "--") are applied on the wrapped
     * type.  There is no separation of the pre/post operators.  If you specify
     * one, then you get both pre and post versions.
     *
     * The indirection operator (*) has a different name ("@") due to conflicts
     * with the multiplication operator, and will return a reference to the
     * wrapped type.
     *
     * The address-of and member-of-pointer operators ("&of", "->") will each
     * return a pointer to the wrapped type.  The address-of operator is named
     * with "&of" instead of "&" because that is already used for the
     * bitwise-and operator.
     *
     * All generated strong types will get an explicit conversion operator for
     * the wrapped type.  The <opt> "bool" will add an an explicit conversion to
     * bool operator, applied directly to the wrapped type.
     *
     * The two iostream operators (<<, >>) can be implemented on the wrapped
     * type, but they also go by different option names ("in", out") for the
     * istream and ostream operators, respectively.  The generated
     * implementations are placed with the generated class, which means that
     * <istream> and <ostream> will be added to the included headers if the
     * operators are included.
     *
     * The <opt> "()" will generate a nullary call operator that returns the
     * wrapped object.
     *
     * The <opt> "(&)" will generate a call operator that takes a callable
     * object that gets invoked with a reference to the wrapped object.
     *
     * The <opt> "[]" will generate a subscript operator that forwards all
     * arguments to the wrapped object. This supports both single-argument
     * (C++20) and multi-argument (C++23) subscripting through variadic
     * templates.
     *
     * The <opt> "hash" will generate a std::hash specialization for the strong
     * type, enabling its use in std::unordered_map, std::unordered_set, and
     * other hash-based containers. The specialization is placed outside the
     * type's namespace but inside the header guard, and delegates hashing to
     * std::hash of the underlying type. The hash function is conditionally
     * noexcept based on the underlying type's hash function.
     *
     * The <opt> "iterable" will generate iterator support for container-like
     * types, enabling range-based for loops and STL algorithm usage. This
     * generates: (1) iterator type aliases (iterator, const_iterator,
     * value_type), and (2) member begin()/end() functions (both const and
     * non-const). Member functions forward to std::begin/std::end which work
     * with both types that have member iterators and types that only provide
     * free function iterators. All iterator functions use decltype for
     * SFINAE-friendly return type deduction and are conditionally noexcept. No
     * free functions are generated as std::begin/std::end will find the members
     * via ADL.
     *
     * The <opt> "fmt" will generate a std::formatter specialization for C++20
     * std::format and std::print support. The specialization is wrapped in
     * __cpp_lib_format >= 202110L feature test macro for compatibility with
     * pre-C++20 compilers. The formatter inherits from the underlying type's
     * formatter and delegates all formatting operations, including custom
     * format specifications.
     *
     * Any <opt> that starts with an octothorpe will designate a required header
     * file that needs to be included.  Depending on the context, it may be
     * easier to use a single quote rather than a double quote.  Either will be
     * used as a double quote.  For example,
     *     + "strong std::string; @, ->, #<string>"
     *     + "strong my:lib::Price; +,-,#'my/lib/Price.hpp'"
     *
     * @note Some standard types will be recognized, and their headers will be
     * automatically included.  The detection is very basic, so it may not catch
     * complicated types.  Currently, these are mostly recognized automatically,
     * and will include the appropriate respective headers: std::any,
     * std::bitset, std::chrono::, std::optional, std::tuple, std::variant,
     * std::string, std::string_view, std::array, std::deque, std::list,
     * std::map, std::queue, std::set, std::span, std::unordered_map,
     * std::unordered_set, std::vector, std::filesystem::, std::regex,
     * std::atomic, std::barrier, std::condition_variable, std::latch,
     * std::mutex, std::semaphore, std::shared_mutex, std::stop_token,
     * std:thread, and the standard integral types.
     */
    std::string description;

    /**
     * Default value for the default constructor.
     * When empty, the default constructor uses default construction (value;).
     * When set, the default constructor initializes to this value
     * (value{default_value}).
     *
     * Examples:
     *   - "42" generates: value{42}
     *   - "3.14159" generates: value{3.14159}
     *   - R"("hello")" generates: value{"hello"}
     *   - "std::vector<int>{1, 2, 3}" generates:
     *         value{std::vector<int>{1, 2, 3}}
     */
    std::string default_value = "";

    /**
     * Named constants for the strong type, similar to scoped enum values.
     * Map of constant name to value. Each constant generates a static member:
     *   static constexpr TypeName name = TypeName(value);
     * or when no-constexpr is specified:
     *   static const TypeName name = TypeName(value);
     *
     * Examples:
     *   - {"zero", "0"} generates: static constexpr Type zero = Type(0);
     *   - {"empty", R"("")"} generates: static constexpr Type empty = Type("");
     *   - {"pi", "3.14159"} generates: static constexpr Type pi =
     * Type(3.14159);
     */
    std::map<std::string, std::string> constants = {};

    /**
     * When empty, the generated header guard will be prefixed with the
     * namespace and type of the generated strong type. Otherwise, this value
     * will be used as the prefix.
     */
    std::string guard_prefix = "";

    /**
     * When the code is generated, a header guard is also created that is unique
     * for that piece of generated code.  The guard is composed of the fully
     * qualified name, which may contain the :: operator.  However, ':' is
     * illegal in a macro, so we need to replace the "::" with something else.
     *
     * If you want to use something different, just set this to something else,
     * but be aware that any symbol containing two consecutive underscores is
     * reserved by the standard.
     *
     * I doubt this will be much of a problem in practice, since the symbol also
     * contains a SHA1 digest, but...
     */
    std::string guard_separator = "_";

    /**
     * When true, the header guard will be converted to uppercase.
     */
    bool upcase_guard = true;

    /**
     * When true, generates free begin() and end() functions to enable
     * range-based for loops via ADL (Argument-Dependent Lookup).
     */
    bool generate_iterators = false;

    /**
     * When true, generates a std::formatter specialization for C++20
     * std::format support, wrapped in __cpp_lib_format feature test macro.
     */
    bool generate_formatter = false;

    /**
     * When true, generates a template assignment operator that accepts any
     * type assignable to the underlying type, using C++20 concepts or C++11
     * SFINAE.
     */
    bool generate_template_assignment = false;

    /**
     * List of explicit cast operators to generate (e.g.,
     * cast<std::string_view>). Each generates: explicit operator TargetType()
     * const
     */
    std::vector<std::string> explicit_casts = {};

    /**
     * List of implicit cast operators to generate (e.g., implicit_cast<bool>).
     * Each generates: operator TargetType() const (no explicit)
     * WARNING: Implicit casts reduce type safety, use sparingly.
     */
    std::vector<std::string> implicit_casts = {};

    /**
     * Target C++ standard for generated code (11, 14, 17, 20, or 23).
     * Defaults to C++11 for maximum compatibility.
     * Generates a static_assert to ensure the code is compiled with the
     * correct standard.
     */
    int cpp_standard = 11;

    /**
     * List of forwarded memfns from the underlying type.
     * Each string can contain comma-separated memfn names, optionally with:
     * - The "const" keyword to generate only const overloads
     * - Alias syntax "memfn:alias" to forward memfn with a different name
     *
     * Examples:
     *   - "size,empty,clear" forwards three memfns
     *   - "const,size,empty" forwards size and empty as const-only
     *   - "size:length,empty:is_empty" forwards with aliases
     *
     * Multiple forward= lines are accumulated and merged.
     */
    std::vector<std::string> forwarded_memfns = {};

    /**
     * Type of constraint to apply (e.g., "positive", "non_negative",
     * "bounded"). Empty string means no constraint.
     */
    std::string constraint_type = "";

    /**
     * Parameters for parameterized constraints (e.g., bounded<min,max>).
     * For bounded: {"min": "0", "max": "100"}
     * For other constraints: empty map
     */
    std::map<std::string, std::string> constraint_params = {};

    /**
     * Whether this type has any constraint applied.
     * Set to true when constraint_type is non-empty.
     */
    bool has_constraint = false;
};

struct StrongTypeGenerator
{
    /**
     * @brief Warning information for diagnostic messages
     */
    struct Warning
    {
        std::string message;
        std::string type_name;
    };

    /**
     * Generate code for a strong type.
     *
     * @return  A string with the entire type definition, including any header
     * file inclusions.  It can be treated as a complete header, or can be
     * merged with others since it gets its own unique header guard.
     *
     * @note  The generated header guard is a combination of the type_name and
     * the SHA1 digest of the generated code.
     *
     * @note  The generator does not enforce that anything you give it is valid
     * C++.  It takes your wrapped type directly as-is, and generates code
     * assuming that its syntax is correct.  Likewise, it does not verify that
     * the provided operators are implemented by the wrapped class.
     */
    std::string operator () (StrongTypeDescription const &);

    /**
     * @brief Get the warnings collected during generation
     */
    std::vector<Warning> const & get_warnings() const { return warnings_; }

    /**
     * @brief Clear all collected warnings
     */
    void clear_warnings() { warnings_.clear(); }

private:
    std::vector<Warning> warnings_;
};

// Removed constexpr instance - use StrongTypeGenerator directly for stateful
// warning collection

/**
 * @brief Generate multiple strong types in a single file with unified header
 * guard
 *
 * This function generates code for multiple strong type definitions,
 * consolidating includes, adding a single NOTICE banner, and using a unified
 * header guard.
 *
 * @param descriptions Vector of type descriptions to generate
 * @param guard_prefix Prefix for the header guard (default: "ATLAS")
 * @param guard_separator Separator between prefix and hash (default: "_")
 * @param upcase_guard Whether to uppercase the guard (default: true)
 * @return Generated C++ header file content
 */
std::string generate_strong_types_file(
    std::vector<StrongTypeDescription> const & descriptions,
    std::string const & guard_prefix = "",
    std::string const & guard_separator = "_",
    bool upcase_guard = true);

} // namespace wjh::atlas

#endif // WJH_ATLAS_4F59B4312A2E4CF9BE42CEE05C67CEC3
