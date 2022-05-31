#include "search_server.h"
#include "remove_duplicates.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const auto words = SearchServer::SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    
    documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);

    // Автоматическое отсеивание дубликатов документов при добавлении на сервер, 
    // Если включить, то из remove_duplicate.cpp/.h будут браться функции обработки контейнеров
    /*int duplicat_count = 0;
    for (auto& element : document_to_word_freqs_) {
        if (WordsEqualCheck(element.second, document_to_word_freqs_[document_id])) {
            duplicat_count++;
        }
    }
    if (duplicat_count <= 1) {
        documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
        document_ids_.push_back(document_id);
    }
    else {
        SearchServer::RemoveDocument(document_id);
    }*/
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

// забыл про неё, простите!
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    std::map<std::string, double> dummy;
    dummy.emplace(" ", 0.0);
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }
    else {
        return dummy;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    
    // Чистим std::vector<int> document_ids_;
    if (find(SearchServer::document_ids_.begin(), SearchServer::document_ids_.end(), document_id) != SearchServer::document_ids_.end()) {
        remove(SearchServer::document_ids_.begin(), SearchServer::document_ids_.end(), document_id);
        SearchServer::document_ids_.resize(SearchServer::GetDocumentCount() - 1);
    }

    // Чистим std::map<int, DocumentData> documents_;
    if (documents_.count(document_id)) {
        documents_.erase(document_id);
    }

    // чистим std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    for (auto& element : SearchServer::word_to_document_freqs_) {
        if (element.second.count(document_id)) {
            element.second.erase(document_id);
        }
    }

    // чистим std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    if (document_to_word_freqs_.count(document_id)) {
        document_to_word_freqs_.erase(document_id);
    }
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

SearchServer::DocumentData::DocumentData(int rating, DocumentStatus status) 
    : rating(rating), status(status) {
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word " + word + " is invalid");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord::QueryWord(std::string data, bool is_minus, bool is_stop) 
    : data(data), is_minus(is_minus), is_stop(is_stop) {
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word " + text + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query::Query(std::set<std::string> plus_words, std::set<std::string> minus_words)
    : plus_words(plus_words), minus_words(minus_words) {
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}