#include "document.h"

Document::Document() = default;
Document::Document(int id, double relevance, int rating) : id(id), relevance(relevance), rating(rating) 
{
}

std::ostream& operator<<(std::ostream& output, Document doc) {
	using namespace std;
	output << "{ document_id = "s << doc.id << ", relevance = "s << doc.relevance << ", rating = "s << doc.rating << " }"s;
	return output;
}