// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_9B2E4D6F8A1C5E3B7D9A2F4C6E8B1D3F
#define WJH_ATLAS_9B2E4D6F8A1C5E3B7D9A2F4C6E8B1D3F

#include "ITemplate.hpp"

#include <string>
#include <string_view>

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Main structure template for strong type generation
 *
 * This template generates the primary class definition for a strong type,
 * including:
 * - Namespace declarations (opening/closing)
 * - Class structure (struct/class keyword, name)
 * - Member variable declaration
 * - Constructor declarations
 * - Cast operators
 * - Placeholder sections for feature-specific operators
 *
 * Design philosophy:
 * The main template provides the skeleton - the overall structure that every
 * strong type must have. It defines the class boundary and includes Mustache
 * placeholders ({{>operator_name}}) for specific functionality that will be
 * filled in by other templates (ArithmeticTemplate, ComparisonTemplate, etc.).
 *
 * Template features:
 * - Always applies to every strong type (this is the foundation)
 * - Uses Mustache partials for composition of operator implementations
 * - Provides compile-time configuration through constexpr/template parameters
 *
 * Modern idioms:
 * - Generates constraint validation using C++20 concepts when applicable
 * - Supports both C++11 SFINAE and modern concepts
 * - Uses designated initializers in generated code when cpp_standard >= 20
 *
 * Example rendered output:
 * @code
 * namespace myapp {
 *
 * struct UserId : private atlas::strong_type_tag {
 *     int value;
 * public:
 *     using atlas_value_type = int;
 *     constexpr explicit UserId() = default;
 *     template<typename... ArgTs, ...>
 *     constexpr explicit UserId(ArgTs&&... args) : value(std::forward...) {}
 *     // ... cast operators, operator implementations via partials ...
 * };
 *
 * } // namespace myapp
 * @endcode
 */
class MainTemplate
: public ITemplate
{
protected:
    /**
     * Unique identifier for this template
     *
     * Uses hierarchical naming: "core.main_structure"
     * - core: fundamental template (not feature-specific)
     * - main_structure: the primary class skeleton
     *
     * @return String identifier (noexcept: returns string literal)
     */
    std::string id_impl() const noexcept override;

    /**
     * Sort key for main template
     *
     * @return Sort key: "" to ensure the main template sorts before others
     */
    std::string sort_key_impl() const noexcept override { return ""; }

    /**
     * Get the Mustache template string
     *
     * Returns the main structure template that includes:
     * - Namespace open/close sections
     * - Class declaration with inheritance from atlas::strong_type_tag
     * - Member variable (with optional default initialization)
     * - Type aliases (atlas_value_type, constraint types)
     * - Constructors (default, variadic forwarding)
     * - Cast operators (explicit to underlying type)
     * - Mustache partials for all operator categories
     *
     * The template uses Mustache sections ({{#flag}}...{{/flag}}) for
     * conditional generation and partials ({{>template_name}}) for operator
     * composition.
     *
     * @return String view to static template data (noexcept: accesses static)
     */
    std::string_view get_template_impl() const noexcept override;

    /**
     * Determine if this template should apply
     *
     * The main template ALWAYS applies - every strong type needs the basic
     * class structure. This is the foundation upon which other templates build.
     *
     * @param info Strong type description (unused - always returns true)
     * @return true (unconditionally, noexcept: trivial logic)
     */
    bool should_apply_impl(ClassInfo const & info) const noexcept override;

    /**
     * Prepare variables for Mustache rendering
     *
     * Converts StrongTypeDescription to ClassInfo, then serializes to JSON.
     *
     * The ClassInfo structure contains all metadata needed for rendering:
     * - Namespace and naming information
     * - Type information (underlying type, full qualified names)
     * - Feature flags (which operators, specializations to generate)
     * - Collections (operators, constants, forwarded member functions)
     * - Original description metadata
     *
     * Process:
     * 1. Parse StrongTypeDescription to extract type, operators, features
     * 2. Build ClassInfo with all metadata populated
     * 3. Convert to JSON via ClassInfo::to_json() for Mustache
     *
     * NOTE: This member function is NOT noexcept - parse() may throw if the
     * StrongTypeDescription contains invalid data.
     *
     * @param info Strong type description from user
     * @return JSON object with all variables for template rendering
     */
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    /**
     * Get required header includes
     *
     * The main template doesn't require any additional headers beyond what's
     * already in the preamble. Specific feature templates (hash, format, etc.)
     * will add their own includes.
     *
     * @return Empty set (noexcept: returns empty container)
     */
    std::set<std::string> required_includes_impl() const noexcept override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_9B2E4D6F8A1C5E3B7D9A2F4C6E8B1D3F
