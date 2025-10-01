// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"
#include "InteractionGenerator.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace wjh::atlas { inline namespace v1 {

namespace {

// Generate template header for C++20/C++17
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
            << "std::enable_if_t<" << constraint.enable_if_expr
            << ", bool> = true>\n";
        oss << "#endif\n";
    } else if (constraint.has_concept()) {
        // C++20 only
        oss << "template<" << constraint.concept_expr << " "
            << template_param_name << ">\n";
    } else if (constraint.has_enable_if()) {
        // C++17 SFINAE
        oss << "template<typename " << template_param_name << ", "
            << "std::enable_if_t<" << constraint.enable_if_expr
            << ", bool> = true>\n";
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
    std::string const & var_type,
    std::string const & value_access_method,
    bool is_template)
{
    // For built-in types or template parameters, use directly
    if (is_template) {
        return var_name;
    }

    // Check if it's a likely built-in type
    if (var_type == "int" || var_type == "double" || var_type == "float" ||
        var_type == "long" || var_type == "short" || var_type == "char" ||
        var_type == "unsigned" || var_type == "bool")
    {
        return var_name;
    }

    // Apply the specified value access method
    if (value_access_method.empty() || value_access_method == "atlas::value") {
        return "atlas::value(" + var_name + ")";
    } else if (value_access_method[0] == '.') {
        // Member access: .value, .get(), etc.
        return var_name + value_access_method;
    } else {
        // Function call: get_value, extract, etc.
        return value_access_method + "(" + var_name + ")";
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

    // Generate function body
    std::string lhs_value = generate_value_access(
        "lhs",
        lhs_type,
        interaction.value_access,
        lhs_is_template);
    std::string rhs_value = generate_value_access(
        "rhs",
        rhs_type,
        interaction.value_access,
        rhs_is_template);

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

#include <type_traits>
#include <utility>

namespace atlas {

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

using value_tag = PriorityTag<10>;

template <typename T, typename U = typename T::atlas_value_type>
constexpr auto
value(T const & val, PriorityTag<10>)
-> decltype(atlas::atlas_detail::value(static_cast<U const &>(val), value_tag{}))
{
    return atlas::atlas_detail::value(static_cast<U const &>(val), value_tag{});
}

template <typename T>
constexpr typename std::enable_if<std::is_scalar<T>::value, T>::type
value(T val, PriorityTag<1>)
{
    return val;
}

template <typename T>
constexpr T const &
value(T const & val, PriorityTag<0>)
{
    return val;
}

} // namespace atlas_detail

template <typename T>
constexpr auto
value(T const & t)
-> decltype(atlas_detail::value(t, atlas_detail::value_tag{}))
{
    return atlas_detail::value(t, atlas_detail::value_tag{});
}

} // namespace atlas


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
