// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AddressOfOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
AddressOfOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Access a pointer to the wrapped object.
     */
    {{{const_expr}}}{{{underlying_type}}} const * operator {{{op}}} () const
    noexcept
    {
        return std::addressof(value);
    }
    {{{const_expr}}}{{{underlying_type}}} * operator {{{op}}} ()
    noexcept
    {
        return std::addressof(value);
    }
)";
    return tmpl;
}

bool
AddressOfOperator::
should_apply_impl(ClassInfo const & info) const
{
    return not info.addressof_operators.empty();
}

boost::json::object
AddressOfOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    variables["underlying_type"] = info.underlying_type;

    // The operator is always "&" for address-of
    if (not info.addressof_operators.empty()) {
        variables["op"] = info.addressof_operators[0].op;
    }

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<AddressOfOperator> addressof_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
