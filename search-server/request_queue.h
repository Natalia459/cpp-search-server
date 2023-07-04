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
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
		std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
		CheckRequests(result.size());
		return result;
	}

	std::vector<Document> AddFindRequest(const std::string&, DocumentStatus);
	std::vector<Document> AddFindRequest(const std::string&);
	int GetNoResultRequests() const;

private:
	const SearchServer& search_server_;
	struct QueryResult {
		uint64_t request_time;
		size_t found_docs;
	};
	std::deque<QueryResult> requests_;
	const static int min_in_day_ = 1440;
	int null_requests_ = 0;
	uint64_t current_time_ = 0;

	void CheckRequests(size_t result_size) {
		++current_time_;
		if (result_size == 0) {
			++null_requests_;
		}
		while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().request_time) {
			if (requests_.front().found_docs == 0) {
				--null_requests_;
			}
			requests_.pop_front();
		}
		requests_.push_back({ current_time_, result_size });
	}
};