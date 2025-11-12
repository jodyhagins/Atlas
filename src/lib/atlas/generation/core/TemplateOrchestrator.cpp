// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ClassInfo.hpp"
#include "ITemplate.hpp"
#include "TemplateOrchestrator.hpp"
#include "TemplateRegistry.hpp"

#include <boost/mustache.hpp>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>

namespace wjh::atlas::generation {

namespace {

/**
 * Create a Mustache-friendly partial name from template ID
 *
 * Template IDs use dots (e.g., "operators.arithmetic.addition.default")
 * Mustache partials in the templates use underscores (e.g.,
 * "arithmetic_binary_operators") or simple names (e.g., "arrow_operator").
 *
 * This function maps template IDs to the partial names expected by the main
 * template.
 */
std::string
to_partial_name(std::string_view template_id)
{
    // Map template IDs to partial names used in main template
    static std::map<std::string_view, std::string> const id_to_partial{
        // Arithmetic operators - these are special, rendered per-operator
        {"operators.arithmetic.addition.default",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.addition.checked",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.addition.saturating",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.addition.wrapping",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.subtraction.default",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.subtraction.checked",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.subtraction.saturating",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.subtraction.wrapping",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.multiplication.default",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.multiplication.checked",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.multiplication.saturating",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.multiplication.wrapping",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.division.default",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.division.checked",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.division.saturating",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.division.wrapping",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.modulo.default", "arithmetic_binary_operators"},
        {"operators.arithmetic.modulo.checked", "arithmetic_binary_operators"},
        {"operators.arithmetic.modulo.saturating",
         "arithmetic_binary_operators"},
        {"operators.arithmetic.modulo.wrapping", "arithmetic_binary_operators"},
        {"operators.arithmetic.unary", "unary_operators"},
        {"operators.arithmetic.increment", "increment_operator"},

        // Bitwise operators
        {"operators.bitwise.and.default", "arithmetic_binary_operators"},
        {"operators.bitwise.or.default", "arithmetic_binary_operators"},
        {"operators.bitwise.xor.default", "arithmetic_binary_operators"},
        {"operators.bitwise.left_shift.default", "arithmetic_binary_operators"},
        {"operators.bitwise.right_shift.default",
         "arithmetic_binary_operators"},

        // Comparison operators
        {"operators.comparison.relational", "relational_operator"},
        {"operators.comparison.spaceship", "spaceship_operator"},
        {"operators.comparison.defaulted_equality",
         "defaulted_equality_operator"},

        // Access operators
        {"operators.access.arrow", "arrow_operator"},
        {"operators.access.indirection", "indirection_operator"},
        {"operators.functional.addressof", "addressof_operators"},

        // Logical operators
        {"operators.logical.not", "logical_not_operator"},
        {"operators.logical.and", "logical_operator"},
        {"operators.logical.or", "logical_operator"},

        // I/O operators
        {"operators.io.ostream", "ostream_operator"},
        {"operators.io.istream", "istream_operator"},

        // Functional operators
        {"operators.functional.nullary", "nullary"},
        {"operators.functional.callable", "callable"},
        {"operators.functional.subscript", "subscript_operator"},

        // Conversion operators
        {"operators.conversion.bool", "bool_operator"},
        {"operators.conversion.explicit", "explicit_cast_operator"},
        {"operators.conversion.implicit", "implicit_cast_operator"},

        // Specializations
        {"specializations.hash", "hash_specialization"},
        {"specializations.formatter", "formatter_specialization"},

        // Features
        {"features.constant_declarations", "constant_declarations"},
        {"features.constant_definitions", "constants"},
        {"features.forwarded_memfn", "forwarded_memfn"},
        {"features.iterator_support", "iterator_support_member"},
        {"features.template_assignment", "template_assignment_operator"},
    };

    auto it = id_to_partial.find(template_id);
    if (it != id_to_partial.end()) {
        return it->second;
    }

    // Default: use the template ID as-is (shouldn't happen with proper
    // mappings)
    return std::string(template_id);
}

/**
 * Check if a template is an arithmetic binary operator template
 *
 * @param template_id The template identifier
 * @return true if this is an arithmetic binary operator template
 */
bool
is_arithmetic_binary_operator_template(std::string_view template_id)
{
    return to_partial_name(template_id) == "arithmetic_binary_operators";
}

/**
 * Build template ID for an arithmetic operator
 *
 * Maps operator symbol and mode to the corresponding template ID.
 * For example: ("+", Checked) -> "operators.arithmetic.addition.checked"
 *
 * @param op Operator symbol ("+", "-", "*", "/", "%")
 * @param mode Arithmetic mode (Default, Checked, Saturating, Wrapping)
 * @return Template ID string
 */
std::string
arithmetic_operator_template_id(std::string_view op, ArithmeticMode mode)
{
    // Map operator symbol to category and name component
    std::string_view category;
    std::string_view op_name;

    if (op == "+") {
        category = "arithmetic";
        op_name = "addition";
    } else if (op == "-") {
        category = "arithmetic";
        op_name = "subtraction";
    } else if (op == "*") {
        category = "arithmetic";
        op_name = "multiplication";
    } else if (op == "/") {
        category = "arithmetic";
        op_name = "division";
    } else if (op == "%") {
        category = "arithmetic";
        op_name = "modulo";
    } else if (op == "&") {
        category = "bitwise";
        op_name = "and";
    } else if (op == "|") {
        category = "bitwise";
        op_name = "or";
    } else if (op == "^") {
        category = "bitwise";
        op_name = "xor";
    } else if (op == "<<") {
        category = "bitwise";
        op_name = "left_shift";
    } else if (op == ">>") {
        category = "bitwise";
        op_name = "right_shift";
    } else {
        // Unknown operator - should not happen
        return "";
    }

    // Map mode to template name component
    // Bitwise operators only support default mode
    std::string_view mode_name;
    if (category == "bitwise") {
        mode_name = "default";
    } else {
        switch (mode) {
        case ArithmeticMode::Default:
            mode_name = "default";
            break;
        case ArithmeticMode::Checked:
            mode_name = "checked";
            break;
        case ArithmeticMode::Saturating:
            mode_name = "saturating";
            break;
        case ArithmeticMode::Wrapping:
            // Wrapping doesn't make sense for division/modulo - use default
            // instead
            if (op == "/" || op == "%") {
                mode_name = "default";
            } else {
                mode_name = "wrapping";
            }
            break;
        }
    }

    // Build template ID: "operators.<category>.<op_name>.<mode_name>"
    return std::string("operators.") + std::string(category) + "." +
        std::string(op_name) + "." + std::string(mode_name);
}

} // anonymous namespace

std::set<std::string>
TemplateOrchestrator::
collect_includes(ClassInfo const & info)
{
    std::set<std::string> includes;

    auto & registry = TemplateRegistry::instance();
    registry.visit_applicable(info, [&](ITemplate const & tmpl) {
        auto tmpl_includes = tmpl.required_includes();
        includes.insert(tmpl_includes.begin(), tmpl_includes.end());
    });

    return includes;
}

std::set<std::string>
TemplateOrchestrator::
collect_preamble(ClassInfo const & info)
{
    std::set<std::string> preamble_components;

    auto & registry = TemplateRegistry::instance();
    registry.visit_applicable(info, [&](ITemplate const & tmpl) {
        auto tmpl_preamble = tmpl.required_preamble();
        preamble_components.insert(tmpl_preamble.begin(), tmpl_preamble.end());
    });

    return preamble_components;
}

std::map<std::string, std::string>
TemplateOrchestrator::
build_partials(ClassInfo const & info)
{
    std::map<std::string, std::string> partials;

    auto & registry = TemplateRegistry::instance();

    // For arithmetic binary operators, we need to render them in the order
    // they appear in info.arithmetic_binary_operators (which is sorted).
    // This ensures operators appear in the correct order: +, -, *, /, %
    std::ostringstream arithmetic_stream;

    // Render arithmetic operators in sorted order from ClassInfo
    for (auto const & op : info.arithmetic_binary_operators) {
        // Build the template ID for this operator and mode
        std::string template_id = arithmetic_operator_template_id(
            op.op,
            op.mode);

        if (template_id.empty()) {
            // Unknown operator - skip
            continue;
        }

        // Get the template from registry
        auto const * tmpl = registry.get_template(template_id);
        if (tmpl && tmpl->should_apply(info)) {
            try {
                std::string rendered = tmpl->render(info);
                arithmetic_stream << rendered;
            } catch (std::exception const & e) {
                // Collect error as warning
                add_warning(
                    std::string("Template rendering error in ") + template_id +
                        ": " + e.what(),
                    info.desc.type_name);
            }
        }
    }

    // Store accumulated arithmetic operators
    std::string arithmetic_code = arithmetic_stream.str();
    if (not arithmetic_code.empty()) {
        partials["arithmetic_binary_operators"] = std::move(arithmetic_code);
    }

    // For logical binary operators, we need to render them in the order
    // they appear in info.logical_operators (which is sorted).
    std::ostringstream logical_stream;

    // Render logical operators in sorted order from ClassInfo
    for (auto const & op : info.logical_operators) {
        // Map operator to template ID
        std::string template_id;
        if (op.op == "and") {
            template_id = "operators.logical.and";
        } else if (op.op == "or") {
            template_id = "operators.logical.or";
        } else {
            // Unknown operator - skip
            continue;
        }

        // Get the template from registry
        auto const * tmpl = registry.get_template(template_id);
        if (tmpl && tmpl->should_apply(info)) {
            try {
                std::string rendered = tmpl->render(info);
                logical_stream << rendered;
            } catch (std::exception const & e) {
                // Collect error as warning
                add_warning(
                    std::string("Template rendering error in ") + template_id +
                        ": " + e.what(),
                    info.desc.type_name);
            }
        }
    }

    // Store accumulated logical operators
    std::string logical_code = logical_stream.str();
    if (not logical_code.empty()) {
        partials["logical_operator"] = std::move(logical_code);
    }

    // Visit all other (non-arithmetic-binary-operator,
    // non-logical-binary-operator) templates
    registry.visit_applicable(info, [&](ITemplate const & tmpl) {
        // Skip arithmetic binary operator templates - already handled above
        if (is_arithmetic_binary_operator_template(tmpl.id())) {
            return;
        }

        // Skip logical binary operator templates - already handled above
        if (tmpl.id() == "operators.logical.and" ||
            tmpl.id() == "operators.logical.or")
        {
            return;
        }

        // Special handling for constant templates:
        // Store the template string (not rendered) because MainTemplate will
        // iterate over the constants array and render the partial multiple
        // times
        if (tmpl.id() == "features.constant_declarations" ||
            tmpl.id() == "features.constant_definitions")
        {
            std::string partial_name = to_partial_name(tmpl.id());
            std::string template_str = std::string(tmpl.get_template());
            partials[partial_name] = std::move(template_str);
            return;
        }

        try {
            std::string rendered = tmpl.render(info);
            std::string partial_name = to_partial_name(tmpl.id());

            // Other templates: store directly (last one wins if multiple)
            partials[partial_name] = std::move(rendered);
        } catch (std::exception const & e) {
            // Collect error as warning
            add_warning(
                std::string("Template rendering error in ") + tmpl.id() + ": " +
                    e.what(),
                info.desc.type_name);
        }
    });

    return partials;
}

void
TemplateOrchestrator::
add_warning(std::string message, std::string type_name)
{
    warnings_.push_back(Warning{std::move(message), std::move(type_name)});
}

std::string
TemplateOrchestrator::
render(ClassInfo const & info)
{
    // Clear warnings from previous render
    warnings_.clear();

    // Get the main template from registry
    auto & registry = TemplateRegistry::instance();
    auto * main_template = registry.get_template("core.main_structure");

    if (not main_template) {
        throw std::runtime_error(
            "Main template 'core.main_structure' not found in registry");
    }

    // For checked, saturating, and wrapping arithmetic modes, we need special
    // handling: render arithmetic operators separately and insert them at the
    // first friend function location (which will be inside the spaceship #if
    // block if present)
    bool const needs_arithmetic_insertion =
        (info.arithmetic_mode == ArithmeticMode::Checked ||
         info.arithmetic_mode == ArithmeticMode::Saturating ||
         info.arithmetic_mode == ArithmeticMode::Wrapping) &&
        not info.arithmetic_binary_operators.empty();

    if (needs_arithmetic_insertion) {
        // Build partials map first to get the arithmetic operators code
        auto partials_map = build_partials(info);
        std::string arithmetic_code =
            partials_map["arithmetic_binary_operators"];

        // Create modified ClassInfo without arithmetic operators for main
        // template
        ClassInfo info_modified = info;
        info_modified.arithmetic_binary_operators.clear();

        // Build partials again with modified info (without arithmetic)
        auto partials_map_modified = build_partials(info_modified);

        // Convert std::map to boost::json::object for Boost.Mustache
        boost::json::object partials;
        for (auto const & [key, value] : partials_map_modified) {
            partials[key] = value;
        }

        // Render the main template without arithmetic operators section
        std::ostringstream output;
        try {
            auto variables = main_template->prepare_variables(info_modified);
            auto template_str = std::string(main_template->get_template());

            boost::mustache::render(template_str, output, variables, partials);

        } catch (std::exception const & e) {
            add_warning(
                std::string("Main template rendering error: ") + e.what(),
                info.desc.type_name);
            throw;
        }

        // Insert arithmetic operators code into the class body
        std::string result = output.str();

        // Find insertion point: look for the first friend function
        // Start search after atlas_bounds if present
        size_t search_start = 0;
        if (info.is_bounded) {
            // Look for "using atlas_constraint" which comes right after
            // atlas_bounds
            size_t constraint_decl = result.find("using atlas_constraint =");
            if (constraint_decl != std::string::npos) {
                // Skip to the end of this line
                size_t line_end = result.find('\n', constraint_decl);
                if (line_end != std::string::npos) {
                    search_start = line_end + 1;
                }
            }
        }

        // Find the first friend function
        size_t insert_pos = result.find("    friend", search_start);

        if (insert_pos == std::string::npos) {
            // No friend functions yet, insert before closing brace
            insert_pos = result.find("};\n", search_start);
        }

        if (insert_pos != std::string::npos) {
            result.insert(insert_pos, arithmetic_code);
        }

        return result;
    }

    // Standard rendering for default arithmetic mode or no arithmetic operators
    auto partials = build_partials(info);

    // Render the main template with all partials
    std::ostringstream output;
    try {
        auto variables = main_template->prepare_variables(info);
        auto template_str = std::string(main_template->get_template());

        boost::mustache::render(template_str, output, variables, partials);

    } catch (std::exception const & e) {
        add_warning(
            std::string("Main template rendering error: ") + e.what(),
            info.desc.type_name);
        throw;
    }

    return output.str();
}

} // namespace wjh::atlas::generation
