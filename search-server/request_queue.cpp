#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :search_server_(search_server)
{
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
	std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
	CheckRequests(result.size());
	return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
	CheckRequests(result.size());
	return result;
}
int RequestQueue::GetNoResultRequests() const {
	return null_requests_;
}