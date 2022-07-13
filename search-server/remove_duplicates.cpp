#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    size_t total_documents = search_server.GetDocumentCount();
    std::set<int> duplicats_ = {};
    std::set<std::set<std::string_view>> check_ = {};

    for (int i = 1; i <= total_documents; i++) {
        const auto& document_ = search_server.GetWordFrequencies(i);
        if (document_.empty()) {
            continue;
        }
        else {
            auto document_words_ = WordsSetCreator(document_);
            if (check_.count(document_words_)) {
                duplicats_.insert(i);
            }
            else {
                check_.insert(document_words_);
            }
        } 
    }

    for (auto duplicat : duplicats_) {
        std::cout << "Found duplicate document id " << duplicat << std::endl;
        search_server.RemoveDocument(duplicat);
    }
}

std::set<std::string_view> WordsSetCreator(const std::map<std::string_view, double>& element) {
    std::set<std::string_view> result = {};
    for (const auto& e : element) {
        result.insert(e.first);
    }
    return result;
}
