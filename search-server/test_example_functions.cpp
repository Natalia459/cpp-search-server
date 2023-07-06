#include "test_example_functions.h"

void AddDocument(SearchServer& search_server, int document_id, const std::string& raw_query, DocumentStatus status, const std::vector<int>& ratings) {
	using namespace std;
	std::cout << "Добавление нового документа: "s << raw_query << std::endl;
	{
		LOG_DURATION_STREAM("AddDocument"s, cout);
		search_server.AddDocument(document_id, raw_query, status, ratings);
	}
}

void MatchDocuments(SearchServer& search_server, const std::string& query) {
	using namespace std;
	std::cout << "Матчинг документов по запросу: "s << query << std::endl;
	{
		LOG_DURATION_STREAM("MatchDocument"s, cout);
		for (int document_id = 1; document_id < search_server.GetDocumentCount(); ++document_id) {
			std::tuple<std::vector<std::string>, DocumentStatus> matched_docs = search_server.MatchDocument(query, document_id);
			std::cout << "{ document_id = "s << document_id << ", "s << matched_docs << " }"s << std::endl;
		}
	}
}

void FindTopDocuments(SearchServer& search_server, const std::string& query) {
	using namespace std;
	std::cout << "Результаты поиска по запросу: "s << query << std::endl;
	std::vector<Document> found_docs;
	found_docs.reserve(static_cast<size_t>(search_server.GetDocumentCount()));
	{
		LOG_DURATION_STREAM("FindTopDocuments"s, cout);
		found_docs = search_server.FindTopDocuments(query);
	}

	for (Document& doc : found_docs) {
		std::cout << doc << std::endl;
	}
}

std::ostream& operator<<(std::ostream& output, std::tuple<std::vector<std::string>, DocumentStatus> document) {
	using namespace std;
	switch (std::get<1>(document)) {
	case DocumentStatus::ACTUAL:
		output << " status = 0, "s;
		break;
	case DocumentStatus::IRRELEVANT:
		output << " status = 1, "s;
		break;
	case DocumentStatus::BANNED:
		output << " status = 2, "s;
		break;
	case DocumentStatus::REMOVED:
		output << " status = 3, "s;
		break;
	}

	output << "word"s;
	for (const std::string& word : std::get<0>(document)) {
		output << " "s << word;
	}
	return output;
}