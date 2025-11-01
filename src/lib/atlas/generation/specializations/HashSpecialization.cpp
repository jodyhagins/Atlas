#include "HashSpecialization.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

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
 * Delegates to std::hash of the underlying type {{{underlying_type}}}
 */
template <>
struct std::hash<{{{full_qualified_name}}}>
{
    ATLAS_NODISCARD
    {{{hash_const_expr}}}std::size_t operator () ({{{full_qualified_name}}} const & t) const
    noexcept(
        noexcept(std::hash<{{{underlying_type}}}>{}(
            std::declval<{{{underlying_type}}} const &>())))
    {
        return std::hash<{{{underlying_type}}}>{}(
            static_cast<{{{underlying_type}}} const &>(t));
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
    variables["underlying_type"] = info.underlying_type;
    variables["hash_const_expr"] = info.hash_const_expr;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<HashSpecialization> hash_specialization_registrar;
}

}} // namespace wjh::atlas::generation::v1
