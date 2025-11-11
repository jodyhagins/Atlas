// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ForwardedMemfnTemplate.hpp"

#include <boost/mustache.hpp>

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <sstream>

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
ForwardedMemfnTemplate::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"xxx(
    /**
     * @brief Forward {{memfn_name}} to wrapped object{{#alias_name}} (aliased as {{alias_name}}){{/alias_name}}{{#return_type}},
     * wrapping return value in {{return_type}}{{/return_type}}
     *
     * This member function forwards all calls to the underlying type's
     * {{memfn_name}} member function, preserving const-correctness,
     * noexcept specifications, and perfect forwarding.
{{#const_only}}     * Only const overloads are generated.
{{/const_only}}{{#return_type}}     * Return value is wrapped in {{return_type}} (requires {{return_type}} to be
     * constructible from the memfn's return type).
{{/return_type}}{{#has_constraint}}     *
     * IMPORTANT: Constraint checking occurs AFTER the operation executes.
     * This is an inherent limitation of generic constraint checking - we cannot
     * know ahead of time if an operation will violate a constraint without
     * operation-specific knowledge.
{{/has_constraint}}     */
{{^const_only}}#if defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L
    // C++23 deducing this - single elegant overload
    template <typename Self, typename... Args>
    {{const_expr}}auto {{#alias_name}}{{alias_name}}{{/alias_name}}{{^alias_name}}{{memfn_name}}{{/alias_name}}(this Self&& self, Args&&... args)
{{#return_type}}
    -> {{return_type}}
{{/return_type}}
{{^return_type}}
{{^has_constraint}}
    noexcept(noexcept(std::forward<Self>(self).value.{{memfn_name}}(std::forward<Args>(args)...)))
{{/has_constraint}}
    -> decltype(std::forward<Self>(self).value.{{memfn_name}}(std::forward<Args>(args)...))
{{/return_type}}
    {
        {{#has_constraint}}
        using atlas::constraints::constraint_guard;
        [[maybe_unused]] auto guard = constraint_guard<atlas_constraint>(
            self.value,
            "{{class_name}}::{{memfn_name}}");
        {{/has_constraint}}
        return {{#return_type}}{{return_type}}({{/return_type}}std::forward<Self>(self).value.{{memfn_name}}(std::forward<Args>(args)...){{#return_type}}){{/return_type}};
    }
#else
{{/const_only}}    // C++11-20: ref-qualified overloads (or just const for const-only)
{{#generate_const_no_ref}}
    template <typename... Args>
    {{const_expr}}auto {{#alias_name}}{{alias_name}}{{/alias_name}}{{^alias_name}}{{memfn_name}}{{/alias_name}}(Args&&... args) const
{{#return_type}}
    -> {{return_type}}
{{/return_type}}
{{^return_type}}
    noexcept(noexcept(value.{{memfn_name}}(std::forward<Args>(args)...)))
    -> decltype(value.{{memfn_name}}(std::forward<Args>(args)...))
{{/return_type}}
    {
        return {{#return_type}}{{return_type}}({{/return_type}}value.{{memfn_name}}(std::forward<Args>(args)...){{#return_type}}){{/return_type}};
    }
{{/generate_const_no_ref}}

{{#generate_const_lvalue}}
    template <typename... Args>
    {{const_expr}}auto {{#alias_name}}{{alias_name}}{{/alias_name}}{{^alias_name}}{{memfn_name}}{{/alias_name}}(Args&&... args) const &
{{#return_type}}
    -> {{return_type}}
{{/return_type}}
{{^return_type}}
    noexcept(noexcept(value.{{memfn_name}}(std::forward<Args>(args)...)))
    -> decltype(value.{{memfn_name}}(std::forward<Args>(args)...))
{{/return_type}}
    {
        return {{#return_type}}{{return_type}}({{/return_type}}value.{{memfn_name}}(std::forward<Args>(args)...){{#return_type}}){{/return_type}};
    }
{{/generate_const_lvalue}}

{{#generate_const_rvalue}}
    template <typename... Args>
    {{const_expr}}auto {{#alias_name}}{{alias_name}}{{/alias_name}}{{^alias_name}}{{memfn_name}}{{/alias_name}}(Args&&... args) const &&
{{#return_type}}
    -> {{return_type}}
{{/return_type}}
{{^return_type}}
    noexcept(noexcept(std::move(value).{{memfn_name}}(std::forward<Args>(args)...)))
    -> decltype(std::move(value).{{memfn_name}}(std::forward<Args>(args)...))
{{/return_type}}
    {
        return {{#return_type}}{{return_type}}({{/return_type}}std::move(value).{{memfn_name}}(std::forward<Args>(args)...){{#return_type}}){{/return_type}};
    }
{{/generate_const_rvalue}}

{{#generate_nonconst_lvalue}}
    template <typename... Args>
    {{const_expr}}auto {{#alias_name}}{{alias_name}}{{/alias_name}}{{^alias_name}}{{memfn_name}}{{/alias_name}}(Args&&... args) &
{{#return_type}}
    -> {{return_type}}
{{/return_type}}
{{^return_type}}
{{^has_constraint}}
    noexcept(noexcept(value.{{memfn_name}}(std::forward<Args>(args)...)))
{{/has_constraint}}
    -> decltype(value.{{memfn_name}}(std::forward<Args>(args)...))
{{/return_type}}
    {
        {{#has_constraint}}
        using atlas::constraints::constraint_guard;
        [[maybe_unused]] auto guard = constraint_guard<atlas_constraint>(
            value,
            "{{class_name}}::{{memfn_name}}");
        {{/has_constraint}}
        return {{#return_type}}{{return_type}}({{/return_type}}value.{{memfn_name}}(std::forward<Args>(args)...){{#return_type}}){{/return_type}};
    }
{{/generate_nonconst_lvalue}}

{{#generate_nonconst_rvalue}}
    template <typename... Args>
    {{const_expr}}auto {{#alias_name}}{{alias_name}}{{/alias_name}}{{^alias_name}}{{memfn_name}}{{/alias_name}}(Args&&... args) &&
{{#return_type}}
    -> {{return_type}}
{{/return_type}}
{{^return_type}}
{{^has_constraint}}
    noexcept(noexcept(std::move(value).{{memfn_name}}(std::forward<Args>(args)...)))
{{/has_constraint}}
    -> decltype(std::move(value).{{memfn_name}}(std::forward<Args>(args)...))
{{/return_type}}
    {
        {{#has_constraint}}
        using atlas::constraints::constraint_guard;
        [[maybe_unused]] auto guard = constraint_guard<atlas_constraint>(
            value,
            "{{class_name}}::{{memfn_name}}");
        {{/has_constraint}}
        return {{#return_type}}{{return_type}}({{/return_type}}std::move(value).{{memfn_name}}(std::forward<Args>(args)...){{#return_type}}){{/return_type}};
    }
{{/generate_nonconst_rvalue}}
{{^const_only}}#endif
{{/const_only}})xxx";
    return tmpl;
}

bool
ForwardedMemfnTemplate::
should_apply_impl(ClassInfo const & info) const
{
    return not info.forwarded_memfns.empty();
}

boost::json::object
ForwardedMemfnTemplate::
prepare_variables_for_function(
    ClassInfo const & info,
    ForwardedMemfn const & fwd) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    variables["class_name"] = info.class_name;

    // Check if type has constraints (used for constraint guard generation)
    bool const has_constraint = not info.constraint_type.empty();
    variables["has_constraint"] = has_constraint;

    // Add per-function variables from ForwardedMemfn
    variables["memfn_name"] = fwd.memfn_name;
    variables["alias_name"] = fwd.alias_name;
    variables["return_type"] = fwd.return_type;
    variables["const_only"] = fwd.const_only;
    variables["generate_const_no_ref"] = fwd.generate_const_no_ref;
    variables["generate_const_lvalue"] = fwd.generate_const_lvalue;
    variables["generate_const_rvalue"] = fwd.generate_const_rvalue;
    variables["generate_nonconst_lvalue"] = fwd.generate_nonconst_lvalue;
    variables["generate_nonconst_rvalue"] = fwd.generate_nonconst_rvalue;

    return variables;
}

std::string
ForwardedMemfnTemplate::
render_impl(ClassInfo const & info) const
{
    // Validate that this template should be applied
    validate(info);

    if (not should_apply(info)) {
        return "";
    }

    // Get the template string once
    std::string_view tmpl_str = get_template();
    if (tmpl_str.empty()) {
        return "";
    }

    // Iterate over all forwarded member functions and render each one
    std::ostringstream accumulated;
    for (auto const & fwd : info.forwarded_memfns) {
        // Prepare variables for this specific forwarded function
        boost::json::object variables = prepare_variables_for_function(
            info,
            fwd);

        // Render this forwarded function
        std::ostringstream oss;
        boost::mustache::render(
            tmpl_str,
            oss,
            variables,
            boost::json::object{});

        // Accumulate into result
        accumulated << oss.str();
    }

    return accumulated.str();
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<ForwardedMemfnTemplate> forwarded_memfn_template_registrar;
}

}} // namespace wjh::atlas::generation::v1
