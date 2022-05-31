#include "document.h"

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }";
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<Document>& documents) {
    for (const auto& document : documents) {
        out << "{ "
            << "document_id = " << document.id << ", "
            << "relevance = " << document.relevance << ", "
            << "rating = " << document.rating << " }" << std::endl;
    }
    
    return out;
}
