#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetQueryNumber() const {
    if (GetQuerySize() == 0) {
        return query_start_number;
    }
    else {
        auto last_element = requests_[GetQuerySize() - 1];
        return last_element.request_numer_ + 1;
    }
}

int RequestQueue::GetNoResultRequests() const {
    int result = 0;
    for (const auto& element : requests_) {
        if (element.no_result_) {
            result++;
        }
    }
    return result;
}

std::pair<std::string, std::vector<Document>> RequestQueue::GetResultByRequestNumber(int request_number) const {
    std::pair<std::string, std::vector<Document>> result = {};
    for (const auto& req : requests_) {
        if (req.request_numer_ == request_number) {
            std::string request = req.query_;
            std::vector<Document> request_result = req.result_;
            result = { request, request_result };
        }
    }
    return result;
}

void RequestQueue::PrintResultByHistoryNumber(int request_number) {
    const auto& history_result = RequestQueue::GetResultByRequestNumber(request_number);
    if (history_result.first.empty() && history_result.second.empty()) {
        std::cout << "R# : " << request_number << " : is not found" << std::endl;
    }
    else {
        std::cout << "R# : " << request_number << " : Request text : " 
            << history_result.first << std::endl << history_result.second << std::endl;
    }
   
}

RequestQueue::QueryResult::QueryResult() = default;

RequestQueue::QueryResult::QueryResult(int req_n, std::string query, bool res, std::vector<Document> vec_result)
    : request_numer_(req_n), query_(query), no_result_(res), result_(vec_result) {
}


