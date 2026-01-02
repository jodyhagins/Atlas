#include "FormatterSpecialization.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

std::string
FormatterSpecialization::
id_impl() const
{
    return "specializations.formatter";
}

std::string_view
FormatterSpecialization::
get_template_impl() const
{
    static constexpr std::string_view tmpl = R"(
/**
 * @brief std::formatter specialization for {{{full_qualified_name}}}
 *
 * Enables use with std::format and std::print in C++20 and later:
 *   std::format("{}", strong_type_instance)
 *
 * This specialization is only available when std::format is available
 * (checked via __cpp_lib_format >= 202110L). Delegates formatting to the
 * underlying type {{{underlying_type}}}
 */
#if defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
template <>
struct std::formatter<{{{full_qualified_name}}}> : std::formatter<{{{underlying_type}}}>
{
    auto format({{{full_qualified_name}}} const & t, std::format_context & ctx) const
    {
        return std::formatter<{{{underlying_type}}}>::format(atlas_value_for(t), ctx);
    }
};
#endif // defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
)";
    return tmpl;
}

bool
FormatterSpecialization::
should_apply_impl(ClassInfo const & info) const
{
    return info.formatter_specialization;
}

boost::json::object
FormatterSpecialization::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["full_qualified_name"] = info.full_qualified_name;
    variables["underlying_type"] = info.underlying_type;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<FormatterSpecialization> formatter_specialization_registrar;
}

} // namespace wjh::atlas::generation
