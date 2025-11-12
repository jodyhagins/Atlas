// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "OStreamOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

std::string_view
OStreamOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Insert the wrapped object into an ostream.
     */
    friend std::ostream & operator << (
        std::ostream & strm,
        {{{class_name}}} const & t)
    {
        return strm << t.value;
    }
)";
    return tmpl;
}

bool
OStreamOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.ostream_operator;
}

boost::json::object
OStreamOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["class_name"] = info.class_name;
    variables["underlying_type"] = info.underlying_type;
    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<OStreamOperator> ostream_operator_registrar;
}

} // namespace wjh::atlas::generation
