#pragma once

#include <map>
#include <algorithm>
#include <cmath>
#include <vector>
#include <execution>
#include <string>
#include <utility>
#include <string_view>
#include <thread>
//#include <numeric>
//#include <functional>
//#include <cassert>

#include "read_input_functions.h"
#include "document.h"
#include "concurrent_map.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_THRESHOLD = 1e-6;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(std::string_view stop_words_text);

    explicit SearchServer(const std::string& stop_words_text);


    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);


    // принимающие политику по количеству потоков
    template <typename Execution, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const Execution& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename Execution>
    std::vector<Document> FindTopDocuments(const Execution& policy, std::string_view raw_query, DocumentStatus status) const;

    template <typename Execution>
    std::vector<Document> FindTopDocuments(const Execution& policy, std::string_view raw_query) const;


    // заведомо однопоточные
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;


    size_t GetDocumentCount() const {
        return documents_.size();
    }

    auto begin() const {
        return SearchServer::document_ids_.begin();
    }

    auto end() const {
        return SearchServer::document_ids_.end();
    }

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;


    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
        RemoveDocument(document_id);
    }

    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    

    std::tuple<std::vector<std::string_view>, DocumentStatus>
        MatchDocument(std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
        MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
        MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;

    int GetDocumentId(int index) const;


    // =========== ФУНКЦИОНАЛ ДЛЯ РАБОТЫ БЕНЧМАРКОВ =============
    
    // Debug-функции для бенчмарков и тестов
    const auto QueryTest(std::string_view text) const {
        return ParseQuery(text);
    }
    // Debug-функции для бенчмарков и тестов
    const auto VectorNoSDQueryTest(std::string_view text) const {
        return ParseVecQueryNOSD(text);
    }
    // Debug-функции для бенчмарков и тестов
    const auto VectorWSDQueryTest(std::string_view text) const {
        return ParseVecQueryWSD(text);
    }
    // Debug-функции для бенчмарков и тестов
    const auto GetCurrentStopWords() const {
        return stop_words_;
    }
    // Debug-функции для бенчмарков и тестов
    const auto GetMassive() const {
        return word_to_document_freqs_;
    }
    // Debug-функции для бенчмарков и тестов
    const auto GetDocumentMassive() const {
        return document_to_word_freqs_;
    }

private:
    // новая DocumentData
    struct DocumentData {
        DocumentData() = default;

        DocumentData(int rating_, DocumentStatus status_, std::string text_);

        int rating_ = 0;
        DocumentStatus status_ = DocumentStatus::ACTUAL;
        std::string text_;
    };

    const VirturlStringSet stop_words_;
    // массивы для хранения всех слов которые попадают в программу

    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<std::string_view, double> dummy_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        QueryWord() = default;

        QueryWord(std::string_view data, bool is_minus, bool is_stop);

        std::string_view data;
        bool is_minus = false;
        bool is_stop = false;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    // ================= ВКЛЮЧЕНА ДЛЯ БЕНЧМАРКА =====================
    // Стандартная версия Query на контейнере std::set на string_view
    struct Query {
        Query() = default;
    
        Query(std::set<std::string_view> plus_words, std::set<std::string_view> minus_words);
    
        std::set<std::string_view> plus_words = {};
        std::set<std::string_view> minus_words = {};
    };
    // ================= ВКЛЮЧЕНА ДЛЯ БЕНЧМАРКА =====================
    // Стандартная версия Query на контейнере std::set
    Query ParseQuery(std::string_view text) const;


    // Векторная версия Query без сортировки и удаления дубликатов на string_view
    // Данная версия применяется для работы с параллельным матчигом
    struct VecQueryNoSD {
        VecQueryNoSD() = default;

        VecQueryNoSD(std::vector<std::string_view> plus_words, std::vector<std::string_view> minus_words);

        std::vector<std::string_view> plus_words = {};
        std::vector<std::string_view> minus_words = {};
    };
    // Векторная версия Query без сортировки и удаления дубликатов на string_view
    // Данная версия применяется для работы с параллельным матчигом
    VecQueryNoSD ParseVecQueryNOSD(std::string_view text) const;


    // Векторная версия Query с сортировкой и удалением дубликатов на string_view
    // Данная версия применяется для работы остальных функций
    struct VecQueryWSD {
        VecQueryWSD() = default;

        VecQueryWSD(std::vector<std::string_view> plus_words, std::vector<std::string_view> minus_words);

        std::vector<std::string_view> plus_words = {};
        std::vector<std::string_view> minus_words = {};
    };
    // Векторная версия Query с сортировкой и удалением дубликатов на string_view
    // Данная версия применяется для работы остальных функций
    VecQueryWSD ParseVecQueryWSD(std::string_view text) const;

    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    // Версия для работы без предиката
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const VecQueryWSD& query) const;
    // Версия для работы без предиката
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const VecQueryWSD& query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, 
        const VecQueryWSD& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, 
        const VecQueryWSD& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename Execution, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const Execution& policy, 
    std::string_view raw_query, DocumentPredicate document_predicate) const {

    const VecQueryWSD query = ParseVecQueryWSD(raw_query);

    std::vector<Document> matched_documents = FindAllDocuments(policy, query, document_predicate);;

    std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_THRESHOLD) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;

    //// Если предикат не попадает в FindAllDocuments 
    //// Скорость выполнения последовательной версии возрастает
    //// Однако треннажер такую версию не пропускает
    //std::vector<Document> pre_matched_documents = FindAllDocuments(policy, query);
    //std::vector<Document> matched_documents;

    //for (Document& document : pre_matched_documents) {
    //    if (document_predicate(document.id, documents_.at(document.id).status_, document.rating)) {
    //        matched_documents.push_back(document);
    //    }
    //}

    //std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
    //    if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_THRESHOLD) {
    //        return lhs.rating > rhs.rating;
    //    }
    //    else {
    //        return lhs.relevance > rhs.relevance;
    //    }
    //});
    //
    //if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
    //    matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    //}

    //return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename Execution>
std::vector<Document> SearchServer::FindTopDocuments(const Execution& policy, 
    std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

template <typename Execution>
std::vector<Document> SearchServer::FindTopDocuments(const Execution& policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&,
    const SearchServer::VecQueryWSD& query, DocumentPredicate document_predicate) const {

    size_t const MIN_PER_THREAD = 25;
    size_t const MAX_THREADS = (query.plus_words.size() + MIN_PER_THREAD - 1) / MIN_PER_THREAD;
    size_t const HARDWARE_THREADS = std::thread::hardware_concurrency();
    size_t const NUM_THREADS = std::min(HARDWARE_THREADS != 0 ? HARDWARE_THREADS : 2, MAX_THREADS);
    size_t const BLOCK_SIZE = query.plus_words.size() / NUM_THREADS;

    ConcurrentMap<int, double> document_to_relevance_(BLOCK_SIZE);


    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        [this, document_predicate, &document_to_relevance_](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status_, document_data.rating_)) {
                        document_to_relevance_[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });

    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance_](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance_.EraseKey(document_id);
                }
            }
        });



    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance_.BuildOrdinaryMap()) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating_ });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&,
    const SearchServer::VecQueryWSD& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;

    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status_, document_data.rating_)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating_ });
    }
    return matched_documents;
}
