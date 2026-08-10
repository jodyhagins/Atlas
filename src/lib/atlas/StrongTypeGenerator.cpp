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
#include "SHA1Hasher.hpp"
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

namespace {

// A type name reduced to the single spelling shared by every equivalent
// spelling of that type.
struct CanonicalTypeName
{
    // Whitespace-canonical, with an explicit global qualifier preserved.
    std::string spelling;

    // The spelling with a leading `::` removed. Type identity ignores the
    // global qualifier, so the key -- not the spelling -- decides identity.
    std::string key;
};

// The include guard is keyed on text, but C++ type identity is semantic:
// `ext::Ref<int, 2>`, `ext::Ref<int,2>`, and `::ext::Ref<int,2>` all name one
// type and so must produce one key. Whitespace is the only difference this
// reduction can safely erase, and only where erasing it cannot fuse two
// identifiers into one (`unsigned int` must not become `unsignedint`).
CanonicalTypeName
canonicalize_type_name(std::string_view name)
{
    auto is_space = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
            c == '\f' || c == '\r';
    };
    auto is_identifier_char = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_';
    };

    while (not name.empty() && is_space(name.front())) {
        name.remove_prefix(1);
    }
    while (not name.empty() && is_space(name.back())) {
        name.remove_suffix(1);
    }

    CanonicalTypeName result;
    result.spelling.reserve(name.size());
    for (std::size_t n = 0; n < name.size();) {
        if (not is_space(name[n])) {
            result.spelling += name[n++];
            continue;
        }
        while (n < name.size() && is_space(name[n])) {
            ++n;
        }
        if (n < name.size() && is_identifier_char(result.spelling.back()) &&
            is_identifier_char(name[n]))
        {
            result.spelling += ' ';
        }
    }

    result.key = result.spelling;
    if (result.key.compare(0, 2, "::") == 0) {
        result.key.erase(0, 2);
    }
    return result;
}

// Emit an atlas::borrowed_type specialization for each declared borrowing
// type, individually include-guarded so that two generated headers naming
// the same type in one translation unit do not collide. Spellings that name
// the same type share one guard, so only the first one included is compiled.
std::string
render_borrowed_type_specializations(
    std::vector<std::string> const & borrowed_types)
{
    std::ostringstream out;
    std::set<std::string> emitted;
    for (auto const & type_name : borrowed_types) {
        auto const canonical = canonicalize_type_name(type_name);
        if (canonical.key.empty() || not emitted.insert(canonical.key).second)
        {
            continue;
        }

        std::string guard = "ATLAS_BORROWED_TYPE_" + get_sha1(canonical.key);
        for (auto & c : guard) {
            c = static_cast<char>(
                std::toupper(static_cast<unsigned char>(c)));
        }
        // The specialization is emitted inside `namespace atlas`, where
        // dropping an explicit global qualifier could change lookup, so the
        // emitted text keeps the qualifier the user wrote.
        out << "#ifndef " << guard << '\n'
            << "#define " << guard << '\n'
            << "namespace atlas {\n"
            << "template <>\n"
            << "struct borrowed_type<" << canonical.spelling
            << "> : std::true_type {};\n"
            << "} // namespace atlas\n"
            << "#endif // " << guard << "\n\n";
    }
    return out.str();
}

// Forward-declare the types defined in this file when any of them carries a
// from_atlas_cast allowlist or the file declares borrowed_types=: either
// directive may name a type that is defined in this file (possibly later in
// it), and both the generated constraint and the borrowed_type
// specialization require the name to be declared. Nested type names cannot
// be forward-declared at namespace scope and are skipped.
std::string
render_forward_declarations(
    std::vector<StrongTypeDescription> const & descriptions,
    bool needed)
{
    if (not needed) {
        return "";
    }

    std::ostringstream out;
    for (auto const & desc : descriptions) {
        if (desc.type_name.find("::") != std::string::npos) {
            continue;
        }

        // Expand the namespace one level at a time for C++11 compatibility.
        std::string_view ns = desc.type_namespace;
        while (ns.substr(0, 2) == "::") {
            ns.remove_prefix(2);
        }
        std::string open;
        std::string close;
        while (not ns.empty()) {
            auto pos = ns.find("::");
            auto part = ns.substr(0, pos);
            open += "namespace " + std::string(part) + " { ";
            close += " }";
            ns.remove_prefix(pos == std::string_view::npos ? ns.size()
                                                           : pos + 2);
        }

        out << open << desc.kind << ' ' << desc.type_name << ';' << close
            << '\n';
    }
    auto decls = out.str();
    if (not decls.empty()) {
        decls += '\n';
    }
    return decls;
}

} // anonymous namespace

std::string
generate_strong_types_file(
    std::vector<StrongTypeDescription> const & descriptions,
    std::string const & guard_prefix,
    std::string const & guard_separator,
    bool upcase_guard,
    PreambleOptions auto_opts,
    std::vector<std::string> const & borrowed_types)
{
    std::set<std::string> all_includes;
    std::map<std::string, std::string> all_guards;
    std::ostringstream combined_code;
    std::vector<StrongTypeGenerator::Warning> warnings;
    bool any_arrow_operator = false;
    bool any_from_atlas_cast_allowlist = false;
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

        // When auto_hash is enabled at file level, every type gets a
        // per-type hash specialization (needed for the pre-C++20 path).
        if (auto_opts.auto_hash && !info.hash_specialization) {
            info.hash_specialization = true;
            info.includes_vec.push_back("<functional>");
            // set_qualified_name() in parse() skips types without
            // hash_specialization, so populate it now.
            if (not info.class_namespace.empty()) {
                info.full_qualified_name = info.class_namespace +
                    "::" + info.full_class_name;
            } else {
                info.full_qualified_name = info.full_class_name;
            }
        }

        if (info.cpp_standard > max_cpp_standard) {
            max_cpp_standard = info.cpp_standard;
        }

        if (info.arrow_operator) {
            any_arrow_operator = true;
        }

        // Explicit from_atlas_cast<...> allowlists live in the description
        // tokens, so they are only known after ClassInfo::parse has merged
        // them into the description's list.
        if (not info.desc.from_atlas_cast_types.empty()) {
            any_from_atlas_cast_allowlist = true;
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
        .include_format_drill = any_formatter_specialization ||
            auto_opts.auto_format,
        .auto_hash = auto_opts.auto_hash || any_hash_specialization,
        .auto_ostream = auto_opts.auto_ostream || any_ostream_operator,
        .auto_istream = auto_opts.auto_istream || any_istream_operator,
        .auto_format = auto_opts.auto_format || any_formatter_specialization,
        .constexpr_hash = auto_opts.constexpr_hash};
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

    // Same-file forward declarations must precede the borrowed-type
    // specializations, which must precede every type definition: the
    // specializations feed the lifetime guards' overload resolution, and
    // either borrowed_types= or a from_atlas_cast allowlist may name a type
    // this file defines (possibly later in it).
    output << render_forward_declarations(
        descriptions,
        any_from_atlas_cast_allowlist || not borrowed_types.empty());
    output << render_borrowed_type_specializations(borrowed_types);

    output << content << "#endif // " << guard << '\n';

    return output.str();
}


} // namespace wjh::atlas
