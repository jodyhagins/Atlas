// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_5D8F3A7E9C2B4F6D1A8E5C7B9F3D2A6E
#define WJH_ATLAS_5D8F3A7E9C2B4F6D1A8E5C7B9F3D2A6E

#include "ITemplate.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Singleton registry for template self-registration
 *
 * The TemplateRegistry maintains a collection of all available code generation
 * templates and provides mechanisms to visit templates that apply to a given
 * strong type.
 *
 * Design features:
 * - Singleton pattern ensures a single global registry
 * - Templates self-register during static initialization
 * - Type-safe visitor pattern using C++20 concepts
 * - Ordered map ensures deterministic iteration
 *
 * Thread Safety:
 * - The singleton instance() method is thread-safe (C++11 magic statics)
 * - Template registration occurs during static initialization, which is
 *   single-threaded by design in C++. All TemplateRegistrar instances
 *   complete their registration before main() starts.
 * - After initialization, the registry is read-only in typical usage,
 *   making it safe to access from multiple threads.
 * - The clear() method is NOT thread-safe and should only be used in
 *   single-threaded test scenarios.
 *
 * Example usage:
 * @code
 * // Registration (typically done via TemplateRegistrar)
 * TemplateRegistry::instance().register_template(
 *     std::make_unique<MyTemplate>()
 * );
 *
 * // Visiting applicable templates
 * StrongTypeDescription info = ...;
 * TemplateRegistry::instance().visit_applicable(info, [](ITemplate const& tmpl)
 * { std::string code = tmpl.render(info);
 *     // ... process generated code
 * });
 * @endcode
 */
class TemplateRegistry
{
public:
    /**
     * Get the singleton instance
     *
     * Thread-safe since C++11 guarantees static local initialization is
     * thread-safe.
     *
     * @return Reference to the singleton registry
     */
    [[nodiscard]]
    static TemplateRegistry & instance();

    /**
     * Register a template with the registry
     *
     * Templates are stored by their id() and must have unique identifiers.
     * Typically called during static initialization via TemplateRegistrar.
     *
     * @param tmpl Unique pointer to template (ownership transferred to
     * registry)
     * @throws std::runtime_error if a template with this ID already exists
     */
    void register_template(std::unique_ptr<ITemplate> tmpl);

    /**
     * Visit all templates applicable to the given class
     *
     * Iterates through all registered templates, checks if they should apply
     * to the given strong type, and invokes the visitor for each applicable
     * template.
     *
     * The visitor can be any callable (lambda, function, function object) that
     * accepts an ITemplate const& parameter and returns void.
     *
     * Templates are visited in deterministic order sorted by sort_key().
     * This ensures operators are generated in the correct order (by operator
     * symbol) rather than alphabetically by template ID.
     *
     * @tparam Visitor Callable type matching the TemplateVisitor concept
     * @param info Strong type class information (ClassInfo)
     * @param visitor Callable invoked for each applicable template
     *
     * Example:
     * @code
     * registry.visit_applicable(info, [&](ITemplate const& tmpl) {
     *     collected_includes.merge(tmpl.required_includes());
     *     generated_code += tmpl.render(info);
     * });
     * @endcode
     */
    template <typename Visitor>
    requires std::invocable<Visitor, ITemplate const &> &&
        std::same_as<std::invoke_result_t<Visitor, ITemplate const &>, void>
    void visit_applicable(ClassInfo const & info, Visitor && visitor) const
    {
        // Collect applicable templates into a vector
        std::vector<ITemplate const *> applicable_templates;
        applicable_templates.reserve(templates_.size());

        for (auto const & [id, tmpl] : templates_) {
            if (tmpl->should_apply(info)) {
                applicable_templates.push_back(tmpl.get());
            }
        }

        // Sort by sort_key() for deterministic operator ordering
        std::ranges::sort(
            applicable_templates,
            [](ITemplate const * a, ITemplate const * b) {
                return a->sort_key() < b->sort_key();
            });

        // Visit in sorted order
        for (auto const * tmpl : applicable_templates) {
            std::invoke(std::forward<Visitor>(visitor), *tmpl);
        }
    }

    /**
     * Get a specific template by ID
     *
     * @param id Template identifier
     * @return Pointer to template, or nullptr if not found
     */
    [[nodiscard]]
    ITemplate const * get_template(std::string_view id) const;

    /**
     * Check if a template with the given ID exists
     *
     * @param id Template identifier
     * @return true if template is registered
     */
    [[nodiscard]]
    bool has_template(std::string_view id) const;

    /**
     * Get count of registered templates
     *
     * @return Number of templates in registry
     */
    [[nodiscard]]
    std::size_t size() const
    {
        return templates_.size();
    }

    /**
     * Clear all registered templates
     *
     * Primarily useful for testing. Use with caution as templates cannot
     * re-register themselves after static initialization.
     */
    void clear();

private:
    // Private constructor for singleton pattern
    TemplateRegistry() = default;

    // Non-copyable, non-movable
    TemplateRegistry(TemplateRegistry const &) = delete;
    TemplateRegistry & operator = (TemplateRegistry const &) = delete;
    TemplateRegistry(TemplateRegistry &&) noexcept = delete;
    TemplateRegistry & operator = (TemplateRegistry &&) noexcept = delete;

    // Map of template ID to template instance
    // Using std::map with transparent comparator for heterogeneous lookup
    // This allows string_view lookups without creating temporary strings
    std::map<std::string, std::unique_ptr<ITemplate>, std::less<>> templates_;
};

/**
 * Helper template for self-registration pattern
 *
 * This RAII helper automatically registers a template during static
 * initialization. Each template type should have a static instance of
 * TemplateRegistrar in its implementation file.
 *
 * The template parameter must be a concrete type derived from ITemplate
 * with a default constructor.
 *
 * Example usage:
 * @code
 * // In ArithmeticTemplate.cpp
 * namespace {
 *     TemplateRegistrar<ArithmeticAdditionTemplate> register_addition;
 *     TemplateRegistrar<ArithmeticSubtractionTemplate> register_subtraction;
 * }
 * @endcode
 *
 * @tparam T Template class derived from ITemplate
 */
template <typename T>
requires std::derived_from<T, ITemplate> && std::default_initializable<T>
class TemplateRegistrar
{
public:
    /**
     * Constructor - registers the template
     *
     * Creates an instance of T and registers it with the TemplateRegistry.
     * Runs during static initialization before main().
     */
    TemplateRegistrar()
    {
        TemplateRegistry::instance().register_template(std::make_unique<T>());
    }

    // Non-copyable, non-movable (not needed for registration)
    TemplateRegistrar(TemplateRegistrar const &) = delete;
    TemplateRegistrar & operator = (TemplateRegistrar const &) = delete;
    TemplateRegistrar(TemplateRegistrar &&) noexcept = delete;
    TemplateRegistrar & operator = (TemplateRegistrar &&) noexcept = delete;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_5D8F3A7E9C2B4F6D1A8E5C7B9F3D2A6E
