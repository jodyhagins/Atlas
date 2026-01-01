// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_TEMPLATE_SYSTEM_HPP
#define WJH_ATLAS_TEMPLATE_SYSTEM_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace wjh::atlas {

class ProfileSystem; // Forward declaration for validation

/**
 * @brief A type template definition
 *
 * Templates are parameterized type definitions:
 *   [template Optional T]
 *   kind=class
 *   description=std::optional<{T}>; <=>, bool
 *   forward=has_value, operator*
 *
 * Instantiated via:
 *   [use Optional ScheduleId]
 */
struct TypeTemplate
{
    std::string name;                    // "Optional"
    std::vector<std::string> parameters; // ["T"]
    std::string kind;                    // "class" or "struct"
    std::string type_namespace;          // optional namespace
    std::string description;             // "std::optional<{T}>; <=>, bool"
    std::string default_value;           // optional default value
    std::vector<std::string> constants;  // constant definitions
    std::vector<std::string> forwards;   // forwarded member functions
};

/**
 * @brief Template system for user-defined parameterized types
 *
 * Templates allow defining reusable type patterns with parameters:
 *   [template Optional T]
 *   description=std::optional<{T}>
 *
 * Used via the 'use' keyword:
 *   [use Optional ScheduleId]
 *
 * Parameter substitution uses the same {NAME} syntax as profiles.
 */
class TemplateSystem
{
public:
    /**
     * @brief Register a template
     *
     * @param tmpl The template definition
     * @param profile_system Profile system to check for name collisions
     * @throws std::runtime_error if name is invalid, already exists,
     *         or a parameter name conflicts with a profile name
     */
    void register_template(
        TypeTemplate const & tmpl,
        ProfileSystem const & profile_system);

    /**
     * @brief Check if a template exists
     */
    bool has_template(std::string const & name) const;

    /**
     * @brief Get a template definition
     * @throws std::runtime_error if template doesn't exist
     */
    TypeTemplate const & get_template(std::string const & name) const;

    /**
     * @brief Get all registered template names
     */
    std::vector<std::string> get_template_names() const;

    /**
     * @brief Clear all templates (useful for testing)
     */
    void clear();

private:
    /**
     * @brief Validate template name
     * @return true if name is a valid C++ identifier
     */
    static bool is_valid_template_name(std::string const & name);

    /**
     * @brief Validate parameter name
     * @return true if name is a valid C++ identifier
     */
    static bool is_valid_parameter_name(std::string const & name);

    std::unordered_map<std::string, TypeTemplate> templates_;
};

/**
 * @brief Substitute template parameters in a string
 *
 * Replaces {PARAM} with the corresponding argument value.
 *
 * @param input The string containing {PARAM} placeholders
 * @param parameters The parameter names (e.g., ["T", "U"])
 * @param arguments The argument values (e.g., ["int", "string"])
 * @return The string with all parameters substituted
 * @throws std::runtime_error if parameter count doesn't match argument count
 */
std::string substitute_template_params(
    std::string const & input,
    std::vector<std::string> const & parameters,
    std::vector<std::string> const & arguments);

} // namespace wjh::atlas

#endif // WJH_ATLAS_TEMPLATE_SYSTEM_HPP
