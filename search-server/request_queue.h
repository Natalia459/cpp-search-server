#pragma once
#include <vector>
#include <string>
#include <deque>
#include "search_server.h"
#include "document.h"
class RequestQueue {
public:
	explicit RequestQueue(const SearchServer&);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string&, DocumentPredicate);

	std::vector<Document> AddFindRequest(const std::string&, DocumentStatus);

	std::vector<Document> AddFindRequest(const std::string&);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		uint64_t request_time;
		size_t found_docs;
	};

	const SearchServer& search_server_;
	std::deque<QueryResult> requests_;
	const static int min_in_day_ = 1440;
	int null_requests_ = 0;
	uint64_t current_time_ = 0;

	void CheckRequests(size_t);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
	CheckRequests(result.size());
	return result;
}