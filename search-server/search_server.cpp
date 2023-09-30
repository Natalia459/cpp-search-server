#include "search_server.h"
#include "string_processing.h"
#include "log_duration.h"


SearchServer::SearchServer(const std::string_view stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

std::set<int>::iterator SearchServer::begin()
{
	return document_ids_.begin();
}
std::set<int>::iterator SearchServer::end()
{
	return document_ids_.end();
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
	const std::vector<int>& ratings)
{
	using namespace std::literals;

	if ((document_id < 0) || (documents_.count(document_id) > 0))
	{
		throw std::invalid_argument("Invalid document_id"s);
	}

	storage_.push_back(std::string(document));
	const std::vector<std::string_view> words = SearchServer::SplitIntoWordsNoStop(storage_.back());

	const double inv_word_count = 1.0 / words.size();
	for (const std::string_view word : words)
	{
		id_to_document_word_[document_id][word] += inv_word_count;
		word_to_document_freqs_[word][document_id] += inv_word_count;
	}

	document_ids_.emplace(document_id);
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const
{
	return SearchServer::FindTopDocuments(raw_query, [status]([[maybe_unused]] int document_id, [[maybe_unused]] DocumentStatus document_status, [[maybe_unused]] int rating)
		{ return document_status == status; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const
{
	return SearchServer::FindTopDocuments(policy, raw_query, [status]([[maybe_unused]] int document_id, [[maybe_unused]] DocumentStatus document_status, [[maybe_unused]] int rating)
		{ return document_status == status; });
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
	return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const
{
	return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
	return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
	using namespace std::literals;
	if (document_ids_.count(document_id) == 0)
	{
		throw std::out_of_range("Document's id doesn't exist"s);
	}

	const SearchServer::Query query = ParseQuery(std::execution::seq, raw_query);

	for (const std::string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id) != 0) {
			return { {}, documents_.at(document_id).status };
		}
	}

	std::vector<std::string_view> matched_words;

	for (const std::string_view word : query.plus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id) != 0)
		{
			matched_words.push_back(std::move(word));
		}
	}

	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const
{
	return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, const std::string_view raw_query, int document_id) const
{
	using namespace std::literals;
	if (document_ids_.count(document_id) == 0)
	{
		throw std::out_of_range("Document's id doesn't exist"s);
	}

	const auto& doc_id = id_to_document_word_.at(document_id);
	SearchServer::Query query = ParseQuery(std::execution::par, raw_query);

	if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view word)
		{
			return doc_id.count(word) != 0;
		}))
	{
		return { {}, documents_.at(document_id).status };
	}

	std::vector<std::string_view> matched_words;

	matched_words.resize(query.plus_words.size());

	auto last_elem = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](const std::string_view word)
		{
			return doc_id.count(word) != 0;
		});

	std::sort(matched_words.begin(), last_elem);

	matched_words.erase(std::unique(matched_words.begin(), last_elem), matched_words.end());

	return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	const auto itera = id_to_document_word_.find(document_id);
	if (itera != id_to_document_word_.end())
	{
		return itera->second;
	}

	static std::map<std::string_view, double> empty_map = {};
	return empty_map;
}

void SearchServer::RemoveDocument(int document_id)
{
	for (auto& [word, _] : id_to_document_word_.at(document_id))
	{
		word_to_document_freqs_.at(word).erase(document_id);
		if (word_to_document_freqs_.at(word).empty())
		{
			word_to_document_freqs_.erase(word);
		}
	}

	id_to_document_word_.erase(document_id);
	documents_.erase(document_id);
	document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id)
{
	SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id)
{
	if (document_ids_.count(document_id) > 0)
	{
		const std::map<std::string_view, double>& document = id_to_document_word_.at(document_id);
		std::vector<std::string_view> vec(document.size());

		std::transform(policy, document.begin(), document.end(), vec.begin(), [](const auto& pair)
			{ return pair.first; });

		std::for_each(policy, vec.begin(), vec.end(), [&](const std::string_view doc)
			{
				if (word_to_document_freqs_.count(doc) != 0)
				{
					word_to_document_freqs_.at(doc).erase(document_id);
				}
			});

		id_to_document_word_.erase(document_id);
		documents_.erase(document_id);
		document_ids_.erase(document_id);
	}
}

bool SearchServer::IsStopWord(const std::string_view word) const
{
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word)
{
	return std::none_of(word.begin(), word.end(), [](char c)
		{ return c >= '\0' && c < ' '; });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
	using namespace std;
	std::vector<std::string_view> words;

	for (const std::string_view& word : SplitIntoWords(text))
	{
		if (!IsValidWord(word))
		{
			throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
		}
		if (!IsStopWord(word))
		{
			words.push_back(std::move(word));
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.empty())
	{
		return 0;
	}

	return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view& word) const
{
	{
		using namespace std;
		if (word.empty())
		{
			throw std::invalid_argument("Query word is empty"s);
		}

		bool is_minus = false;

		if (word[0] == '-')
		{
			is_minus = true;
			word = word.substr(1);
		}

		if (word.empty() || word[0] == '-' || !IsValidWord(word))
		{
			throw std::invalid_argument("Query word is invalid");
		}

		return { word, is_minus, IsStopWord(word) };
	}
}

SearchServer::Query SearchServer::CommonOfParseQuery(const std::string_view text) const
{
	std::list<std::string_view> words = SplitIntoWords(text);
	Query result;

	result.plus_words.reserve(words.size());
	result.minus_words.reserve(words.size());

	for (std::string_view word : words)
	{
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			(query_word.is_minus) ?
				result.minus_words.push_back(std::move(query_word.data))
				: result.plus_words.push_back(std::move(query_word.data));
		}
	}
	return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy&, const std::string_view text) const
{
	Query result = CommonOfParseQuery(text);

	std::sort(result.plus_words.begin(), result.plus_words.end());
	const auto last_uniq_plus = std::unique(result.plus_words.begin(), result.plus_words.end());
	result.plus_words.erase(last_uniq_plus, result.plus_words.end());

	std::sort(result.minus_words.begin(), result.minus_words.end());
	const auto last_uniq_minus = std::unique(result.minus_words.begin(), result.minus_words.end());
	result.minus_words.erase(last_uniq_minus, result.minus_words.end());

	return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy&, const std::string_view text) const
{
	return CommonOfParseQuery(text);
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const
{
	return log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
