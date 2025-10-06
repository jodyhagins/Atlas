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
// 1. If wrapped values support compound assignment, use it (optimized, no
// temporary)
// 2. Otherwise fall back to binary operator + assignment (creates temporary)
static constexpr char const compound_operator_template[] = R"(
namespace atlas_detail {
template <typename L, typename R, typename = void>
struct has_compound_op_{{{op_id}}}
: std::false_type
{ };

template <typename L, typename R>
struct has_compound_op_{{{op_id}}}<
    L,
    R,
    decltype((void)(atlas::value(std::declval<L&>()) {{{compound_op}}}
        atlas::value(std::declval<R const&>())))>
: std::true_type
{ };

template <typename L, typename R>
constexpr L &
compound_assign_impl_{{{op_id}}}(L & lhs, R const & rhs, std::true_type)
{
    atlas::value(lhs) {{{compound_op}}} atlas::value(rhs);
    return lhs;
}

template <typename L, typename R>
constexpr L &
compound_assign_impl_{{{op_id}}}(L & lhs, R const & rhs, std::false_type)
{
    atlas::value(lhs) = atlas::value(lhs {{{binary_op}}} rhs);
    return lhs;
}
}

template <
    typename L,
    typename R,
    typename std::enable_if<
        std::is_base_of<atlas::strong_type_tag, L>::value,
        bool>::type = true>
inline auto
operator{{{compound_op}}}(L & lhs, R const & rhs)
-> decltype(atlas_detail::compound_assign_impl_{{{op_id}}}(
    lhs,
    rhs,
    atlas_detail::has_compound_op_{{{op_id}}}<L, R>{}))
{
    return atlas_detail::compound_assign_impl_{{{op_id}}}(
        lhs,
        rhs,
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
        oss << "template <" << constraint.concept_expr << " "
            << template_param_name << ">\n";
        oss << "#else\n";
        oss << "template <typename " << template_param_name << ", "
            << "typename std::enable_if<" << constraint.enable_if_expr
            << ", bool>::type = true>\n";
        oss << "#endif\n";
    } else if (constraint.has_concept()) {
        oss << "template <" << constraint.concept_expr << " "
            << template_param_name << ">\n";
    } else if (constraint.has_enable_if()) {
        oss << "template <typename " << template_param_name << ", "
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

// Type classification for proper name qualification
enum class TypeCategory
{
    Primitive, // int, double, size_t, etc.
    StdLibrary, // std::string, std::vector, etc.
    UserDefined // User's strong types
};

TypeCategory
classify_type(std::string const & type_name)
{
    std::string trimmed = trim(type_name);

    // Check for std:: types
    if (trimmed.find("std::") == 0 || trimmed.find("::std::") == 0) {
        return TypeCategory::StdLibrary;
    }

    // List of primitive types (including from <cstddef>, <cstdint>)
    static std::set<std::string> const primitives = {
        "void",
        "bool",
        "char",
        "signed char",
        "unsigned char",
        "char8_t",
        "char16_t",
        "char32_t",
        "wchar_t",
        "short",
        "signed short",
        "unsigned short",
        "int",
        "signed int",
        "unsigned int",
        "signed",
        "unsigned",
        "long",
        "signed long",
        "unsigned long",
        "long long",
        "signed long long",
        "unsigned long long",
        "float",
        "double",
        "long double",

        // <cstddef> types
        "size_t",
        "ptrdiff_t",
        "nullptr_t",

        // <cstdint> types
        "int8_t",
        "uint8_t",
        "int16_t",
        "uint16_t",
        "int32_t",
        "uint32_t",
        "int64_t",
        "uint64_t",
        "int_fast8_t",
        "uint_fast8_t",
        "int_fast16_t",
        "uint_fast16_t",
        "int_fast32_t",
        "uint_fast32_t",
        "int_fast64_t",
        "uint_fast64_t",
        "int_least8_t",
        "uint_least8_t",
        "int_least16_t",
        "uint_least16_t",
        "int_least32_t",
        "uint_least32_t",
        "int_least64_t",
        "uint_least64_t",
        "intmax_t",
        "uintmax_t",
        "intptr_t",
        "uintptr_t"};

    if (primitives.find(trimmed) != primitives.end()) {
        return TypeCategory::Primitive;
    }

    return TypeCategory::UserDefined;
}

// Generate value access expression
std::string
generate_value_access(
    std::string const & var_name,
    std::string const & type_name,
    std::string const & specific_access,
    std::string const & default_access)
{
    // Classify the type to determine if it's a primitive or std library type
    TypeCategory category = classify_type(type_name);

    // Primitives and std library types don't have .value members
    // Use them directly regardless of value_access settings
    if (category == TypeCategory::Primitive ||
        category == TypeCategory::StdLibrary)
    {
        return var_name;
    }

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

std::string
qualify_type_name(
    std::string const & type_name,
    std::string const & current_namespace)
{
    TypeCategory category = classify_type(type_name);

    switch (category) {
    case TypeCategory::Primitive:
        // Primitives: use as-is, no namespace qualification
        return type_name;

    case TypeCategory::StdLibrary:
        // std:: types: ensure global qualification if not already present
        if (type_name[0] == ':') {
            return type_name; // Already globally qualified
        }
        if (type_name.find("std::") == 0) {
            return "::" + type_name; // Add global qualifier
        }
        return type_name; // Already correct

    case TypeCategory::UserDefined:
        // User-defined types: qualify with namespace if not already qualified
        if (type_name.find("::") != std::string::npos) {
            // Already has namespace qualification
            if (type_name[0] == ':') {
                return type_name; // Already globally qualified
            }
            return "::" + type_name; // Add global qualifier
        }

        // No namespace in type name - use current namespace
        if (current_namespace.empty()) {
            return "::" + type_name;
        }
        return "::" + current_namespace + "::" + type_name;
    }

    return type_name; // Fallback
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
        // Both are templates
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

        // Check if both sides use the same template parameter
        if (lhs_type == rhs_type) {
            // Same type - use single template parameter with original name
            auto constraint = constraints.at(lhs_type);
            oss << generate_template_header(constraint, lhs_type);
        } else {
            // Different types - need two template parameters
            // Generate a combined template declaration
            auto lhs_constraint = constraints.at(lhs_type);
            auto rhs_constraint = constraints.at(rhs_type);

            // TODO: Handle combined template with both concept and enable_if
            // For now, generate simple templates
            if (lhs_constraint.has_concept() && rhs_constraint.has_concept()) {
                oss << "template <" << lhs_constraint.concept_expr << " TL, "
                    << rhs_constraint.concept_expr << " TR>\n";
            } else {
                oss << "template <typename TL, typename TR>\n";
            }
        }
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
    } else {
        oss << "inline ";
    }

    // Generate function signature
    if (interaction.is_constexpr) {
        oss << "constexpr ";
    }

    oss << interaction.result_type << "\noperator" << interaction.op_symbol
        << "(";

    // Determine actual parameter types
    std::string lhs_param_name;
    std::string rhs_param_name;

    if (lhs_is_template && rhs_is_template) {
        // Both are templates
        if (lhs_type == rhs_type) {
            // Same type - use original name
            lhs_param_name = lhs_type;
            rhs_param_name = rhs_type;
        } else {
            // Different types - use TL/TR
            lhs_param_name = "TL";
            rhs_param_name = "TR";
        }
    } else {
        // Only one (or neither) is a template - use T
        lhs_param_name = lhs_is_template ? "T" : "";
        rhs_param_name = rhs_is_template ? "T" : "";
    }

    std::string lhs_param_type =
        get_signature_type(lhs_type, lhs_is_template, lhs_param_name);
    std::string rhs_param_type =
        get_signature_type(rhs_type, rhs_is_template, rhs_param_name);

    oss << lhs_param_type << " lhs, " << rhs_param_type << " rhs";
    oss << ")\n{\n";

    // Generate function body - use specific value access or fall back to
    // default
    std::string lhs_value = generate_value_access(
        "lhs",
        lhs_type,
        reverse ? interaction.rhs_value_access : interaction.lhs_value_access,
        interaction.value_access);
    std::string rhs_value = generate_value_access(
        "rhs",
        rhs_type,
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

    // Always include <type_traits> and <utility> for atlas::value
    // Embed atlas::value implementation
    body << preamble();

    // Collect RHS types that need custom atlas_value functions
    // Map: Fully qualified RHS type -> (value access expression, is_constexpr)
    struct ValueAccessInfo
    {
        std::string access_expr;
        bool is_constexpr;
    };

    std::map<std::string, ValueAccessInfo> rhs_value_accessors;

    for (auto const & interaction : desc.interactions) {
        // Build fully qualified RHS type name using proper type qualification
        std::string fully_qualified_rhs = qualify_type_name(
            interaction.rhs_type,
            interaction.interaction_namespace);

        // Only generate if RHS has a custom value access that's not
        // atlas::value
        std::string value_access_expr;
        if (not interaction.rhs_value_access.empty() &&
            interaction.rhs_value_access != "atlas::value")
        {
            value_access_expr = interaction.rhs_value_access;
        } else if (
            not interaction.value_access.empty() &&
            interaction.value_access != "atlas::value" &&
            interaction.rhs_value_access.empty())
        {
            // Fallback to value_access if rhs_value_access not specified
            value_access_expr = interaction.value_access;
        }

        if (not value_access_expr.empty()) {
            // Skip primitives and std library types - they don't need
            // custom atlas_value overloads
            TypeCategory rhs_category = classify_type(interaction.rhs_type);
            if (rhs_category != TypeCategory::Primitive &&
                rhs_category != TypeCategory::StdLibrary)
            {
                auto it = rhs_value_accessors.find(fully_qualified_rhs);
                if (it == rhs_value_accessors.end()) {
                    // First time seeing this type
                    rhs_value_accessors[fully_qualified_rhs] = ValueAccessInfo{
                        value_access_expr,
                        interaction.is_constexpr};
                } else {
                    // Type already exists - if ANY interaction is
                    // non-constexpr, the atlas_value must be non-constexpr
                    if (not interaction.is_constexpr) {
                        it->second.is_constexpr = false;
                    }
                }
            }
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

        for (auto const & [rhs_type, info] : rhs_value_accessors) {
            body << "inline ";
            if (info.is_constexpr) {
                body << "constexpr ";
            }
            body << "auto\natlas_value(" << rhs_type
                << " const& v, value_tag)\n";
            body << "-> decltype("
                << generate_value_access("v", rhs_type, info.access_expr, "")
                << ")\n";
            body << "{\n";
            body << "    return "
                << generate_value_access("v", rhs_type, info.access_expr, "")
                << ";\n";
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
    output << "#ifndef " << guard << "\n";
    output << "#define " << guard << "\n\n";
    output << banner + 1;
    output << content;
    output << "#endif // " << guard << "\n";

    return output.str();
}

}} // namespace wjh::atlas::v1
