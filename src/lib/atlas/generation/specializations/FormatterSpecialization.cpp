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
 * Drills down to find the first formattable type in the wrapping chain.
 * Falls back to underlying type for enums without std::formatter.
 */
#if defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
template <>
struct std::formatter<{{{full_qualified_name}}}>
{
private:
    using drilled_type_ = atlas::atlas_detail::format_drilled_type_t<{{{full_qualified_name}}}::atlas_value_type>;
    std::formatter<drilled_type_> underlying_formatter_;

public:
    constexpr auto parse(std::format_parse_context & ctx)
    {
        return underlying_formatter_.parse(ctx);
    }

    auto format({{{full_qualified_name}}} const & t, std::format_context & ctx) const
    {
        return underlying_formatter_.format(
            atlas::atlas_detail::format_value_drill(atlas_value_for(t)),
            ctx);
    }
};
#endif // defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
)";
    return tmpl;
}

bool
FormatterSpecialization::
should_apply_impl(ClassInfo const &) const
{
    // Per-type formatter specialization is disabled.
    // Use auto_format=true at file level for automatic std::formatter support.
    return false;
}

boost::json::object
FormatterSpecialization::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["full_qualified_name"] = info.full_qualified_name;
    variables["underlying_type"] =
        info.underlying_type; // Used to determine drilled type

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<FormatterSpecialization> formatter_specialization_registrar;
}

} // namespace wjh::atlas::generation
