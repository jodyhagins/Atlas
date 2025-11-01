// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "TokenToHeaderMapper.hpp"

#include <algorithm>

namespace wjh::atlas { inline namespace v1 {

HeaderMapper::
HeaderMapper()
{
    init_exact_matches();
    init_namespace_prefixes();
}

void
HeaderMapper::
init_exact_matches()
{
    // Common standard library types
    exact_matches_["std::any"] = "<any>";
    exact_matches_["std::binary_semaphore"] = "<semaphore>";
    exact_matches_["std::condition_variable"] = "<condition_variable>";
    exact_matches_["std::condition_variable_any"] = "<condition_variable>";
    exact_matches_["std::ifstream"] = "<fstream>";
    exact_matches_["std::iostream"] = "<iostream>";
    exact_matches_["std::istringstream"] = "<sstream>";
    exact_matches_["std::jthread"] = "<thread>";
    exact_matches_["std::latch"] = "<latch>";
    exact_matches_["std::mutex"] = "<mutex>";
    exact_matches_["std::ofstream"] = "<fstream>";
    exact_matches_["std::ostringstream"] = "<sstream>";
    exact_matches_["std::reference_wrapper"] = "<functional>";
    exact_matches_["std::regex"] = "<regex>";
    exact_matches_["std::recursive_mutex"] = "<mutex>";
    exact_matches_["std::recursive_timed_mutex"] = "<mutex>";
    exact_matches_["std::shared_mutex"] = "<shared_mutex>";
    exact_matches_["std::shared_timed_mutex"] = "<shared_mutex>";
    exact_matches_["std::stop_source"] = "<stop_token>";
    exact_matches_["std::stop_token"] = "<stop_token>";
    exact_matches_["std::string"] = "<string>";
    exact_matches_["std::string_view"] = "<string_view>";
    exact_matches_["std::stringstream"] = "<sstream>";
    exact_matches_["std::thread"] = "<thread>";
    exact_matches_["std::thread_id"] = "<thread>";
    exact_matches_["std::timed_mutex"] = "<mutex>";

    // Container types
    exact_matches_["std::array"] = "<array>";
    exact_matches_["std::basic_string"] = "<string>";
    exact_matches_["std::basic_string_view"] = "<string_view>";
    exact_matches_["std::bitset"] = "<bitset>";
    exact_matches_["std::deque"] = "<deque>";
    exact_matches_["std::forward_list"] = "<forward_list>";
    exact_matches_["std::list"] = "<list>";
    exact_matches_["std::map"] = "<map>";
    exact_matches_["std::multimap"] = "<map>";
    exact_matches_["std::multiset"] = "<set>";
    exact_matches_["std::optional"] = "<optional>";
    exact_matches_["std::priority_queue"] = "<queue>";
    exact_matches_["std::queue"] = "<queue>";
    exact_matches_["std::set"] = "<set>";
    exact_matches_["std::span"] = "<span>";
    exact_matches_["std::stack"] = "<stack>";
    exact_matches_["std::tuple"] = "<tuple>";
    exact_matches_["std::unordered_map"] = "<unordered_map>";
    exact_matches_["std::unordered_multimap"] = "<unordered_map>";
    exact_matches_["std::unordered_multiset"] = "<unordered_set>";
    exact_matches_["std::unordered_set"] = "<unordered_set>";
    exact_matches_["std::variant"] = "<variant>";
    exact_matches_["std::vector"] = "<vector>";

    // Memory and functional
    exact_matches_["std::atomic"] = "<atomic>";
    exact_matches_["std::barrier"] = "<barrier>";
    exact_matches_["std::basic_regex"] = "<regex>";
    exact_matches_["std::counting_semaphore"] = "<semaphore>";
    exact_matches_["std::expected"] = "<expected>";
    exact_matches_["std::function"] = "<functional>";
    exact_matches_["std::hash"] = "<functional>";
    exact_matches_["std::pair"] = "<utility>";
    exact_matches_["std::shared_ptr"] = "<memory>";
    exact_matches_["std::stop_callback"] = "<stop_token>";
    exact_matches_["std::unique_ptr"] = "<memory>";
    exact_matches_["std::weak_ptr"] = "<memory>";

    // PMR container aliases - each in its respective header
    exact_matches_["std::pmr::deque"] = "<deque>";
    exact_matches_["std::pmr::forward_list"] = "<forward_list>";
    exact_matches_["std::pmr::list"] = "<list>";
    exact_matches_["std::pmr::map"] = "<map>";
    exact_matches_["std::pmr::multimap"] = "<map>";
    exact_matches_["std::pmr::multiset"] = "<set>";
    exact_matches_["std::pmr::set"] = "<set>";

    // PMR string types from <string>
    exact_matches_["std::pmr::string"] = "<string>";
    exact_matches_["std::pmr::u16string"] = "<string>";
    exact_matches_["std::pmr::u32string"] = "<string>";
    exact_matches_["std::pmr::u8string"] = "<string>";
    exact_matches_["std::pmr::wstring"] = "<string>";

    // PMR unordered containers
    exact_matches_["std::pmr::unordered_map"] = "<unordered_map>";
    exact_matches_["std::pmr::unordered_multimap"] = "<unordered_map>";
    exact_matches_["std::pmr::unordered_multiset"] = "<unordered_set>";
    exact_matches_["std::pmr::unordered_set"] = "<unordered_set>";

    // PMR vector from <vector>
    exact_matches_["std::pmr::vector"] = "<vector>";

    // PMR regex types from <regex>
    exact_matches_["std::pmr::cmatch"] = "<regex>";
    exact_matches_["std::pmr::match_results"] = "<regex>";
    exact_matches_["std::pmr::smatch"] = "<regex>";
    exact_matches_["std::pmr::wcmatch"] = "<regex>";
    exact_matches_["std::pmr::wsmatch"] = "<regex>";
}

void
HeaderMapper::
init_namespace_prefixes()
{
    // These are checked with starts_with logic.
    namespace_prefixes_.emplace_back("std::chrono::", "<chrono>");
    namespace_prefixes_.emplace_back("std::execution::", "<execution>");
    namespace_prefixes_.emplace_back("std::filesystem::", "<filesystem>");
    namespace_prefixes_.emplace_back("std::pmr::", "<memory_resource>");
    namespace_prefixes_.emplace_back("std::ranges::", "<ranges>");
}

std::vector<std::string>
HeaderMapper::
get_headers(std::string_view token) const
{
    std::vector<std::string> result;

    if (auto it = exact_matches_.find(token); it != exact_matches_.end()) {
        result.emplace_back(it->second);
    }

    for (auto && s : check_integral_type(token)) {
        result.push_back(std::move(s));
    }

    for (auto && s : check_namespace_prefix(token)) {
        result.push_back(std::move(s));
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

std::vector<std::string>
HeaderMapper::
check_integral_type(std::string_view token) const
{
    std::vector<std::string> result;

    if (token.starts_with("std::")) {
        token.remove_prefix(5);
    }
    if (token.length() > 2 && token.substr(token.length() - 2) == "_t" &&
        std::ranges::all_of(token, [](unsigned char c) {
            return std::islower(c) || std::isdigit(c) || c == '_';
        }))
    {
        for (auto s : {"int", "uint"}) {
            if (token.starts_with(s)) {
                result.emplace_back("<cstdint>");
            }
        }
        for (auto s : {"size", "ptrdiff", "ssize", "max_align"}) {
            if (token.starts_with(s)) {
                result.emplace_back("<cstddef>");
            }
        }
    }

    return result;
}

std::vector<std::string>
HeaderMapper::
check_namespace_prefix(std::string_view token) const
{
    std::vector<std::string> result;

    for (auto const & [prefix, header] : namespace_prefixes_) {
        if (token.starts_with(prefix)) {
            result.emplace_back(header);
        }
    }

    return result;
}

}} // namespace wjh::atlas::v1
