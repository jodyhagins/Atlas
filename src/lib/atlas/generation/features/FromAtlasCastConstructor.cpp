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
     * @brief Construct from another atlas type via atlas::cast drilling
     *
     * Accepts any atlas type whose underlying value chain reaches a type
     * convertible to the underlying type of this strong type.
     *
     * SFINAE rejects:
     * - Non-atlas types (handled by the variadic constructor)
     * - Same type (copy/move constructors handle that)
     * - Types directly constructible to underlying (variadic ctor handles that)
     * - Types not reachable via atlas::cast
     */
    template <
        typename ArgT,
        typename std::enable_if<
            atlas::atlas_detail::has_atlas_value_type<
                atlas::atlas_detail::remove_cvref_t<ArgT>>::value &&
            !std::is_same<
                atlas::atlas_detail::remove_cvref_t<ArgT>, {{{class_name}}}>::value &&
            !std::is_constructible<{{{underlying_type}}}, ArgT &&>::value &&
            atlas::atlas_detail::is_atlas_castable<ArgT, {{{underlying_type}}}>::value,
            bool>::type = true>
    {{{const_expr}}}explicit {{{class_name}}}(ArgT && arg)
    : {{{value}}}(atlas::cast<
        atlas::atlas_detail::forward_like_t<ArgT &&, {{{underlying_type}}}>>(
            std::forward<ArgT>(arg)))
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

// Self-registration with the template registry
namespace {
TemplateRegistrar<FromAtlasCastConstructor>
    from_atlas_cast_constructor_registrar;
}

} // namespace wjh::atlas::generation
