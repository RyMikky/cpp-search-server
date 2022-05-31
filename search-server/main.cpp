// search_server v1_s4_my_realisation

#include <optional>
#include <stdexcept>
#include <vector>
#include "read_input_functions.h" // функции ввода данных
#include "document.h" // структура документ
#include "paginator.h" // постраничный вывод
#include "search_server.h" // поисковый сервер
#include "request_queue.h" // сервер запросов
#include "log_duration.h" // профилировщик
#include "remove_duplicates.h" // функционал поиска и удаления дубликатов
#include "test_example_functions.h" // модули тестирования
#include "locale.h"

using namespace std;

int main() {
    setlocale(LC_ALL, "Russian");

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

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicatesTwo(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;

    

    // прошлые тестовые материалы
    //SearchServer search_server("and in at"s);
    //RequestQueue request_queue(search_server);

    //search_server.AddDocument(1, "curly cat curly tail", DocumentStatus::ACTUAL, { 7, 2, 7 });
    //search_server.AddDocument(2, "curly dog and fancy collar", DocumentStatus::ACTUAL, { 1, 2, 3 });
    //search_server.AddDocument(3, "big cat fancy collar", DocumentStatus::ACTUAL, { 1, 2, 8 });
    //search_server.AddDocument(4, "big dog sparrow Eugene", DocumentStatus::ACTUAL, { 1, 3, 2 });
    //search_server.AddDocument(5, "big dog and sparrow Vasiliy", DocumentStatus::ACTUAL, { 1, 1, 1 });
    //search_server.AddDocument(6, "dog big sparrow in Vasiliy", DocumentStatus::ACTUAL, { 1, 1, 1 });
    //search_server.AddDocument(7, "dog horry sparrow Vasiliy", DocumentStatus::ACTUAL, { 1, 2, 3 });

    //{

    //    LogDuration timer("Operation time", std::cout);
    //    FindTopDocuments(search_server, "big -Eugene"s);
    //}
    //std::cout << std::endl;
    //{
    //    LOG_DURATION_STREAM("big");
    //    MatchDocuments(search_server, "big -Eugene"s);
    //}
    //std::cout << std::endl;
    //RemoveDuplicates(search_server);
    //search_server.RemoveDocument(3);

    //std::vector<Document> result = request_queue.AddFindRequest("big cat -Eugene"s);
    //for (const auto& element : result) {
    //    PrintDocument(element);
    //}

    //std::cout << std::endl;

    //int page_size = 1;
    //const auto pages = Paginate(result, page_size);

    //for (auto page = pages.begin(); page != pages.end(); ++page) {
    //    std::cout << *page << std::endl;
    //    std::cout << "Page end"s << std::endl;
    //}
    //std::cout << std::endl;
    //const auto search_res = search_server.FindTopDocuments("big cat -Eugene"s);

    //// 1439 запросов с нулевым результатом
    //for (int i = 0; i < 1439; ++i) {
    //    request_queue.AddFindRequest("empty request");
    //}
    //// все еще 1439 запросов с нулевым результатом
    //request_queue.AddFindRequest("curly dog");
    //// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    //request_queue.AddFindRequest("big collar");
    //// первый запрос удален, 1437 запросов с нулевым результатом
    //request_queue.AddFindRequest("sparrow");
    //std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl << std::endl;

    //int result_history_number = 1440;

    //request_queue.PrintResultByHistoryNumber(result_history_number);
    //request_queue.PrintResultByHistoryNumber(1534);

    return 0;
}