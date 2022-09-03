#include "search_server.h"

SearchServer::SearchServer(std::string_view stop_words_text) 
    : SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }

    //предварительный чек, чтобы в случае кривого документа ничего не делать и не сохранять в памяти
    {
        auto check = SearchServer::SplitIntoWordsNoStop(document);
    }
    
    // создаём строку из string_view для передачи на хранение
    const std::string document_string{ document };

    //вставляем строку в documents_ там и храним в дальнейшем
    documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status, document_string });

    const auto words = SearchServer::SplitIntoWordsNoStop(documents_.at(document_id).text_);

    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }

    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }
    else {
        return dummy_;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    
    // Чистим std::set<int> document_ids_;
    if (!document_ids_.count(document_id)) {
        using namespace std::literals::string_literals;
        throw std::invalid_argument("Invalid document ID to remove"s);
    }

    document_ids_.erase(document_id);
    
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

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {

    if (!document_ids_.count(document_id)) {
        using namespace std::literals::string_literals;
        throw std::invalid_argument("Invalid document ID to remove"s);
    }

    //версия на векторе указателей на words
    std::map<std::string_view, double> word_freqs_ = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> words_(word_freqs_.size());

    std::transform(std::execution::par, word_freqs_.begin(), word_freqs_.end(), words_.begin(),
        [](const auto item) { return item.first; });

    std::for_each(std::execution::par, words_.begin(), words_.end(),
        [this, document_id](std::string_view word) {
            word_to_document_freqs_.at(word).erase(document_id); });

    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);

}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
    
    const auto query = ParseVecQueryWSD(raw_query);

    std::vector<std::string_view> matched_words;

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { matched_words, documents_.at(document_id).status_ };
        }
    }

    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status_ };
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const {

    const auto query = ParseVecQueryNOSD(raw_query);

    // сразу ищем минуса, если таковые будут то выходим из функции с нулем.
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](std::string_view word) {return word_to_document_freqs_.at(word).count(document_id); })) {
        return { {}, documents_.at(document_id).status_ };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto end_it = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](std::string_view word) {return word_to_document_freqs_.at(word).count(document_id); });

    std::sort(matched_words.begin(), end_it);
    end_it = unique(matched_words.begin(), end_it);
    matched_words.erase(end_it, matched_words.end());

    return { matched_words, documents_.at(document_id).status_ };
}


int SearchServer::GetDocumentId(int index) const {
    if (index >= 0 && index < GetDocumentCount()) {
        return *std::next(document_ids_.begin(), index);
    }

    throw std::out_of_range("index out of range");
}

SearchServer::DocumentData::DocumentData(int rating, DocumentStatus status, std::string text)
    : rating_(rating), status_(status), text_(text) {
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            std::string string_word{ word };
            throw std::invalid_argument("Word {\"" + string_word + "\"} is invalid");
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

SearchServer::QueryWord::QueryWord(std::string_view data, bool is_minus, bool is_stop)
    : data(data), is_minus(is_minus), is_stop(is_stop) {
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string_view word(text);
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        std::string string_word(text);
        throw std::invalid_argument("Query word {\"" + string_word + "\"} is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}

// ================= ВКЛЮЧЕНА ДЛЯ БЕНЧМАРКА =====================
// Стандартная версия Query на контейнере std::set на string_view
SearchServer::Query::Query(std::set<std::string_view> plus_words, std::set<std::string_view> minus_words)
    : plus_words(plus_words), minus_words(minus_words) {
}
// ================= ВКЛЮЧЕНА ДЛЯ БЕНЧМАРКА =====================
// Стандартная версия Query на контейнере std::set на string_view
SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (std::string_view word : SplitIntoWords(text)) {
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


// Векторная версия Query без сортировки и удаления дубликатов
// Данная версия применяется для работы с параллельным матчигом
SearchServer::VecQueryNoSD::VecQueryNoSD(std::vector<std::string_view> plus_words, std::vector<std::string_view> minus_words)
    : plus_words(plus_words), minus_words(minus_words) {
}
// Векторная версия Query без сортировки и удаления дубликатов
// Данная версия применяется для работы с параллельным матчигом
SearchServer::VecQueryNoSD SearchServer::ParseVecQueryNOSD(std::string_view text) const {
    
    VecQueryNoSD result;
    
    for (std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}

// Векторная версия Query с сортировкой и удалением дубликатов на string_view
// Данная версия применяется для работы остальных функций
SearchServer::VecQueryWSD::VecQueryWSD(std::vector<std::string_view> plus_words, std::vector<std::string_view> minus_words)
    : plus_words(plus_words), minus_words(minus_words) {
}
// Векторная версия Query с сортировкой и удалением дубликатов на string_view
// Данная версия применяется для работы остальных функций
SearchServer::VecQueryWSD SearchServer::ParseVecQueryWSD(std::string_view text) const {

    std::vector<std::string_view> plus_words;
    std::vector<std::string_view> minus_words;

    for (std::string_view word : SplitIntoWords(text)) {
        const SearchServer::QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                minus_words.push_back(query_word.data);
            }
            else {
                plus_words.push_back(query_word.data);
            }
        }
    }

    VecQueryWSD result;
    result.minus_words.resize(minus_words.size());
    result.plus_words.resize(plus_words.size());

    auto copy_sort_deduplucator =
        [](std::vector<std::string_view>& input_vector, std::vector<std::string_view>& output_vector) {
        auto end_it = std::move(std::execution::par, input_vector.begin(), input_vector.end(), output_vector.begin());
        std::sort(std::execution::par, output_vector.begin(), end_it);
        end_it = unique(output_vector.begin(), end_it);
        output_vector.erase(end_it, output_vector.end());
    };

    copy_sort_deduplucator(plus_words, result.plus_words);
    copy_sort_deduplucator(minus_words, result.minus_words);

    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

// Версия для работы без предиката
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&,
    const SearchServer::VecQueryWSD& query) const {

    size_t const MIN_PER_THREAD = 25;
    size_t const MAX_THREADS = (query.plus_words.size() + MIN_PER_THREAD - 1) / MIN_PER_THREAD;
    size_t const HARDWARE_THREADS = std::thread::hardware_concurrency();
    size_t const NUM_THREADS = std::min(HARDWARE_THREADS != 0 ? HARDWARE_THREADS : 2, MAX_THREADS);
    size_t const BLOCK_SIZE = query.plus_words.size() / NUM_THREADS;

    ConcurrentMap<int, double> document_to_relevance_(BLOCK_SIZE);


    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        [this, &document_to_relevance_](std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    document_to_relevance_[document_id].ref_to_value += term_freq * inverse_document_freq;
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
// Версия для работы без предиката
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&,
    const SearchServer::VecQueryWSD& query) const {
    std::map<int, double> document_to_relevance;
    
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
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