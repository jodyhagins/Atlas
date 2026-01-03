// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "IStreamOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

std::string_view
IStreamOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Extract the wrapped object from an istream.
     * Drills down to find the first istreamable type.
     */
    friend std::istream & operator>>(
        std::istream & strm,
        {{{class_name}}} & t)
    {
        atlas::atlas_detail::istream_drill(
            strm, t.value, atlas::atlas_detail::PriorityTag<2>{});
        return strm;
    }
)";
    return tmpl;
}

bool
IStreamOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.istream_operator;
}

boost::json::object
IStreamOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["class_name"] = info.class_name;
    variables["underlying_type"] = info.underlying_type;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<IStreamOperator> istream_operator_registrar;
}

} // namespace wjh::atlas::generation
