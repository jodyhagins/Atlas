// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "TemplateAssignmentOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
TemplateAssignmentOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * @brief Template assignment operator
     *
     * Allows assignment from any type that is assignable to the underlying type.
     * This provides convenience while maintaining type safety through SFINAE.
     *
     * Example:
     *   StrongType s{"initial"};
     *   s = "new value";        // Works if assignable
     *   s = std::string("foo"); // Works if assignable
     *   s = 42;                 // Rejected if not assignable
     *
     * Note: constexpr is applied only in C++14 and later because in C++11,
     * constexpr non-static member functions are implicitly const.
     */
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
    template <typename T>
      requires (std::assignable_from<{{{underlying_type}}}&, T> &&
                not std::same_as<std::decay_t<T>, {{{class_name}}}>)
#else
    template <typename T,
        typename std::enable_if<
            std::is_assignable<{{{underlying_type}}}&, T>::value &&
            not std::is_same<typename std::decay<T>::type, {{{class_name}}}>::value,
            int>::type = 0>
#endif
#if __cplusplus >= 201402L
    {{{const_expr}}}{{{class_name}}}& operator=(T&& t)
#else
    {{{class_name}}}& operator=(T&& t)
#endif
    noexcept(noexcept(std::declval<{{{underlying_type}}}&>() = std::declval<T>()))
    {
        {{{value}}} = std::forward<T>(t);
        return *this;
    }
)";
    return tmpl;
}

bool
TemplateAssignmentOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.template_assignment_operator;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<TemplateAssignmentOperator>
    template_assignment_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
