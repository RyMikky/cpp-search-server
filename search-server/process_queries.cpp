#include "process_queries.h"
#include <cassert>

class SimpleDocuments	
{

public:
	using InnerIterator = std::vector<Document>::iterator;
	using ConstInnerIterator = std::vector<Document>::const_iterator;

	explicit SimpleDocuments(std::vector<std::vector<Document>> input);

	~SimpleDocuments() = default;

	InnerIterator begin(size_t i) {
		assert(!IsEmpty());
		assert(i < size_);
		return documents_[i].begin();
	}

	InnerIterator end(size_t i) {
		assert(!IsEmpty());
		assert(i < size_);
		return documents_[i].end();
	}

	size_t GetSize() {
		return documents_.size();
	}

	bool IsEmpty() {
		return !size_;
	}

	void PrintAllRange() {
		for (int i = 0; i != size_; i++) {
			for (auto it = begin(i); it != end(i); it++) {
				std::cout << *it << std::endl;
			}
		}
	}

	std::vector<Document> GetDataInOneMassive() {
		std::vector<Document> result(docs_count_);
		auto start_it = result.begin();
		for (int i = 0; i != size_; i++) {
			std::transform(std::execution::par, begin(i), end(i), start_it,
				[](const Document& doc) {return doc; });
			start_it += GetDocSize(i);
		}
		return result;
	}


private:
	std::vector<std::vector<Document>> documents_;
	size_t size_ = 0;
	size_t docs_count_ = 0;

	size_t GetDocSize(int i) {
		assert(!IsEmpty());
		assert(i < size_);
		return documents_[i].size();
	}

	size_t GetDocsCount() {
		int result = 0;
		for (int i = 0; i != size_; i++) {
			result += GetDocSize(i);
		}
		return result;
	}
};

SimpleDocuments::SimpleDocuments(
	std::vector<std::vector<Document>> input) 
	: documents_(input), size_(documents_.size()), docs_count_(GetDocsCount()) {
}

std::vector<std::vector<Document>> ProcessQueries(
	const SearchServer& search_server, const std::vector<std::string>& queries) {

	std::vector<std::vector<Document>> result(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), 
		[&search_server](std::string query) {return search_server.FindTopDocuments(query); });

	return result;
}
std::vector<Document> ProcessQueriesJoined(
	const SearchServer& search_server, const std::vector<std::string>& queries) {
	
	SimpleDocuments result(ProcessQueries(search_server, queries));
	return result.GetDataInOneMassive();
}