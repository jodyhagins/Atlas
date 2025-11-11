// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "IteratorSupportTemplate.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
IteratorSupportTemplate::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Iterator type aliases for container-like interface.
     */
    using iterator = decltype(atlas::atlas_detail::begin_(
        std::declval<{{{underlying_type}}}&>()));
    using const_iterator = decltype(atlas::atlas_detail::begin_(
        std::declval<{{{underlying_type}}} const&>()));
    using value_type = typename std::remove_reference<decltype(
        *atlas::atlas_detail::begin_(
            std::declval<{{{underlying_type}}}&>()))>::type;

    /**
     * Member functions for iterator access.
     * Enables both explicit calls (e.g., s.begin()) and range-based for loops.
     * Uses ADL-enabled helpers that work in decltype/noexcept contexts.
     */
    {{{const_expr}}}auto begin()
    noexcept(noexcept(atlas::atlas_detail::begin_(value)))
    -> decltype(atlas::atlas_detail::begin_(value))
    {
        return atlas::atlas_detail::begin_(value);
    }

    {{{const_expr}}}auto end()
    noexcept(noexcept(atlas::atlas_detail::end_(value)))
    -> decltype(atlas::atlas_detail::end_(value))
    {
        return atlas::atlas_detail::end_(value);
    }

    {{{const_expr}}}auto begin() const
    noexcept(noexcept(atlas::atlas_detail::begin_(value)))
    -> decltype(atlas::atlas_detail::begin_(value))
    {
        return atlas::atlas_detail::begin_(value);
    }

    {{{const_expr}}}auto end() const
    noexcept(noexcept(atlas::atlas_detail::end_(value)))
    -> decltype(atlas::atlas_detail::end_(value))
    {
        return atlas::atlas_detail::end_(value);
    }
)";
    return tmpl;
}

bool
IteratorSupportTemplate::
should_apply_impl(ClassInfo const & info) const
{
    return info.iterator_support_member;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<IteratorSupportTemplate> iterator_support_template_registrar;
}

}} // namespace wjh::atlas::generation::v1
