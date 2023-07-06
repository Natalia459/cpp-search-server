#include <vector>
#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	using namespace std;
	std::set<int> document_ids;

	for (const auto id : search_server) {
		document_ids.emplace(id);
	}

	std::map<int, std::set<std::string>> words_from_documents_ = {};
	for (int id : document_ids) {
		bool is_found(false);
		std::set<std::string> current_words;

		for (const auto& [word, freq] : search_server.GetWordFrequencies(id)) {
			current_words.insert(word);
		}

		for (auto& [document_id, words] : words_from_documents_) {
			if (words == current_words) {
				std::cout << "Found duplicate document id "s << id << endl;
				is_found = true;
				search_server.RemoveDocument(id);
				break;
			}
		}

		if (is_found == false) {
			words_from_documents_[id] = current_words;
		}
	}
}