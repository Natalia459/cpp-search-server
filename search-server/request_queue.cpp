#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) 
{
	std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
	CheckRequests(result.size());
	return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) 
{
	std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
	CheckRequests(result.size());
	return result;
}

int RequestQueue::GetNoResultRequests() const 
{
	return null_requests_;
}

void RequestQueue::CheckRequests(size_t result_size) 
{
	++current_time_;
	if (result_size == 0) 
	{
		++null_requests_;
	}

	while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().request_time) 
	{
		if (requests_.front().found_docs == 0) 
		{
			--null_requests_;
		}
		
		requests_.pop_front();
	}
	requests_.push_back({ current_time_, result_size });
}