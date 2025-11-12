// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryLogicalOperatorHelpers.hpp"
#include "LogicalOrOperator.hpp"

#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

using namespace logical_helpers;

std::string_view
LogicalOrOperator::
get_template_impl() const noexcept
{
    return get_binary_logical_operator_template();
}

bool
LogicalOrOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_logical_operator(info, "or");
}

boost::json::object
LogicalOrOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    return prepare_binary_logical_operator_variables(info, "or");
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<LogicalOrOperator> logical_or_operator_registrar;
}

} // namespace wjh::atlas::generation
