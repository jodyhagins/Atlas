// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_CODE_STRUCTURE_PARSER_HPP
#define WJH_ATLAS_CODE_STRUCTURE_PARSER_HPP

#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace wjh::atlas::testing {

/**
 * @brief Represents the structure of generated C++ code
 *
 * This provides a semantic representation of generated code that can be
 * validated, rather than relying on fragile string matching.
 */
struct CodeStructure
{
    // Header guard information
    std::string guard_name;

    // All #include directives (in order)
    std::vector<std::string> includes;

    // Type information
    std::string kind; // "struct" or "class"
    std::string namespace_name;
    std::string type_name;
    std::string full_qualified_name; // namespace::Type
    std::string underlying_type;

    // Member variable
    std::string member_type;
    std::string member_name;
    std::optional<std::string> member_default_value;

    // Visibility
    bool has_public_specifier = false;
    bool has_private_specifier = false;

    // Operator information
    struct Operator
    {
        std::string signature; // Full signature for exact matching
        std::string name; // e.g., "operator +"
        bool is_constexpr = false;
        bool is_friend = false;
        bool is_default = false;
        bool is_const = false;

        // Specific operator checks
        bool is_comparison() const
        {
            return name == "operator ==" || name == "operator !=" ||
                name == "operator <" || name == "operator <=" ||
                name == "operator >" || name == "operator >=" ||
                name == "operator <=>";
        }

        bool is_arithmetic() const
        {
            return name == "operator +" || name == "operator -" ||
                name == "operator *" || name == "operator /" ||
                name == "operator %";
        }
    };

    std::vector<Operator> operators;

    // Hash specialization
    bool has_hash_specialization = false;
    bool hash_is_constexpr = false;

    // Constructor information
    bool has_constexpr_constructor = false;

    /**
     * @brief Find an operator by name (returns first match)
     */
    std::optional<Operator> find_operator(std::string const & op_name) const
    {
        for (auto const & op : operators) {
            if (op.name == op_name) {
                return op;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Find all operators with a given name
     */
    std::vector<Operator> find_all_operators(std::string const & op_name) const
    {
        std::vector<Operator> result;
        for (auto const & op : operators) {
            if (op.name == op_name) {
                result.push_back(op);
            }
        }
        return result;
    }

    /**
     * @brief Count operators matching a predicate
     */
    template <typename Pred>
    size_t count_operators(Pred pred) const
    {
        return std::count_if(operators.begin(), operators.end(), pred);
    }

    /**
     * @brief Check if an include exists
     */
    bool has_include(std::string const & include) const
    {
        return std::find(includes.begin(), includes.end(), include) !=
            includes.end();
    }
};

/**
 * @brief Parse generated code into a CodeStructure for validation
 *
 * This extracts semantic information from the generated code, making tests
 * more robust and less brittle than string matching.
 */
class CodeStructureParser
{
public:
    CodeStructure parse(std::string const & code);

private:
    std::string extract_guard(std::string const & code);
    std::vector<std::string> extract_includes(std::string const & code);
    void extract_type_info(std::string const & code, CodeStructure & result);
    void extract_operators(std::string const & code, CodeStructure & result);
    void extract_hash_info(std::string const & code, CodeStructure & result);
    std::string extract_namespace_name(std::string const & search_area);
};

} // namespace wjh::atlas::testing

#endif // WJH_ATLAS_CODE_STRUCTURE_PARSER_HPP
