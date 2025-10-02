// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"
#include "InteractionGenerator.hpp"

#include <boost/mustache.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace wjh::atlas { inline namespace v1 {

namespace {

// Template for generic compound assignment operator (ONE for all interactions)
// Two overloads with priority via tag dispatch:
// 1. If wrapped values support compound assignment, use it (optimized, no temporary)
// 2. Otherwise fall back to binary operator + assignment (creates temporary)
static constexpr char const compound_operator_template[] = R"(
namespace atlas_detail {
    // Helper to detect if compound assignment is supported
    template<typename L, typename R, typename = void>
    struct has_compound_op_{{{op_id}}} : std::false_type {};

    template<typename L, typename R>
    struct has_compound_op_{{{op_id}}}<L, R,
        decltype((void)(atlas::value(std::declval<L&>()) {{{compound_op}}}
            atlas::value(std::declval<R const&>())))>
    : std::true_type {};

    // Optimized version: use compound assignment on wrapped values directly
    template<typename L, typename R>
    inline auto compound_assign_impl_{{{op_id}}}(L & lhs, R const & rhs, std::true_type)
    -> L &
    {
        atlas::value(lhs) {{{compound_op}}} atlas::value(rhs);
        return lhs;
    }

    // Fallback version: use binary operator and assignment
    template<typename L, typename R>
    inline auto compound_assign_impl_{{{op_id}}}(L & lhs, R const & rhs, std::false_type)
    -> L &
    {
        atlas::value(lhs) = atlas::value(lhs {{{binary_op}}} rhs);
        return lhs;
    }
}

    // Public interface
    template<
        typename L,
        typename R,
        typename std::enable_if<
            std::is_base_of<atlas::strong_type_tag, L>::value,
            bool>::type = true>
    inline auto operator{{{compound_op}}}(L & lhs, R const & rhs)
    -> decltype(atlas_detail::compound_assign_impl_{{{op_id}}}(lhs, rhs,
        atlas_detail::has_compound_op_{{{op_id}}}<L, R>{}))
    {
        return atlas_detail::compound_assign_impl_{{{op_id}}}(lhs, rhs,
            atlas_detail::has_compound_op_{{{op_id}}}<L, R>{});
    }
)";

std::string
generate_template_header(
    TypeConstraint const & constraint,
    std::string const & template_param_name)
{
    std::ostringstream oss;

    if (constraint.has_concept() && constraint.has_enable_if()) {
        // Both available - use feature detection
        oss << "#if __cpp_concepts >= 201907L\n";
        oss << "template<" << constraint.concept_expr << " "
            << template_param_name << ">\n";
        oss << "#else\n";
        oss << "template<typename " << template_param_name << ", "
            << "typename std::enable_if<" << constraint.enable_if_expr
            << ", bool>::type = true>\n";
        oss << "#endif\n";
    } else if (constraint.has_concept()) {
        oss << "template<" << constraint.concept_expr << " "
            << template_param_name << ">\n";
    } else if (constraint.has_enable_if()) {
        oss << "template<typename " << template_param_name << ", "
            << "typename std::enable_if<" << constraint.enable_if_expr
            << ", bool>::type = true>\n";
    } else {
        throw std::runtime_error(
            "TypeConstraint has neither concept nor enable_if expression");
    }

    return oss.str();
}

// Get the actual type name to use in the function signature
std::string
get_signature_type(
    std::string const & type_name,
    bool is_template,
    std::string const & template_param_name = "")
{
    if (is_template) {
        return template_param_name;
    }
    return type_name;
}

// Generate value access expression
std::string
generate_value_access(
    std::string const & var_name,
    std::string const & specific_access,
    std::string const & default_access)
{
    // Use specific access if provided, otherwise fall back to default
    std::string value_access = specific_access.empty() ? default_access
                                                       : specific_access;
    if (value_access.empty()) {
        value_access = "atlas::value";
    }
    if (value_access[0] == '.') {
        // Member access: .value, .get(), etc.
        return var_name + value_access;
    } else if (value_access == "()") {
        // Function call operator
        return var_name + "()";
    } else {
        // Function call: get_value, extract, atlas::value, etc.
        return value_access + "(" + var_name + ")";
    }
}

// Generate a single operator function
std::string
generate_operator_function(
    InteractionDescription const & interaction,
    std::map<std::string, TypeConstraint> const & constraints,
    bool reverse = false)
{
    std::ostringstream oss;

    std::string lhs_type = reverse ? interaction.rhs_type
                                   : interaction.lhs_type;
    std::string rhs_type = reverse ? interaction.lhs_type
                                   : interaction.rhs_type;
    bool lhs_is_template = reverse ? interaction.rhs_is_template
                                   : interaction.lhs_is_template;
    bool rhs_is_template = reverse ? interaction.lhs_is_template
                                   : interaction.rhs_is_template;

    // Generate template headers if needed
    if (lhs_is_template && rhs_is_template) {
        // Both are templates - generate two template parameters
        if (not constraints.contains(lhs_type)) {
            throw std::runtime_error(
                "Template type '" + lhs_type +
                "' used but no constraint defined");
        }
        if (not constraints.contains(rhs_type)) {
            throw std::runtime_error(
                "Template type '" + rhs_type +
                "' used but no constraint defined");
        }
        auto lhs_constraint = constraints.at(lhs_type);
        auto rhs_constraint = constraints.at(rhs_type);
        oss << generate_template_header(lhs_constraint, "TL");
        oss << generate_template_header(rhs_constraint, "TR");
    } else if (lhs_is_template) {
        if (not constraints.contains(lhs_type)) {
            throw std::runtime_error(
                "Template type '" + lhs_type +
                "' used but no constraint defined");
        }
        auto constraint = constraints.at(lhs_type);
        oss << generate_template_header(constraint, "T");
    } else if (rhs_is_template) {
        if (not constraints.contains(rhs_type)) {
            throw std::runtime_error(
                "Template type '" + rhs_type +
                "' used but no constraint defined");
        }
        auto constraint = constraints.at(rhs_type);
        oss << generate_template_header(constraint, "T");
    }

    // Generate function signature
    if (interaction.is_constexpr) {
        oss << "constexpr ";
    }

    oss << interaction.result_type << " operator" << interaction.op_symbol
        << "(";

    // Determine actual parameter types
    std::string lhs_param_type = get_signature_type(
        lhs_type,
        lhs_is_template,
        lhs_is_template && rhs_is_template ? "TL" : "T");
    std::string rhs_param_type = get_signature_type(
        rhs_type,
        rhs_is_template,
        lhs_is_template && rhs_is_template ? "TR" : "T");

    oss << lhs_param_type << " lhs, " << rhs_param_type << " rhs";
    oss << ")\n{\n";

    // Generate function body - use specific value access or fall back to
    // default
    std::string lhs_value = generate_value_access(
        "lhs",
        reverse ? interaction.rhs_value_access : interaction.lhs_value_access,
        interaction.value_access);
    std::string rhs_value = generate_value_access(
        "rhs",
        reverse ? interaction.lhs_value_access : interaction.rhs_value_access,
        interaction.value_access);

    oss << "    return " << interaction.result_type << "{" << lhs_value << " "
        << interaction.op_symbol << " " << rhs_value << "};\n";
    oss << "}\n";

    return oss.str();
}

} // anonymous namespace

std::string
InteractionGenerator::
operator () (InteractionFileDescription const & desc) const
{
    std::ostringstream body;

    // Generate includes
    for (auto const & include : desc.includes) {
        if (include[0] == '<' || include[0] == '"') {
            body << "#include " << include << "\n";
        } else {
            body << "#include \"" << include << "\"\n";
        }
    }

    if (not desc.includes.empty()) {
        body << "\n";
    }

    // Always include <type_traits> and <utility> for atlas::value
    // Embed atlas::value implementation
    body << R"(
// This is boilerplate that is part of every Atlas interaction file.
// Nothing to see here, move along.

#ifndef WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90
#define WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90

#include <type_traits>
#include <utility>

namespace atlas {

#ifndef WJH_ATLAS_34E45276DD204E33A734018DE4B04C40
#define WJH_ATLAS_34E45276DD204E33A734018DE4B04C40
struct strong_type_tag
{
    friend auto
    operator<=>(strong_type_tag const &, strong_type_tag const &) = default;
};
#endif // WJH_ATLAS_34E45276DD204E33A734018DE4B04C40

struct value_tag {};

namespace atlas_detail {

template <typename... Ts>
struct make_void
{
    using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T, typename = void>
struct IsAtlasType
: std::false_type
{ };

template <typename T>
struct IsAtlasType<T, void_t<typename T::atlas_value_type>>
: std::true_type
{ };

template <std::size_t N>
struct PriorityTag
: PriorityTag<N - 1>
{ };

template <>
struct PriorityTag<0u>
{ };

using value_tag = PriorityTag<3>;

template <bool B>
using bool_c = std::integral_constant<bool, B>;
template <typename T>
using bool_ = bool_c<T::value>;
template <typename T>
using not_ = bool_c<not T::value>;
template <typename T, typename U>
using and_ = bool_c<T::value && U::value>;
template <typename T>
using is_lref = std::is_lvalue_reference<T>;
template <typename T, typename U = void>
using enable_if = typename std::enable_if<T::value, U>::type;

template <typename T>
using _t = typename T::type;

void atlas_value();

template <typename T>
constexpr T &
value(T & val, PriorityTag<0>)
{
    return val;
}

template <typename T>
constexpr auto
value(T const & t, PriorityTag<1>)
-> decltype(atlas_value(t, atlas::value_tag{}))
{
    return atlas_value(t, atlas::value_tag{});
}

template <typename T>
constexpr auto
value(T const & t, PriorityTag<2>)
-> decltype(atlas_value(t))
{
    return atlas_value(t);
}

template <typename T, typename U = typename T::atlas_value_type>
using val_t = _t<std::conditional<
    std::is_const<T>::value,
    U const &,
    U &>>;

template <typename T, typename U = val_t<T>>
constexpr auto
value(T & val, PriorityTag<3>)
-> decltype(atlas::atlas_detail::value(static_cast<U>(val), value_tag{}))
{
    return atlas::atlas_detail::value(static_cast<U>(val), value_tag{});
}

class Value
{
    template <
        typename U,
        typename T,
        typename V = _t<std::conditional<is_lref<U &&>::value, T &, T>>>
    static constexpr V rval(T && t)
    {
        return t;
    }

public:
    template <typename T>
    constexpr auto
    operator()(T && t) const
    -> decltype(rval<T>(atlas_detail::value(t, atlas_detail::value_tag{})))
    {
        return rval<T>(atlas_detail::value(t, atlas_detail::value_tag{}));
    }
};

} // namespace atlas_detail

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 9201606L
inline constexpr auto value = atlas_detail::Value{};
#else
template <typename T>
constexpr auto
value(T && t)
-> decltype(atlas_detail::Value{}(std::forward<T>(t)))
{
    return atlas_detail::Value{}(std::forward<T>(t));
}
#endif

} // namespace atlas

#endif // WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90

)";

    // Collect RHS types that need custom atlas_value functions
    // Map: Fully qualified RHS type -> value access expression
    std::map<std::string, std::string> rhs_value_accessors;

    for (auto const & interaction : desc.interactions) {
        // Build fully qualified RHS type name
        std::string fully_qualified_rhs = interaction.rhs_type;
        if (not interaction.interaction_namespace.empty() &&
            fully_qualified_rhs.find("::") == std::string::npos) {
            // RHS is unqualified - prepend the interaction namespace
            fully_qualified_rhs = interaction.interaction_namespace + "::" + interaction.rhs_type;
        }

        // Only generate if RHS has a custom value access that's not atlas::value
        if (not interaction.rhs_value_access.empty() &&
            interaction.rhs_value_access != "atlas::value") {
            rhs_value_accessors[fully_qualified_rhs] = interaction.rhs_value_access;
        } else if (not interaction.value_access.empty() &&
                   interaction.value_access != "atlas::value" &&
                   interaction.rhs_value_access.empty()) {
            // Fallback to value_access if rhs_value_access not specified
            rhs_value_accessors[fully_qualified_rhs] = interaction.value_access;
        }
    }

    // Generate atlas_value functions for RHS types with custom accessors
    if (not rhs_value_accessors.empty()) {
        body << R"(
// Custom value accessors for non-Atlas types
// These allow atlas::value() to work with external library types
// Users can override by providing atlas_value(T const&) without the tag parameter
namespace atlas {
)";

        for (auto const & [rhs_type, value_access] : rhs_value_accessors) {
            body << "inline constexpr auto atlas_value(" << rhs_type
                 << " const& v, value_tag)\n";
            body << "-> decltype(" << generate_value_access("v", value_access, "") << ")\n";
            body << "{\n";
            body << "    return " << generate_value_access("v", value_access, "") << ";\n";
            body << "}\n\n";
        }

        body << R"(} // namespace atlas

)";
    }

    // Collect unique operator symbols that need compound operators
    std::set<std::string> needed_compound_ops;
    static std::map<std::string, std::string> const op_to_compound{
        {"+", "+="},
        {"-", "-="},
        {"*", "*="},
        {"/", "/="},
        {"%", "%="},
        {"&", "&="},
        {"|", "|="},
        {"^", "^="},
        {"<<", "<<="},
        {">>", ">>="}};

    for (auto const & interaction : desc.interactions) {
        auto it = op_to_compound.find(interaction.op_symbol);
        if (it != op_to_compound.end()) {
            needed_compound_ops.insert(it->second);
        }
    }

    // Generate compound operators if needed
    if (not needed_compound_ops.empty()) {
        body << R"(
// Compound assignment operators for cross-type interactions
// These use ADL to be found automatically for atlas strong types
// The decltype ensures they only match when the binary operator is defined
namespace atlas {
)";

        // Map to create unique IDs for each operator
        static std::map<std::string, std::string> const op_ids{
            {"+=", "plus"},
            {"-=", "minus"},
            {"*=", "times"},
            {"/=", "divide"},
            {"%=", "modulo"},
            {"&=", "bitand"},
            {"|=", "bitor"},
            {"^=", "bitxor"},
            {"<<=", "lshift"},
            {">>=", "rshift"}};

        for (auto const & compound_op : needed_compound_ops) {
            // Get the corresponding binary operator (remove '=' from compound
            // op)
            std::string binary_op = compound_op.substr(
                0,
                compound_op.size() - 1);

            // Get unique ID for this operator
            std::string op_id = op_ids.at(compound_op);

            // Populate mustache data
            boost::json::object data{
                {"compound_op", compound_op},
                {"binary_op", binary_op},
                {"op_id", op_id}};

            // Render template
            std::ostringstream oss;
            boost::mustache::render(
                compound_operator_template,
                oss,
                data,
                boost::json::object{});
            body << oss.str();
        }

        body << R"(
} // namespace atlas
)";
    }

    body << R"(

//////////////////////////////////////////////////////////////////////
///
/// These are the droids you are looking for!
///
//////////////////////////////////////////////////////////////////////
)";

    // Group interactions by namespace
    std::map<std::string, std::vector<InteractionDescription>> by_namespace;
    for (auto const & interaction : desc.interactions) {
        by_namespace[interaction.interaction_namespace].push_back(interaction);
    }

    // Generate interactions namespace by namespace
    for (auto const & [ns, interactions] : by_namespace) {
        if (not ns.empty()) {
            body << "namespace " << ns << " {\n\n";
        }

        for (auto const & interaction : interactions) {
            // Generate the primary operator
            body << generate_operator_function(
                interaction,
                desc.constraints,
                false);
            body << "\n";

            // Generate symmetric version if requested
            if (interaction.symmetric) {
                body << generate_operator_function(
                    interaction,
                    desc.constraints,
                    true);
                body << "\n";
            }
        }

        if (not ns.empty()) {
            body << "} // namespace " << ns << "\n\n";
        }
    }

    // Generate header guard
    std::string content = body.str();
    std::string hash = get_sha1(content);
    std::string prefix = desc.guard_prefix.empty() ? "ATLAS"
                                                   : desc.guard_prefix;
    std::string guard = generate_header_guard(
        prefix,
        desc.guard_separator,
        hash,
        desc.upcase_guard);

    // Assemble final output
    std::ostringstream output;
    static constexpr char const banner[] = R"(
// ======================================================================
// NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE
// ----------------------------------------------------------------------
//
// DO NOT EDIT THIS FILE DIRECTLY.
//
// This source file has been generated by Atlas Interaction Generator
// https://github.com/jodyhagins/Atlas
//
// DO NOT EDIT THIS FILE DIRECTLY.
//
// ----------------------------------------------------------------------
// NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE  NOTICE
// ======================================================================
)";
    output << banner + 1;
    output << "#ifndef " << guard << "\n";
    output << "#define " << guard << "\n\n";
    output << content;
    output << "#endif // " << guard << "\n";

    return output.str();
}

}} // namespace wjh::atlas::v1
