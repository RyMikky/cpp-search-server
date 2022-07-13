#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;

    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    const int64_t pos_end = str.npos;
    while (str.size() != 0) {

        int64_t next_space = str.find(' ');
        if (next_space == pos_end) {
            result.push_back(str.substr(0));
            str.remove_prefix(str.size());
        }
        else {
            result.push_back(str.substr(0, next_space));
            str.remove_prefix(next_space);
        }

        str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    }

    return result;
}