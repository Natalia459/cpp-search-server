#include "process_queries.h"

#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<std::vector<Document>> res(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), res.begin(), [&search_server](const std::string_view& query)
		{
			return search_server.FindTopDocuments(query);
		}
	);
	return res;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::list<Document> res;
	const std::vector<std::vector<Document>> answers = ProcessQueries(search_server, queries);
	for (const auto& answer : answers)
	{
		for (const auto& ans : answer)
		{
			res.push_back(ans);
		}
	}
	return res;
}