//Вставьте сюда своё решение из урока «‎Очередь запросов».‎
#pragma once
#include <stack>
#include <vector>
#include "document.h"
#include "search_server.h"

class RequestQueue {
    const SearchServer& search_server_;
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {

        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);

        if (GetQuerySize() < min_in_day_) {
            if (result.empty()) {
                requests_.push_back({ GetQueryNumber(), raw_query, true, result });
            }
            else {
                requests_.push_back({ GetQueryNumber(), raw_query, false, result });
            }
        }
        else {
            requests_.pop_front();
            if (result.empty()) {
                requests_.push_back({ GetQueryNumber(), raw_query, true, result });
            }
            else {
                requests_.push_back({ GetQueryNumber(), raw_query, false, result });
            }
        }
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    size_t GetQuerySize() const {
        return requests_.size();
    }

    int GetQueryNumber() const;

    int GetNoResultRequests() const; // выдает количество безрезультативных запросов

    std::pair <std::string, std::vector<Document>> GetResultByRequestNumber(int request_number) const; // достаёт результат запроса из памяти

    void PrintResultByHistoryNumber(int request_number);

private:
    struct QueryResult {
        QueryResult();

        QueryResult(int req_n, std::string query, bool res, std::vector<Document> vec_result);

        //система хранит 1440 запроса в памяти
        int request_numer_ = 0;              // хранит номер запроса 
        std::string query_;                  // строка запроса
        bool no_result_ = true;              // был ли результат
        std::vector<Document> result_ = {};  // результат запроса
    };
    std::deque<QueryResult> requests_;
    const static int query_start_number = 1;
    const static int min_in_day_ = 1440;
};

