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
     * Drills down to find the first ostreamable type.
     */
    friend std::ostream & operator<<(
        std::ostream & strm,
        {{{class_name}}} const & t)
    {
        atlas::atlas_detail::ostream_drill(
            strm, t.value, atlas::atlas_detail::PriorityTag<2>{});
        return strm;
    }
)";
    return tmpl;
}

bool
OStreamOperator::
should_apply_impl(ClassInfo const &) const
{
    // Per-type ostream operator is disabled.
    // Use auto_ostream=true at file level for automatic operator<< support.
    return false;
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
