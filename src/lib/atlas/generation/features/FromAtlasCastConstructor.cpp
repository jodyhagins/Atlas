// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "FromAtlasCastConstructor.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

std::string_view
FromAtlasCastConstructor::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * @brief Construct from a listed atlas type via atlas::cast drilling
     *
     * Accepts an argument only when its type is, or drills to, one of the
     * types named in this type's from_atlas_cast allowlist.  The cast always
     * produces a value: lvalue arguments drill by reference and copy at the
     * boundary layer, rvalue arguments move layer by layer, so the
     * constructed value never holds a reference into a temporary.
     *
     * SFINAE rejects:
     * - Non-atlas types (handled by the variadic constructor)
     * - Same type (copy/move constructors handle that)
     * - Types directly constructible to underlying (variadic ctor handles that)
     * - Types that are not, and do not drill to, a listed type
     * - Constructions the lifetime guards forbid (storing a borrow of an
     *   expiring owner)
     */
    template <
        typename ArgT,
        typename std::enable_if<
            atlas::atlas_detail::has_atlas_value_type<
                atlas::atlas_detail::remove_cvref_t<ArgT>>::value &&
            !std::is_same<
                atlas::atlas_detail::remove_cvref_t<ArgT>, {{{class_name}}}>::value &&
            !std::is_constructible<{{{underlying_type}}}, ArgT &&>::value &&
            ({{{allowlist_constraint}}}) &&
            atlas::atlas_detail::is_from_cast_constructible<
                ArgT &&, {{{underlying_type}}}>::value,
            bool>::type = true>
    {{{const_expr}}}explicit {{{class_name}}}(ArgT && arg)
    : {{{value}}}(atlas::cast<{{{underlying_type}}}>(std::forward<ArgT>(arg)))
    { }
)";
    return tmpl;
}

bool
FromAtlasCastConstructor::
should_apply_impl(ClassInfo const & info) const
{
    return info.from_atlas_cast_constructor;
}

boost::json::object
FromAtlasCastConstructor::
prepare_variables_impl(ClassInfo const & info) const
{
    // The allowlist constraint is a disjunction of drill-to probes, one per
    // named type, emitted as written so a type that is incomplete at the
    // point of the constraint is accepted once complete at the point of use.
    std::string constraint;
    for (auto const & named : info.desc.from_atlas_cast_types) {
        if (not constraint.empty()) {
            constraint += " ||\n             ";
        }
        constraint += "atlas::atlas_detail::is_atlas_castable<ArgT, " + named +
            ">::value";
    }

    boost::json::object vars;
    vars["allowlist_constraint"] = constraint;
    return vars;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<FromAtlasCastConstructor>
    from_atlas_cast_constructor_registrar;
}

} // namespace wjh::atlas::generation
