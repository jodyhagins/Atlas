// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_3C7A9F2E1D5B4E6A8C1F3D7E9B2A5F4C
#define WJH_ATLAS_3C7A9F2E1D5B4E6A8C1F3D7E9B2A5F4C

#include <map>
#include <set>
#include <string>
#include <vector>

namespace wjh::atlas::generation {

// Forward declaration
struct ClassInfo;

} // namespace wjh::atlas::generation

namespace wjh::atlas::generation {

/**
 * Coordinates template rendering for strong type code generation
 *
 * The TemplateOrchestrator is responsible for:
 * - Discovering applicable templates via TemplateRegistry
 * - Collecting required includes and preamble components
 * - Rendering templates in the correct order
 * - Managing Mustache partials for operator composition
 * - Collecting and forwarding warnings from templates
 *
 * Design philosophy:
 * - Uses visitor pattern to query the registry for applicable templates
 * - Deduplicates includes and preamble components across templates
 * - Maintains deterministic ordering for reproducible output
 * - Separates concerns: templates know what to render, orchestrator knows when
 *
 * The orchestrator does NOT:
 * - Generate header guards (GuardGenerator's responsibility)
 * - Parse StrongTypeDescription (OperatorParser's responsibility)
 * - Know about specific operators (templates self-register and self-describe)
 *
 * Example usage:
 * @code
 * TemplateOrchestrator orchestrator;
 * ClassInfo info = ...;
 * std::string code = orchestrator.render(info);
 * auto warnings = orchestrator.get_warnings();
 * @endcode
 */
class TemplateOrchestrator
{
public:
    /**
     * Warning information from template rendering
     *
     * Templates may generate warnings during rendering (e.g., redundant
     * operator specifications, deprecated features). These are collected
     * and made available to the caller.
     */
    struct Warning
    {
        std::string message;
        std::string type_name;
    };

    /**
     * Render a complete strong type definition
     *
     * This is the main entry point that coordinates the entire rendering
     * pipeline:
     * 1. Visit all applicable templates via TemplateRegistry
     * 2. Collect includes and preamble components
     * 3. Build Mustache partials map for template composition
     * 4. Render main template with all partials
     * 5. Collect warnings from all templates
     *
     * The rendering process uses the visitor pattern:
     * - Each template decides if it applies (should_apply)
     * - Applicable templates are rendered to partials
     * - Partials are composed into the main template
     *
     * @param info Strong type class information
     * @return Complete C++ code for the strong type
     * @throws std::runtime_error if rendering fails
     */
    std::string render(ClassInfo const & info);

    /**
     * Get warnings collected during last render
     *
     * Templates may emit warnings about:
     * - Redundant operator specifications
     * - Deprecated features
     * - Conflicting options
     * - Performance implications
     *
     * Warnings are cleared at the start of each render() call.
     *
     * @return Vector of warnings from last render
     */
    std::vector<Warning> get_warnings() const { return warnings_; }

    /**
     * Clear accumulated warnings
     *
     * Primarily useful for testing. Warnings are automatically cleared
     * at the start of each render() call.
     */
    void clear_warnings() { warnings_.clear(); }

    /**
     * Collect all required includes from applicable templates
     *
     * Visits all templates that apply to the given description and
     * merges their required_includes() into a single deduplicated set.
     *
     * This is exposed publicly to allow StrongTypeGenerator to coordinate
     * the final header assembly (guard, includes, preamble, code).
     *
     * @param info Strong type class information
     * @return Set of unique include paths
     */
    std::set<std::string> collect_includes(ClassInfo const & info);

    /**
     * Collect all required preamble components from applicable templates
     *
     * Visits all templates that apply to the given description and
     * merges their required_preamble() into a single deduplicated set.
     *
     * Preamble components are identifiers for helper code that must
     * appear before the main class definition (e.g., type traits, helper
     * functions).
     *
     * This is exposed publicly to allow StrongTypeGenerator to coordinate
     * the final header assembly (guard, includes, preamble, code).
     *
     * @param info Strong type class information
     * @return Set of unique preamble component identifiers
     */
    std::set<std::string> collect_preamble(ClassInfo const & info);

private:
    /**
     * Build Mustache partials map for template composition
     *
     * Renders each applicable template and stores the result as a Mustache
     * partial. The main template can then include these partials using
     * {{>partial_name}} syntax.
     *
     * Partials are keyed by template ID (e.g.,
     * "operators.arithmetic.addition"). The main template uses these partials
     * to compose the final output.
     *
     * @param info Strong type class information
     * @return Map of partial name to rendered content
     */
    std::map<std::string, std::string> build_partials(ClassInfo const & info);

    /**
     * Add a warning to the warning list
     *
     * @param message Warning message
     * @param type_name Strong type name for context
     */
    void add_warning(std::string message, std::string type_name);

    // Accumulated warnings from template rendering
    std::vector<Warning> warnings_;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_3C7A9F2E1D5B4E6A8C1F3D7E9B2A5F4C
