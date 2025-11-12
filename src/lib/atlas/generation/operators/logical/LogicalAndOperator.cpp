// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryLogicalOperatorHelpers.hpp"
#include "LogicalAndOperator.hpp"

#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

using namespace logical_helpers;

std::string_view
LogicalAndOperator::
get_template_impl() const noexcept
{
    return get_binary_logical_operator_template();
}

bool
LogicalAndOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_logical_operator(info, "and");
}

boost::json::object
LogicalAndOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    return prepare_binary_logical_operator_variables(info, "and");
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<LogicalAndOperator> logical_and_operator_registrar;
}

} // namespace wjh::atlas::generation
