#pragma once
#include <optional>
#include <stdexcept>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>
#include <atomic>
#include "read_input_functions.h" // функции ввода данных
#include "document.h" // структура документ
#include "paginator.h" // постраничный вывод
#include "search_server.h" // поисковый сервер
#include "request_queue.h" // сервер запросов
#include "log_duration.h" // профилировщик
#include "concurrent_map.h" // параллельная мапа
#include "process_queries.h" // система параллельной обработки запросов
#include "remove_duplicates.h" // функционал поиска и удаления дубликатов
#include "test_example_functions.h" // модули тестовых запусков через try/catch


using namespace std;

// ======================= БЛОК ГЕНЕРАТОРОВ =============================

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution<int>('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int max_word_count) {
    const int word_count = uniform_int_distribution(1, max_word_count)(generator);
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

string GenerateQueryWMinus(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

// ======================= БЛОК ТЕСТ_ФУНКЦИЙ ============================

template <typename QueriesProcessor>
void Test(string_view mark, QueriesProcessor processor, const SearchServer& search_server, const vector<string>& queries) {
    LOG_DURATION(mark);
    const auto documents_lists = processor(search_server, queries);
}

template <typename ExecutionPolicy>
void RDTest(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }
    std::cout << "SearchServer has |" << search_server.GetDocumentCount() << "| documents" << std::endl;
}

template <typename ExecutionPolicy>
void MDTest(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << word_count << endl;
}

template <typename ExecutionPolicy>
void FTest(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}

#define TEST(processor) Test(#processor, processor, search_server, queries)

#define RDTEST(mode) RDTest(#mode, search_server, execution::mode)

#define MDTEST(policy) MDTest(#policy, search_server, query, execution::policy)

#define FTEST(policy) FTest(#policy, search_server, queries, execution::policy)


template <typename Container, typename Predicate>
vector<typename Container::value_type> CopyIfUnordered(const Container& container,
    Predicate predicate) {

    size_t SIZE = container.size();

    vector<typename Container::value_type> result(SIZE);
    atomic_int unit_place_ = 0;


    std::for_each(std::execution::par, container.begin(), container.end(),
        [predicate, &result, &unit_place_](const auto& value) {
            if (predicate(value)) {

                {
                    result[unit_place_++] = value;
                }
            }
        });
    result.resize(unit_place_);
    return result;
}

template <typename Container>
vector<typename Container::value_type> CopyUnordered(const Container& container) {

    size_t SIZE = container.size();

    vector<typename Container::value_type> result(SIZE);
    atomic_int unit_place_ = 0;


    std::for_each(std::execution::par, container.begin(), container.end(),
        [&result, &unit_place_](const auto& value) {
            result[unit_place_++] = value;
        });
    result.resize(unit_place_);
    return result;
}

// ===================== БЛОК ОСНОВНЫХ ТЕСТОВ ===========================

void QueueTests() { 
    
    std::cout << "--------------- Queue testing in progress ---------------" << std::endl << std::endl;

    {
        SearchServer search_server("and in at"s);
        RequestQueue request_queue(search_server);

        search_server.AddDocument(1, "curly cat curly tail", DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "curly dog and fancy collar", DocumentStatus::ACTUAL, { 1, 2, 3 });
        search_server.AddDocument(3, "big cat fancy collar", DocumentStatus::ACTUAL, { 1, 2, 8 });
        search_server.AddDocument(4, "big dog sparrow Eugene", DocumentStatus::ACTUAL, { 1, 3, 2 });
        search_server.AddDocument(5, "big dog and sparrow Vasiliy", DocumentStatus::ACTUAL, { 1, 1, 1 });
        search_server.AddDocument(6, "dog big sparrow in Vasiliy", DocumentStatus::ACTUAL, { 1, 1, 1 });
        search_server.AddDocument(7, "dog horry sparrow Vasiliy", DocumentStatus::ACTUAL, { 1, 2, 3 });

        {

            LogDuration timer("Operation time", std::cout);
            FindTopDocuments(search_server, "big -Eugene"s);
        }
        std::cout << std::endl;
        {
            LOG_DURATION("big");
            MatchDocuments(search_server, "big -Eugene"s);
        }
        std::cout << std::endl;
        RemoveDuplicates(search_server);
        search_server.RemoveDocument(3);

        std::vector<Document> result = request_queue.AddFindRequest("big cat -Eugene"s);
        for (const auto& element : result) {
            PrintDocument(element);
        }

        std::cout << std::endl;

        int page_size = 1;
        const auto pages = Paginate(result, page_size);

        for (auto page = pages.begin(); page != pages.end(); ++page) {
            std::cout << *page << std::endl;
            std::cout << "Page end"s << std::endl;
        }
        std::cout << std::endl;
        const auto search_res = search_server.FindTopDocuments("big cat -Eugene"s);

        // 1439 запросов с нулевым результатом
        for (int i = 0; i < 1439; ++i) {
            request_queue.AddFindRequest("empty request");
        }
        // все еще 1439 запросов с нулевым результатом
        request_queue.AddFindRequest("curly dog");
        // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
        request_queue.AddFindRequest("big collar");
        // первый запрос удален, 1437 запросов с нулевым результатом
        request_queue.AddFindRequest("sparrow");
        std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl << std::endl;

        int result_history_number = 1440;

        request_queue.PrintResultByHistoryNumber(result_history_number);
        request_queue.PrintResultByHistoryNumber(1534);
        std::cout << std::endl;
    }
    
    std::cout << "----------------- Queue testing complete ----------------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void DuplicatesTests() {

    std::cout << "------------- Duplicates testing in progress ------------" << std::endl << std::endl;

    {
        SearchServer search_server("and with"s);

        AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // дубликат документа 2, будет удалён
        AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // отличие только в стоп-словах, считаем дубликатом
        AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // множество слов такое же, считаем дубликатом документа 1
        AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // добавились новые слова, дубликатом не является
        AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
        AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

        // есть не все слова, не является дубликатом
        AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // слова из разных документов, не является дубликатом
        AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });


        std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
        RemoveDuplicates(search_server);

        std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
        std::cout << std::endl;
    }
    
    std::cout << "--------------- Duplicates testing complete -------------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void ProcessQueriesTest() {

    std::cout << "---------- Process Queries testing in progress ----------" << std::endl << std::endl;

    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        id = 0;
        
        {
            LOG_DURATION("Выполнение");
            std::cout << "---------------- Оптимизированное решение ---------------" << std::endl;
            for (
                const auto& documents : ProcessQueries(search_server, queries)
                ) {
                std::cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
            }
        }
        std::cout << std::endl;
        
        id = 0;

        {
            LOG_DURATION("Выполнение");
            std::cout << "------------------- Тривиальное решение -----------------" << std::endl;
            vector<vector<Document>> documents_lists;
            for (const std::string& query : queries) {
                documents_lists.push_back(search_server.FindTopDocuments(query));
            }

            for (const auto& documents : documents_lists) {
                std::cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "------------ Process Queries testing complete -----------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void BenchMark_QueriesTest() {

    std::cout << "--------- BenchMark Queries testing in progress ---------" << std::endl << std::endl;

    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 2'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);

        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }

        const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
        //TEST(ProcessQueries);
        int id = 0;
        {
            LOG_DURATION("Выполнение");
            std::cout << "---------------- Оптимизированное решение ---------------" << std::endl;
            vector<vector<Document>> documents_lists = ProcessQueries(search_server, queries);
            
        }
        std::cout << std::endl;

        id = 0;
        {
            LOG_DURATION("Выполнение");
            std::cout << "------------------- Тривиальное решение -----------------" << std::endl;
            vector<vector<Document>> documents_lists;
            for (const std::string& query : queries) {
                documents_lists.push_back(search_server.FindTopDocuments(query));
            }
        }
        std::cout << std::endl;
    }

    std::cout << "----------- BenchMark Queries testing complete ----------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void ProcessQueriesJoinedTest() {

    std::cout << "------ Process Queries Joined testing in progress -------" << std::endl << std::endl;

    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        id = 0;

        {
            LOG_DURATION("Выполнение");
            std::cout << "---------------- Оптимизированное решение ---------------" << std::endl;

            for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
                std::cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
            }
        }
        std::cout << std::endl;

        id = 0;

        {
            LOG_DURATION("Выполнение");
            std::cout << "------------------- Тривиальное решение -----------------" << std::endl;
            vector<vector<Document>> documents_lists;
            for (const std::string& query : queries) {
                documents_lists.push_back(search_server.FindTopDocuments(query));
            }

            for (const auto& documents : documents_lists) {
                for (const auto& document : documents) {
                    std::cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
                }
            }
        }
        std::cout << std::endl;
    }

    
    std::cout << "-------- Process Queries Joined testing complete --------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;

}

void BenchMark_QueriesJoinedTest() {

    std::cout << "----- BenchMark Queries Joined testing in progress ------" << std::endl << std::endl;

    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 2'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);

        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }

        const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
        //TEST(ProcessQueries);
        int id = 0;
        {
            LOG_DURATION("Выполнение");
            std::cout << "---------------- Оптимизированное решение ---------------" << std::endl;
            vector<Document> documents_lists = ProcessQueriesJoined(search_server, queries);

        }
        std::cout << std::endl;

        id = 0;
        {
            LOG_DURATION("Выполнение");
            std::cout << "------------------- Тривиальное решение -----------------" << std::endl;
            vector<vector<Document>> documents_lists;
            for (const std::string& query : queries) {
                documents_lists.push_back(search_server.FindTopDocuments(query));
            }

            vector<Document> result;

            for (const auto& documents : documents_lists) {
                for (const auto& document : documents) {
                    result.push_back(document);
                }
            }

        }
        std::cout << std::endl;
    }

    std::cout << "------- BenchMark Queries Joined testing complete -------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void RemoveDocumentsTest() {

    std::cout << "---------- RemoveDocuments testing in progress ----------" << std::endl << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        std::cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(execution::par, 2);
    report();
    std::cout << std::endl;
    std::cout << "------------ RemoveDocuments testing complete -----------" << std::endl;
    std::cout << "------------------------- Done --------------------------" << std::endl << std::endl << std::endl;
}

void BenchMark_RemoveDocumentsTest() {
    {
        std::cout << "----- BenchMark RemoveDocuments testing in progress -----" << std::endl << std::endl;
    
        mt19937 generator;

        int word_count_ = 10000; // 10000
        int word_max_lenght_ = 25; // 25

        int query_count_ = 10000; // 10000
        int max_word_count_ = 100; // 100


        std::cout << "--- Total Words Count = " << word_count_ 
            << " | Word Max Lenght = " << word_max_lenght_ << " ----" << std::endl;

        std::cout << "--- Total Query Count = " << query_count_ 
            << " | Max Word Count = " << max_word_count_ << " ----" << std::endl << std::endl;

        const auto dictionary = GenerateDictionary(generator, word_count_/*10'000*/, word_max_lenght_/*25*/);
        const auto documents = GenerateQueries(generator, dictionary, query_count_/*10'000*/, max_word_count_/*100*/);


        {
            SearchServer search_server(dictionary[0]);
            for (size_t i = 0; i < documents.size(); ++i) {
                search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
            }
 
            std::cout << "------ Sequence Policy RemoveDocuments in progress ------" << std::endl;
            RDTEST(seq);
            std::cout << std::endl;
        }
        {
            SearchServer search_server(dictionary[0]);
            for (size_t i = 0; i < documents.size(); ++i) {
                search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
            }
            std::cout << "------ Parallel Policy RemoveDocuments in progress ------" << std::endl;
            RDTEST(par);
            std::cout << std::endl;
        }

        
        std::cout << "------- BenchMark RemoveDocuments testing complete ------" << std::endl;
        std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
    }
}

void ParseQueryTest() {
    std::cout << "--------------- ParseQuery testing in progress ----------" << std::endl << std::endl;

    SearchServer search_server("and with"s);
    std::string query = "funny and cute funny cat ruby cat funny funny -dog";
    auto simple_query_result = search_server.QueryTest(query);
    auto vector_nosd_query_result = search_server.VectorNoSDQueryTest(query);
    auto vector_wsd_query_result = search_server.VectorWSDQueryTest(query);

    std::cout << "Query is : \"" << query << "\"" << std::endl << std::endl;
    std::cout << "Simple Query container type is { std::set } auto(sorted deduplicated)" << std::endl;
    std::cout << "Plus/Minus Words massive has : {";
    for (auto& item : simple_query_result.plus_words) {
        std::cout << " " << item;
    }
    std::cout << " } | {";;

    for (auto& item : simple_query_result.minus_words) {
        std::cout << " " << item;
    }
    std::cout << " }" << std::endl << std::endl;

    std::cout << "VectorNoSD Query container type is { std::vector } unsorted duplicated" << std::endl;
    std::cout << "Plus/Minus Words massive has : {";
    for (auto& item : vector_nosd_query_result.plus_words) {
        std::cout << " " << item;
    }
    std::cout << " } | {";;

    for (auto& item : vector_nosd_query_result.minus_words) {
        std::cout << " " << item;
    }
    std::cout << " }" << std::endl << std::endl;

    std::cout << "VectorWCD Query container type is { std::vector } sorted deduplicated" << std::endl;
    std::cout << "Plus/Minus Words massive has : {";
    for (auto& item : vector_wsd_query_result.plus_words) {
        std::cout << " " << item;
    }
    std::cout << " } | {";;

    for (auto& item : vector_wsd_query_result.minus_words) {
        std::cout << " " << item;
    }
    std::cout << " }" << std::endl << std::endl;

    std::cout << std::endl; 
    std::cout << "----------------- ParseQuery testing complete -----------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void BenchMark_ParseQueryTest() {

    std::cout << "-------- BenchMark ParseQuery testing in progress -------" << std::endl << std::endl;

    SearchServer search_server("and with"s);

    int word_count_ = 5000; //5000
    int word_max_lenght_ = 25; //25

    int query_count_ = 3000; //5000
    int max_word_count_ = 9999; //5000

    std::cout << "--- Total Words Count = " << word_count_
        << " | Word Max Lenght = " << word_max_lenght_ << " -----" << std::endl;

    std::cout << "--- Total Query Count = " << query_count_
        << " | Max Word Count = " << max_word_count_ << " ----" << std::endl << std::endl;

    mt19937 generator;
    auto dictionary = GenerateDictionary(generator, word_count_, word_max_lenght_);

    //создаем минус-слова
    int i = 0; //счетчик
    for (auto& word : dictionary) {
        if (i == (rand() % 4) || i == 25) {
            if (word.size() <= 1) continue;
            word[0] = '-';
            i = 0;
        }
        i++;
    }

    const auto queries = GenerateQueries(generator, dictionary, query_count_, max_word_count_);

    {
        LOG_DURATION("Sequence Standart Query W/R Plus Words");
        for (const auto& query : queries) {
            auto struct_query = search_server.QueryTest(query);
            auto plus_words_ = CopyUnordered(struct_query.plus_words);
        }
    }

    {
        LOG_DURATION("Sorted deduplicated New Query W/R Plus Words");
        for (const auto& query : queries) {
            auto struct_par_query = search_server.VectorWSDQueryTest(query);
            auto plus_words_ = CopyUnordered(struct_par_query.plus_words);
        }
    }

    {
        LOG_DURATION("Unsorted duplicated New Query W/R Plus Words");
        for (const auto& query : queries) {
            auto struct_par_query = search_server.VectorNoSDQueryTest(query);
            auto plus_words_ = CopyUnordered(struct_par_query.plus_words);
        }
    }
 
    std::cout << std::endl;
    std::cout << "---------- BenchMark ParseQuery testing complete --------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void MatchDocumentsTest() {

    std::cout << "----------- MatchDocuments testing in progress ----------" << std::endl << std::endl;

    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }

    const string query = "curly and funny funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        std::cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        std::cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        std::cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }

    {
        const auto first = search_server.MatchDocument(query, 2);
        const auto second = search_server.MatchDocument(execution::seq, query, 2);
        const auto third = search_server.MatchDocument(execution::par, query, 2);
        std::cout << endl;
    }

    std::cout << std::endl;
    std::cout << "------------- MatchDocuments testing complete -----------" << std::endl;
    std::cout << "------------------------- Done --------------------------" << std::endl << std::endl << std::endl;
}

void BenchMark_MatchDocumentsTest() {

    std::cout << "------ BenchMark MatchDocuments testing in progress -----" << std::endl << std::endl;

    int word_count_ = 1000; //1000
    int word_max_lenght_ = 10; //10

    int query_count_ = 10000; //10000
    int max_word_count_ = 70; //70

    int query_word_count_ = 500; //500
    double minus_prob_ratio_ = 0.1; //0.1

    std::cout << "--- Total Words Count = " << word_count_
        << " | Word Max Lenght = " << word_max_lenght_ << " -----" << std::endl;

    std::cout << "--- Total Query Count = " << query_count_
        << " | Max Word Count = " << max_word_count_ << " -----" << std::endl;

    std::cout << "-- Total Word in Query = " << query_word_count_
        << " | Minus_Probe Ratio = " << minus_prob_ratio_ << " --" << std::endl << std::endl;

    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, word_count_, word_max_lenght_);
    const auto documents = GenerateQueries(generator, dictionary, query_count_, max_word_count_);

    const string query = GenerateQueryWMinus(generator, dictionary, query_word_count_, minus_prob_ratio_);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    std::cout << "------- Sequence Policy MatchDocuments in progress ------" << std::endl;
    MDTEST(seq);
    std::cout << std::endl;

    std::cout << "------- Parallel Policy MatchDocuments in progress ------" << std::endl;
    MDTEST(par);
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "-------- BenchMark MatchDocuments testing complete ------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;

}

void StringViewTest() {
    std::cout << "---------- StringView System testing in progress --------" << std::endl << std::endl;

    {
        std::cout << "SearchServer Constructors check" << std::endl;
        // тестируем создание сервера различными вариациями строк
        std::string stop_words_s = "not but and or";
        std::string_view stop_words_sv = stop_words_s;


        SearchServer server_s(stop_words_s);
        SearchServer server_s2("not but and or"s);

        SearchServer server_sv(stop_words_sv);
        SearchServer server_sv2("not but and or"sv);
    }

    {
        // тестируем создание сервера контейнерами строк и string_view
        std::string a = "not"; std::string b = "or"; 
        std::string c = "and"; std::string d = "but";
        std::string e = "and"; std::string f = "but";

        std::string_view a_sv = a; std::string_view b_sv = b; 
        std::string_view c_sv = c; std::string_view d_sv = d;
        std::string_view e_sv = e; std::string_view f_sv = f;

        {
            std::vector<std::string> stop_words = { "not", "or", "and", "but" };
            SearchServer server(stop_words);

            assert(server.GetCurrentStopWords().size() == 4);
            assert(server.GetCurrentStopWords().count(a) == 1);
            assert(server.GetCurrentStopWords().count(b) == 1);
            assert(server.GetCurrentStopWords().count(c) == 1);
            assert(server.GetCurrentStopWords().count(d) == 1);
        }
        
        {
            std::vector<std::string> stop_words = { a, b, c, d };
            SearchServer server(stop_words);

            assert(server.GetCurrentStopWords().size() == 4);
            assert(server.GetCurrentStopWords().count(a) == 1);
            assert(server.GetCurrentStopWords().count(b) == 1);
            assert(server.GetCurrentStopWords().count(c) == 1);
            assert(server.GetCurrentStopWords().count(d) == 1);
        }

        {
            std::vector<std::string_view> stop_words = { "not", "or", "and", "but" };
            SearchServer server(stop_words);

            assert(server.GetCurrentStopWords().size() == 4);
            assert(server.GetCurrentStopWords().count(a) == 1);
            assert(server.GetCurrentStopWords().count(b) == 1);
            assert(server.GetCurrentStopWords().count(c) == 1);
            assert(server.GetCurrentStopWords().count(d) == 1);
        }

        {
            std::vector<std::string_view> stop_words = { a, b, c, d };
            SearchServer server(stop_words);
            
            assert(server.GetCurrentStopWords().size() == 4);
            assert(server.GetCurrentStopWords().count(a) == 1);
            assert(server.GetCurrentStopWords().count(b) == 1);
            assert(server.GetCurrentStopWords().count(c) == 1);
            assert(server.GetCurrentStopWords().count(d) == 1);
        }

        {
            std::vector<std::string_view> stop_words = { a_sv, b_sv, c_sv, d_sv };
            SearchServer server(stop_words);

            assert(server.GetCurrentStopWords().size() == 4);
            assert(server.GetCurrentStopWords().count(a) == 1);
            assert(server.GetCurrentStopWords().count(b) == 1);
            assert(server.GetCurrentStopWords().count(c) == 1);
            assert(server.GetCurrentStopWords().count(d) == 1);
        }

        {
            std::vector<std::string_view> stop_words = { a, b, c, d, e, f };
            SearchServer server(stop_words);

            assert(server.GetCurrentStopWords().size() == 4);
        } 

        std::cout << "SearchServer Constructors check complete. Done" << std::endl << std::endl;
    }

    {
        // тестируем добавление документов
        std::cout << "SearchServer AddDocument check" << std::endl << std::endl;
        std::cout << "{" << std::endl;

        std::string stop_words_s = "not but and or";
        std::string_view stop_words_sv = stop_words_s;
        SearchServer search_server(stop_words_sv);
        std::string text_1 = "good day today"s;
        std::string_view text_2 = text_1;

        std::cout << "   AddDocument, id: 1, text: {\"" << "no rain day good night" << "\"} text type - std::string" << std::endl;
        search_server.AddDocument(1, "no rain day good night", DocumentStatus::ACTUAL, { 1, 2 });
        std::cout << "   AddDocument 1. Done" << std::endl << std::endl;

        std::cout << "   AddDocument, id: 2, text: {\"" << text_2 << "\"} text type - std::string_view" << std::endl;
        AddDocument(search_server, 2, text_2, DocumentStatus::ACTUAL, { 1, 2 });

        std::cout << "   AddDocument 2. Done" << std::endl << std::endl;

        std::cout << "   AddDocument, id: 1, text: {\"" << " good night"s << "\"} text type - std::string" << std::endl;
        std::cout << "   ";
        AddDocument(search_server, 1, " good night"s, DocumentStatus::ACTUAL, { 1, 2 });
        std::cout << std::endl;

        std::cout << "   AddDocument, id: 3, text: {\"" << " %agf\1good night"sv << "\"} text type - std::string_view" << std::endl;
        std::cout << "   ";
        AddDocument(search_server, 3, " %agf\1good night"sv, DocumentStatus::ACTUAL, { 1, 2 });
        std::cout << std::endl;

        assert(search_server.GetDocumentCount() == 2);

        std::cout << "   Total Document on Server is: " << search_server.GetDocumentCount() << " documents" << std::endl;

        {
            std::cout << "   Creat new SearchServer mass_docs_server" << std::endl;
            SearchServer mass_docs_server(stop_words_s);

            for (int i = 0; i != 2500; i++) {
                mass_docs_server.AddDocument(i, "Здесь лежит этот текст", DocumentStatus::ACTUAL, { 1, 2 });
            }

            int docs_cout = mass_docs_server.GetDocumentCount();
            for (int i = 0; i != docs_cout; i++) {
                mass_docs_server.RemoveDocument(std::execution::par, i);
            }
        }
        

        std::cout << "}" << std::endl << std::endl;
        std::cout << "SearchServer AddDocument check complete. Done" << std::endl << std::endl;
    }

    std::string_view stop_words = "not but and or in the";
    SearchServer search_server(stop_words);

    std::vector<std::string> documents = {
        {"funny lazy cat tom", "crazy nut mouse jerry", "funny unlucky dog spike", "grey little mouse tuffy",
         "son of spike tike", "serious black cat butch", "white cute cat pussy", "lazy yellow canaria bird"}
    };

    std::cout << "   Create New SearchServer: " << std::endl;
    std::cout << "      SearchServer stop_words {\"" << stop_words << "}" << std::endl << std::endl;
    std::cout << "   Add Documents: " << std::endl;
    std::cout << "   {" << std::endl;
    for (int i = 0; i != documents.size(); i++) {
        if (i == documents.size() - 1) {
            std::cout << "      \"" << documents.at(i) << "\"" << std::endl;
            continue;
        }
        std::cout << "      \"" << documents.at(i) << "\"," << std::endl;
    }
    std::cout << "   }" << std::endl << std::endl;
    int docs_id = 0;
    for (auto& item : documents) {
        AddDocument(search_server, docs_id++, item, DocumentStatus::ACTUAL, { 2, 1, 10 });
    }

    std::cout << "   Total Document on Server is: "
        << search_server.GetDocumentCount() << " documents" << std::endl << std::endl;

    {
        // тестируем поиск документов
        std::cout << "SearchServer FindDocument check" << std::endl << std::endl;
        std::cout << "{" << std::endl;

        {
            auto result = search_server.FindTopDocuments("cat"sv);
            std::cout << "   Result by request: \"" << "cat" << "\". Request type - std::string_view" << std::endl;
            for (auto& item : result) {
                std::cout << "   " << item << std::endl;
            }
            std::cout << std::endl;
        }
        
        {
            auto result = search_server.FindTopDocuments("cat -tom"s);
            std::cout << "   Result by request: \"" << "cat -tom" << "\". Request type - std::string" << std::endl;
            for (auto& item : result) {
                std::cout << "   " << item << std::endl;
            }
            std::cout << std::endl;
        }

        {
            std::string request = "the lazy -cat";
            std::string_view request_sv = request;
            auto result = search_server.FindTopDocuments(request_sv);
            std::cout << "   Result by request: \"" << request_sv << "\". Request type - std::string_view" << std::endl;
            for (auto& item : result) {
                std::cout << "   " << item << std::endl;
            }
            std::cout << std::endl;
        }

        {
            std::string request = "funny good dog -cat";
            auto result = search_server.FindTopDocuments(request);
            std::cout << "   Result by request: \"" << request << "\". Request type - std::string" << std::endl;
            for (auto& item : result) {
                std::cout << "   " << item << std::endl;
            }
            std::cout << std::endl;
        }

        {
            std::string request = "fresh fish in the pool";
            std::string_view request_sv = request;
            auto result = search_server.FindTopDocuments(request_sv);
            std::cout << "   Result by request: \"" << request << "\". Request type - std::string" << std::endl;
            for (auto& item : result) {
                std::cout << "   " << item << std::endl;
            }
            if (result.size() == 0) {
                std::cout << "   No results by this request" << std::endl;
            }
            std::cout << std::endl;
        }

        {
            std::string request = "fresh fish \1in the pool";
            std::string_view request_sv = request;
            FindTopDocuments(search_server, request_sv);
        }
        std::cout << "}" << std::endl << std::endl;
        std::cout << "SearchServer FindDocument check complete. Done" << std::endl << std::endl;
    }
    
    {
        // тестируем матчинг документов
        std::cout << "SearchServer MatchDocument check" << std::endl << std::endl;
        std::cout << "{" << std::endl;

        {
            std::string query = "cute cat tom"s;
            std::string_view query_sv = query;
            std::cout << "   Matching by request: \"" << query_sv << "\". Request type - std::string_view" << std::endl;
            for (auto it = search_server.begin(); it != search_server.end(); it++) {
                const int document_id = *it;
                const auto [words, status] = search_server.MatchDocument(query_sv, document_id);
                
                std::cout << "   { " << "document_id = " << document_id << ", " 
                    << "status = " << static_cast<int>(status) << ", "
                    << "words =";
                for (std::string_view word : words) {
                    std::cout << ' ' << word;
                }
                std::cout << "}" << std::endl;
            }

            std::cout << std::endl;
        }

        {
            std::string query = "cute lazy grey mouse"s;
            std::string_view query_sv = query;
            std::cout << "   Matching by request: \"" << "cute lazy grey mouse" << "\". Request type - std::string" << std::endl;
            for (auto it = search_server.begin(); it != search_server.end(); it++) {
                const int document_id = *it;
                const auto [words, status] = search_server.MatchDocument(std::execution::par, "cute lazy grey mouse", document_id);

                std::cout << "   { " << "document_id = " << document_id << ", "
                    << "status = " << static_cast<int>(status) << ", "
                    << "words =";
                for (std::string_view word : words) {
                    std::cout << ' ' << word;
                }
                std::cout << "}" << std::endl;
            }

            //std::cout << std::endl;
        }

        std::cout << "}" << std::endl << std::endl;
        std::cout << "SearchServer MatchDocument check complete. Done" << std::endl << std::endl;
    }

    {
        // тестируем удаление документов
        std::cout << "SearchServer RemoveDocument check" << std::endl << std::endl;
        std::cout << "{" << std::endl;
        std::cout << "   RemoveDocumen id: \"" << "1" << "\". Request type - standart" << std::endl;
        search_server.RemoveDocument(1);
        assert(search_server.GetDocumentCount() == 7);
        std::cout << "   Total Document on Server is: "
            << search_server.GetDocumentCount() << " documents" << std::endl << std::endl;

        std::cout << "   RemoveDocumen id: \"" << "3" << "\". Request type - std::execution::seq" << std::endl;
        search_server.RemoveDocument(std::execution::seq, 3);
        assert(search_server.GetDocumentCount() == 6);
        std::cout << "   Total Document on Server is: "
            << search_server.GetDocumentCount() << " documents" << std::endl << std::endl;

        std::cout << "   RemoveDocumen id: \"" << "5" << "\". Request type - std::execution::par" << std::endl;
        search_server.RemoveDocument(std::execution::par, 5);
        assert(search_server.GetDocumentCount() == 5);
        std::cout << "   Total Document on Server is: "
            << search_server.GetDocumentCount() << " documents" << std::endl << std::endl;
        const auto server_docs = search_server.GetDocumentMassive();
        std::cout << "   SearchServer has: \"" << "5" << "\". documents" << std::endl;

        for (const auto& item : server_docs) {
            std::cout << "      { document id: " << item.first << ", document text: \"";
            for (const auto& text : item.second) {
                if (text == *item.second.begin()) {
                    std::cout << text.first;
                }
                else {
                    std::cout << " " << text.first;
                }
            }
            std::cout << "\" }" << std::endl;
        }

        std::cout << "}" << std::endl << std::endl;
        std::cout << "SearchServer RemoveDocument check complete. Done" << std::endl << std::endl;
    }

    std::cout << std::endl;
    std::cout << "------------ StringView System testing complete ---------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void FindDocumentsTest() {
    std::cout << "------------ FindDocuments testing in progress ----------" << std::endl << std::endl;

    // Yandex Test
    {
        std::string stop_words = "and with"s;
        std::vector<std::string> documents = { "white cat and yellow hat"s,
            "curly cat curly tail"s, "nasty dog with big eyes"s, "nasty pigeon john"s };

        std::cout << "   Create New SearchServer: " << std::endl;
        std::cout << "      SearchServer stop_words {\"" << stop_words << "\"}" << std::endl << std::endl;
        
        SearchServer search_server(stop_words);
       
        std::cout << "   Add Documents: " << std::endl;
        std::cout << "   {" << std::endl;
        for (int i = 0; i != documents.size(); i++) {
            if (i == documents.size() - 1) {
                std::cout << "      \"" << documents.at(i) << "\"" << std::endl;
                continue;
            }
            std::cout << "      \"" << documents.at(i) << "\"," << std::endl;
        }
        std::cout << "   }" << std::endl << std::endl;


        int id = 0;
        for (const string& text : documents) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        std::cout << "   Total Document on Server is: "
            << search_server.GetDocumentCount() << " documents" << std::endl << std::endl;


        cout << "ACTUAL by default:"s << endl;
        // последовательная версия
        for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
            PrintDocument(document);
        }
        cout << "BANNED:"s << endl;
        // последовательная версия
        for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
            PrintDocument(document);
        }

        cout << "Even ids:"s << endl;
        // параллельная версия
        for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
            PrintDocument(document);
        }
    }



    std::cout << std::endl;
    std::cout << "-------------- FindDocuments testing complete -----------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}

void BenchMark_FindDocumentsTest() {
    std::cout << "------ BenchMark FindDocuments testing in progress ------" << std::endl << std::endl;
    

    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueries(generator, dictionary, 100, 70);

    FTEST(seq);
    FTEST(par);


    std::cout << std::endl;
    std::cout << "-------- BenchMark FindDocuments testing complete -------" << std::endl;
    std::cout << "-------------------------- Done -------------------------" << std::endl << std::endl << std::endl;
}