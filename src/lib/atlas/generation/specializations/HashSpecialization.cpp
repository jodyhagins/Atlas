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
    // Pre-C++20: one-liner inheriting from hash_base.
    // In C++20+ the concept-constrained partial specialization in the
    // preamble handles all types, so these are skipped.
    static constexpr std::string_view tmpl = R"(
#if __cplusplus < 202002L
template <>
struct std::hash<{{{full_qualified_name}}}>
: atlas::atlas_detail::hash_base<{{{full_qualified_name}}}>
{};
#endif
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
