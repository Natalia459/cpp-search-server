#include <vector>
#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::vector<int> document_ids;
	for (const auto id : search_server) {
		document_ids.emplace_back(id);
	}

	for (int id : document_ids) {
		if (search_server.CheckForRemoveDocument(id)) {
			search_server.RemoveDocument(id);
		}
	}
}