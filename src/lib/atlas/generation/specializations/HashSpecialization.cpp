#include "HashSpecialization.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

std::string
HashSpecialization::
id_impl() const
{
    return "specializations.hash";
}

std::string_view
HashSpecialization::
get_template_impl() const
{
    static constexpr std::string_view tmpl = R"(
/**
 * @brief std::hash specialization for {{{full_qualified_name}}}
 *
 * Drills down to find the first hashable type in the wrapping chain.
 * Falls back to underlying type for enums without std::hash.
 */
template <>
struct std::hash<{{{full_qualified_name}}}>
{
    ATLAS_NODISCARD
    auto operator()({{{full_qualified_name}}} const & t) const
    noexcept(noexcept(atlas::atlas_detail::hash_drill(
        atlas_value_for(t), atlas::atlas_detail::PriorityTag<2>{})))
    -> decltype(atlas::atlas_detail::hash_drill(
        atlas_value_for(t), atlas::atlas_detail::PriorityTag<2>{}))
    {
        return atlas::atlas_detail::hash_drill(
            atlas_value_for(t), atlas::atlas_detail::PriorityTag<2>{});
    }
};
)";
    return tmpl;
}

bool
HashSpecialization::
should_apply_impl(ClassInfo const & info) const
{
    return info.hash_specialization;
}

boost::json::object
HashSpecialization::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["full_qualified_name"] = info.full_qualified_name;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<HashSpecialization> hash_specialization_registrar;
}

} // namespace wjh::atlas::generation
