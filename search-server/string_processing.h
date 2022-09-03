#pragma once
#include <string>
#include <vector>
#include <set>
#include <iostream>

using namespace std::string_literals;
using namespace std::string_view_literals;

std::vector<std::string> SplitIntoWords(const std::string& text);

std::vector<std::string_view> SplitIntoWords(std::string_view str);

using VirturlStringSet = std::set<std::string, std::less<>>;

template <typename StringContainer>
VirturlStringSet MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    VirturlStringSet non_empty_strings;
    for (std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}