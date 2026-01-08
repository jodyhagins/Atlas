// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"
#include "StrongTypeDescriptionBoostDescribe.hpp"
#include "StrongTypeGenerator.hpp"
#include "TypeTokenizer.hpp"

#include <boost/describe.hpp>
#include <boost/json/src.hpp>
#include <boost/mustache.hpp>
#include <boost/uuid/detail/sha1.hpp>

#include "atlas/version.hpp"
#include "generation/core/ClassInfo.hpp"
#include "generation/core/GuardGenerator.hpp"
#include "generation/core/TemplateOrchestrator.hpp"
#include "generation/parsing/OperatorParser.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

#include <iostream>

namespace wjh::atlas {

using namespace wjh::atlas::generation;

// Bring header types into scope
using wjh::atlas::generation::CastOperator;
using wjh::atlas::generation::ClassInfo;
using wjh::atlas::generation::Constant;
using wjh::atlas::generation::ForwardedMemfn;
using wjh::atlas::generation::Operator;

namespace {

// Print warnings to stderr with optional color
void
print_warnings(std::vector<StrongTypeGenerator::Warning> const & warnings)
{
    if (warnings.empty()) {
        return;
    }

    bool use_color = supports_color(fileno(stderr));

    std::cerr << "\n";
    if (use_color) {
        std::cerr << color::red << "Warnings:" << color::reset << "\n";
        for (auto const & w : warnings) {
            std::cerr << "  " << color::yellow << w.type_name << ": "
                << w.message << color::reset << "\n";
        }
    } else {
        std::cerr << "Warnings:\n";
        for (auto const & w : warnings) {
            std::cerr << "  " << w.type_name << ": " << w.message << "\n";
        }
    }
    std::cerr << std::endl;
}

std::string
render_code(ClassInfo const & info)
{
    TemplateOrchestrator orchestrator;
    return orchestrator.render(info);
}

} // anonymous namespace

std::string
StrongTypeGenerator::
operator () (StrongTypeDescription const & desc)
{
    auto const info = ClassInfo::parse(desc, &warnings_);
    auto const code = render_code(info);
    auto const guard = GuardGenerator::make_guard(desc, code);

    // Collect all includes: user includes + preamble includes
    PreambleOptions preamble_opts{
        .include_arrow_operator_traits = info.arrow_operator,
        .include_dereference_operator_traits = info.indirection_operator,
        .include_checked_helpers =
            (info.arithmetic_mode == ArithmeticMode::Checked),
        .include_saturating_helpers =
            (info.arithmetic_mode == ArithmeticMode::Saturating),
        .include_constraints = info.has_constraint,
        .include_nilable_support = info.nil_value_is_constant,
        .include_hash_drill = info.hash_specialization,
        .include_ostream_drill = info.ostream_operator,
        .include_istream_drill = info.istream_operator,
        .include_format_drill = info.formatter_specialization};

    auto preamble_includes = get_preamble_includes(preamble_opts);

    // Merge with user includes from info
    std::set<std::string> all_includes_set(
        info.includes_vec.begin(),
        info.includes_vec.end());
    all_includes_set.insert(preamble_includes.begin(), preamble_includes.end());

    // Remove <version> and <compare> as they're handled separately
    all_includes_set.erase("<version>");
    all_includes_set.erase("<compare>");

    // Build the includes string with guards
    std::ostringstream includes_stream;
    for (auto const & include : all_includes_set) {
        auto guard_it = info.include_guards.find(include);
        if (guard_it != info.include_guards.end()) {
            includes_stream << "#if " << guard_it->second << '\n';
            includes_stream << "#include " << include << '\n';
            includes_stream << "#endif // " << guard_it->second << '\n';
        } else {
            includes_stream << "#include " << include << '\n';
        }
    }

    std::stringstream strm;
    strm << "#ifndef " << guard << '\n'
        << "#define " << guard << "\n\n"
        << generate_cpp_standard_assertion(info.cpp_standard)
        << GuardGenerator::make_notice_banner() << '\n'
        << R"(#if __has_include(<version>)
#include <version>
#endif
)" << includes_stream.str()
        << '\n'
        << preamble(preamble_opts) << code << "#endif // " << guard << '\n';
    return strm.str();
}

std::string
generate_strong_types_file(
    std::vector<StrongTypeDescription> const & descriptions,
    std::string const & guard_prefix,
    std::string const & guard_separator,
    bool upcase_guard,
    PreambleOptions auto_opts)
{
    std::set<std::string> all_includes;
    std::map<std::string, std::string> all_guards;
    std::ostringstream combined_code;
    std::vector<StrongTypeGenerator::Warning> warnings;
    bool any_arrow_operator = false;
    bool any_indirection_operator = false;
    bool any_checked_arithmetic = false;
    bool any_saturating_arithmetic = false;
    bool any_constraints = false;
    bool any_nil_value = false;
    bool any_hash_specialization = false;
    bool any_ostream_operator = false;
    bool any_istream_operator = false;
    bool any_formatter_specialization = false;
    int max_cpp_standard = 11;

    // Generate each type WITHOUT preamble, and collect includes
    for (auto const & desc : descriptions) {
        auto info = ClassInfo::parse(desc, &warnings);

        if (info.cpp_standard > max_cpp_standard) {
            max_cpp_standard = info.cpp_standard;
        }

        if (info.arrow_operator) {
            any_arrow_operator = true;
        }

        if (info.indirection_operator) {
            any_indirection_operator = true;
        }

        if (info.arithmetic_mode == ArithmeticMode::Checked) {
            any_checked_arithmetic = true;
        }

        if (info.arithmetic_mode == ArithmeticMode::Saturating) {
            any_saturating_arithmetic = true;
        }

        if (info.has_constraint) {
            any_constraints = true;
        }

        if (info.nil_value_is_constant) {
            any_nil_value = true;
        }

        if (info.hash_specialization) {
            any_hash_specialization = true;
        }

        if (info.ostream_operator) {
            any_ostream_operator = true;
        }

        if (info.istream_operator) {
            any_istream_operator = true;
        }

        if (info.formatter_specialization) {
            any_formatter_specialization = true;
        }

        // Collect includes and guards from this type
        for (auto const & include : info.includes_vec) {
            all_includes.insert(include);
        }
        for (auto const & [header, guard] : info.include_guards) {
            all_guards[header] = guard;
        }

        // Generate just the type code
        combined_code << render_code(info);
    }

    // Output warnings to stderr
    print_warnings(warnings);

    // Generate header guard with SHA of combined content
    std::string content = combined_code.str();

    // Create a temporary description for guard generation
    StrongTypeDescription temp_desc{
        .kind = "struct",
        .type_namespace = "foo",
        .type_name = "Bar",
        .description = "string int; ->",
        .guard_prefix = guard_prefix,
        .guard_separator = guard_separator,
        .upcase_guard = upcase_guard};
    std::string guard = GuardGenerator::make_guard(temp_desc, content);

    // Add preamble includes to the collected includes
    // Merge per-type requests with global auto_opts
    // If any type requests hash/ostream/istream/format, enable the automatic
    // support for ALL types via the preamble boilerplate
    PreambleOptions preamble_opts{
        .include_arrow_operator_traits = any_arrow_operator,
        .include_dereference_operator_traits = any_indirection_operator,
        .include_checked_helpers = any_checked_arithmetic,
        .include_saturating_helpers = any_saturating_arithmetic,
        .include_constraints = any_constraints,
        .include_nilable_support = any_nil_value,
        .include_hash_drill = any_hash_specialization || auto_opts.auto_hash,
        .include_ostream_drill = any_ostream_operator || auto_opts.auto_ostream,
        .include_istream_drill = any_istream_operator || auto_opts.auto_istream,
        .include_format_drill = any_formatter_specialization || auto_opts.auto_format,
        .auto_hash = auto_opts.auto_hash || any_hash_specialization,
        .auto_ostream = auto_opts.auto_ostream || any_ostream_operator,
        .auto_istream = auto_opts.auto_istream || any_istream_operator,
        .auto_format = auto_opts.auto_format || any_formatter_specialization};
    auto preamble_includes = get_preamble_includes(preamble_opts);
    for (auto const & include : preamble_includes) {
        all_includes.insert(include);
    }

    // Build final output
    std::ostringstream output;

    // Add header guard first, then static_assert, then NOTICE banner
    output << "#ifndef " << guard << '\n'
        << "#define " << guard << "\n\n"
        << generate_cpp_standard_assertion(max_cpp_standard)
        << GuardGenerator::make_notice_banner() << '\n'
        << R"(#if __has_include(<version>)
#include <version>
#endif
)";

    // Remove <compare> from top-level includes since it's already
    // conditionally included in the preamble
    all_includes.erase("<compare>");
    all_includes.erase("<version>");

    // Add all unique includes with guards
    for (auto const & include : all_includes) {
        auto guard_it = all_guards.find(include);
        if (guard_it != all_guards.end()) {
            output << "#if " << guard_it->second << '\n';
            output << "#include " << include << '\n';
            output << "#endif\n";
        } else {
            output << "#include " << include << '\n';
        }
    }
    if (not all_includes.empty()) {
        output << '\n';
    }

    // Add strong_type_tag definition once for the entire file
    output << preamble(preamble_opts);
    output << content << "#endif // " << guard << '\n';

    return output.str();
}


} // namespace wjh::atlas
