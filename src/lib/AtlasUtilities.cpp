// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"

#include <boost/uuid/detail/sha1.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
#else
    #include <unistd.h>
#endif

namespace wjh::atlas { inline namespace v1 {

std::string
get_sha1(std::string const & s)
{
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(s.data(), s.size());
    boost::uuids::detail::sha1::digest_type hash;
    sha1.get_digest(hash);

    std::string result;
    for (unsigned int x : hash) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%08x", x);
        result += buffer;
    }
    return result;
}

std::string
generate_header_guard(
    std::string const & prefix,
    std::string const & separator,
    std::string const & content_hash,
    bool upcase)
{
    std::string guard_prefix = prefix.empty() ? "ATLAS" : prefix;
    std::string guard = guard_prefix + separator + content_hash;

    if (upcase) {
        std::transform(
            guard.begin(),
            guard.end(),
            guard.begin(),
            [](unsigned char c) { return std::toupper(c); });
    }

    return guard;
}

std::string
trim(std::string const & str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string
preamble()
{
    static constexpr char const result[] = R"(
#ifndef WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90
#define WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90

// This is boilerplate that is part of every Atlas interaction file.
// Nothing to see here, move along.

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
#include <compare>
#endif
#include <type_traits>
#include <utility>

namespace atlas {

struct strong_type_tag
{
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    friend auto operator <=> (
        strong_type_tag const &,
        strong_type_tag const &) = default;
#endif
};

struct value_tag
{ };

namespace atlas_detail {

template <typename... Ts>
struct make_void
{
    using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T, typename = void>
struct IsAtlasType
: std::false_type
{ };

template <typename T>
struct IsAtlasType<T, void_t<typename T::atlas_value_type>>
: std::true_type
{ };

template <std::size_t N>
struct PriorityTag
: PriorityTag<N - 1>
{ };

template <>
struct PriorityTag<0u>
{ };

using value_tag = PriorityTag<3>;

template <bool B>
using bool_c = std::integral_constant<bool, B>;
template <typename T>
using bool_ = bool_c<T::value>;
template <typename T>
using not_ = bool_c<not T::value>;
template <typename T, typename U>
using and_ = bool_c<T::value && U::value>;
template <typename T>
using is_lref = std::is_lvalue_reference<T>;
template <typename T, typename U = void>
using enable_if = typename std::enable_if<T::value, U>::type;

template <typename T>
using _t = typename T::type;

void atlas_value();

template <typename T>
constexpr T &
value(T & val, PriorityTag<0>)
{
    return val;
}

template <typename T, typename U = typename T::atlas_value_type>
using val_t = _t<std::conditional<std::is_const<T>::value, U const &, U &>>;

template <typename T, typename U = val_t<T>>
constexpr auto
value(T & val, PriorityTag<1>)
-> decltype(atlas::atlas_detail::value(static_cast<U>(val), value_tag{}))
{
    return atlas::atlas_detail::value(static_cast<U>(val), value_tag{});
}

template <typename T>
constexpr auto
value(T const & t, PriorityTag<2>)
-> decltype(atlas_value(t, atlas::value_tag{}))
{
    return atlas_value(t, atlas::value_tag{});
}

template <typename T>
constexpr auto
value(T const & t, PriorityTag<3>)
-> decltype(atlas_value(t))
{
    return atlas_value(t);
}

class Value
{
    template <
        typename U,
        typename T,
        typename V = _t<std::conditional<is_lref<U &&>::value, T &, T>>>
    static constexpr V rval(T && t)
    {
        return t;
    }

public:
    template <typename T>
    constexpr auto operator () (T && t) const
    -> decltype(rval<T>(atlas_detail::value(t, atlas_detail::value_tag{})))
    {
        return rval<T>(atlas_detail::value(t, atlas_detail::value_tag{}));
    }
};

} // namespace atlas_detail

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
inline constexpr auto value = atlas_detail::Value{};
#else
template <typename T>
constexpr auto
value(T && t)
-> decltype(atlas_detail::Value{}(std::forward<T>(t)))
{
    return atlas_detail::Value{}(std::forward<T>(t));
}
#endif

} // namespace atlas

#endif // WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90


//////////////////////////////////////////////////////////////////////
///
/// These are the droids you are looking for!
///
//////////////////////////////////////////////////////////////////////

)";
    return result + 1;
}

bool
supports_color(int fd)
{
    // Check if output is a TTY
    if (not isatty(fd)) {
        return false;
    }

    // Check TERM environment variable
    auto term = std::getenv("TERM");
    if (not term) {
        return false;
    }

    std::string term_str(term);
    // Most modern terminals support color
    return term_str != "dumb";
}

}} // namespace wjh::atlas::v1
