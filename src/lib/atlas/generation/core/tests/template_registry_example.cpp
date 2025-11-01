// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

/**
 * @file template_registry_example.cpp
 * Example demonstrating the template infrastructure usage
 *
 * This file shows how to create a concrete template class and register it
 * with the TemplateRegistry. It serves both as documentation and as a
 * compilation test for the infrastructure.
 */

#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <iostream>

namespace wjh::atlas::generation::examples {

/**
 * Example template that generates a simple getter method
 *
 * This demonstrates the minimal implementation needed for a concrete template.
 */
class SimpleGetterTemplate
: public ITemplate
{
protected:
    std::string id_impl() const override { return "examples.simple_getter"; }

    std::string_view get_template_impl() const override
    {
        static constexpr char const tmpl[] = R"(
    // Getter for the wrapped value
    constexpr auto const& get_value() const noexcept {
        return value;
    }
)";
        return tmpl;
    }

    bool should_apply_impl(
        wjh::atlas::StrongTypeDescription const & info) const override
    {
        // This example template always applies
        (void)info;
        return true;
    }

    boost::json::object prepare_variables_impl(
        wjh::atlas::StrongTypeDescription const & info) const override
    {
        // No variables needed for this simple template
        (void)info;
        return boost::json::object{};
    }

    std::set<std::string> required_includes_impl() const override
    {
        // No additional includes needed
        return {};
    }
};

/**
 * Example template that demonstrates variable substitution
 *
 * This shows how to use Mustache variables in templates.
 */
class TypeInfoTemplate
: public ITemplate
{
protected:
    std::string id_impl() const override { return "examples.type_info"; }

    std::string_view get_template_impl() const override
    {
        static constexpr char const tmpl[] = R"(
    // Type information
    // Strong type: {{{type_name}}}
    // Wraps: {{{wrapped_type}}}
    // Namespace: {{{type_namespace}}}
)";
        return tmpl;
    }

    bool should_apply_impl(
        wjh::atlas::StrongTypeDescription const & info) const override
    {
        // Only apply if we have a type name
        return not info.type_name.empty();
    }

    boost::json::object prepare_variables_impl(
        wjh::atlas::StrongTypeDescription const & info) const override
    {
        boost::json::object vars;
        vars["type_name"] = info.type_name;
        vars["type_namespace"] = info.type_namespace;

        // Extract wrapped type from description
        // This is simplified - real implementation would parse the description
        vars["wrapped_type"] = "int";

        return vars;
    }
};

// Example of self-registration using TemplateRegistrar
// In a real implementation, this would be in a separate .cpp file
namespace {
// These static instances register the templates during static initialization
// TemplateRegistrar<SimpleGetterTemplate> register_simple_getter;
// TemplateRegistrar<TypeInfoTemplate> register_type_info;
//
// Note: Commented out to avoid actual registration in this example file
}

} // namespace wjh::atlas::generation::examples

/**
 * Example usage of the template infrastructure
 *
 * This function demonstrates how to:
 * 1. Register templates manually (alternative to self-registration)
 * 2. Visit applicable templates
 * 3. Render templates for a given strong type
 */
void
example_usage()
{
    using namespace wjh::atlas;
    using namespace wjh::atlas::generation;

    // Create a sample strong type description
    StrongTypeDescription info;
    info.type_name = "UserId";
    info.type_namespace = "myapp";
    info.description = "strong int; ==, !=, hash";

    // Manually register templates (alternative to static TemplateRegistrar)
    auto & registry = TemplateRegistry::instance();
    registry.register_template(
        std::make_unique<examples::SimpleGetterTemplate>());
    registry.register_template(std::make_unique<examples::TypeInfoTemplate>());

    std::cout << "Registered templates: " << registry.size() << "\n\n";

    // Visit all applicable templates
    registry.visit_applicable(info, [&](ITemplate const & tmpl) {
        std::cout << "Template: " << tmpl.id() << "\n";
        std::cout << "Applies to " << info.type_name << ": "
            << (tmpl.should_apply(info) ? "yes" : "no") << "\n";

        // Render the template
        std::string code = tmpl.render(info);
        std::cout << "Generated code:\n" << code << "\n";
    });
}

/**
 * Main function for the example
 *
 * This is only compiled when building as a standalone example,
 * not when included in the main library.
 */
#ifdef BUILD_TEMPLATE_REGISTRY_EXAMPLE
int
main()
{
    try {
        example_usage();
        return 0;
    } catch (std::exception const & e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
#endif
