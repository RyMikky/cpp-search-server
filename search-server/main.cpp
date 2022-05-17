//Вставьте сюда своё решение из урока «‎Очередь запросов».‎
// search_server v1_s4_my_realisation

#include <optional>
#include <stdexcept>
#include <vector>
#include "read_input_functions.h" // функции ввода данных
#include "document.h" // структура документ
#include "paginator.h" // постраничный вывод
#include "search_server.h" // поисковый сервер
#include "request_queue.h" // сервер запросов

using namespace std;

void PrintDocument(const Document& document) {
    std::cout << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }" << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "
        << "document_id = " << document_id << ", "
        << "status = " << static_cast<int>(status) << ", "
        << "words =";
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка добавления документа " << document_id << ": " << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Результаты поиска по запросу: " << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка поиска: " << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Матчинг документов по запросу: " << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка матчинга документов на запрос " << query << ": " << e.what() << std::endl;
    }
}


int main() {
    setlocale(LC_ALL, "Russian");

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail", DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar", DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar", DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene", DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy", DocumentStatus::ACTUAL, { 1, 1, 1 });


    
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
    std::cout << "Total empty requests: " << request_queue.GetNoResultRequests() << std::endl << std::endl;

    int result_history_number = 1442;

    request_queue.PrintResultByHistoryNumber(result_history_number);
    request_queue.PrintResultByHistoryNumber(1534);

    return 0;
}
