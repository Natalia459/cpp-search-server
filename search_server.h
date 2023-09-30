#pragma once
#include <algorithm>
#include <numeric>
#include <cmath>
#include <tuple>
#include <map>
#include <set>
#include <deque>
#include <execution>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <iterator>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const static int MAX_RESULT_DOCUMENT_COUNT = 5;
const static double EPSILON = 1e-6;
const static size_t MAX_BUCKET_MAP_COUNT = 10;

class SearchServer {
private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	std::deque<std::string> storage_;
	const std::set<std::string, std::less<>> stop_words_;
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	std::map<int, std::map<std::string_view, double>> id_to_document_word_;
	std::map<int, struct DocumentData> documents_;
	std::set<int> document_ids_;

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	struct Query {
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words;
	};

public:
	template <typename StringContainer>
	SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string_view stop_words_text);
	explicit SearchServer(const std::string& stop_words_text);

	std::set<int>::iterator begin();

	std::set<int>::iterator end();

	void AddDocument(int, const std::string_view, DocumentStatus, const std::vector<int>&);

	template <typename DocumentPredicate, typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy&, const std::string_view, DocumentPredicate) const;
	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string_view, DocumentPredicate) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy&, const std::string_view, DocumentStatus) const;
	std::vector<Document> FindTopDocuments(const std::string_view, DocumentStatus) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy&, const std::string_view) const;
	std::vector<Document> FindTopDocuments(const std::string_view) const;

	int GetDocumentCount() const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view, int) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, const std::string_view, int) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const std::string_view, int) const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int document_id);
	void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
	void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:
	bool IsStopWord(const std::string_view) const;

	static bool IsValidWord(const std::string_view);

	std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view) const;

	static int ComputeAverageRating(const std::vector<int>&);

	QueryWord ParseQueryWord(std::string_view&) const;

	Query CommonOfParseQuery(const std::string_view) const;
	Query ParseQuery(const std::execution::sequenced_policy&, const std::string_view) const;
	Query ParseQuery(const std::execution::parallel_policy&, const std::string_view) const;

	double ComputeWordInverseDocumentFreq(const std::string_view) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query&, DocumentPredicate) const;
	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query&, DocumentPredicate) const;

	template <typename DocumentPredicate, typename Map>
	std::vector<Document> CommonOfFindAllDocuments(Map&, const std::vector<std::string_view>&, DocumentPredicate)const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
	if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
	{
		throw std::invalid_argument("Some of stop words are invalid");
	}
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const
{
	const SearchServer::Query query = ParseQuery(std::execution::seq, raw_query);

	std::vector<Document> matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

	sort(matched_documents.begin(), matched_documents.end(),
		[](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
			{
				return lhs.rating > rhs.rating;
			}
			else
			{
				return lhs.relevance > rhs.relevance;
			}
		});

	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
	{
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const
{
	SearchServer::Query query = ParseQuery(policy, raw_query);

	std::vector<Document> matched_documents = FindAllDocuments(policy, query, document_predicate);

	sort(policy, matched_documents.begin(), matched_documents.end(),
		[](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
			{
				return lhs.rating > rhs.rating;
			}
			else
			{
				return lhs.relevance > rhs.relevance;
			}
		});

	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
	{
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}



template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy& policy, const Query& query, DocumentPredicate document_predicate) const
{
	std::map<int, double> document_to_relevance;
	for (const std::string_view word : query.plus_words)
	{
		if (word_to_document_freqs_.count(word) != 0)
		{
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
			{
				const auto& document_data = documents_.at(document_id);
				if (document_predicate(document_id, document_data.status, document_data.rating))
				{
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}
	}

	return CommonOfFindAllDocuments(document_to_relevance, query.minus_words, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query, DocumentPredicate document_predicate) const {
	ConcurrentMap<int, double> document_to_relevance(MAX_BUCKET_MAP_COUNT);

	std::vector<std::string_view> plus_query(query.plus_words.begin(), query.plus_words.end());
	std::sort(plus_query.begin(), plus_query.end());
	plus_query.erase(std::unique(plus_query.begin(), plus_query.end()), plus_query.end());

	std::for_each(policy, plus_query.begin(), plus_query.end(), [&](const std::string_view word)
		{
			if (word_to_document_freqs_.count(word) != 0)
			{
				const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
				for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
				{
					const auto& document_data = documents_.at(document_id);
					if (document_predicate(document_id, document_data.status, document_data.rating))
					{
						document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
					}
				}
			}
		});

	return CommonOfFindAllDocuments(document_to_relevance, query.minus_words, document_predicate);
}

template <typename DocumentPredicate, typename Map>
std::vector<Document> SearchServer::CommonOfFindAllDocuments(Map& document_to_relevance, const std::vector<std::string_view>& minus_query, DocumentPredicate document_predicate) const
{
	std::for_each(minus_query.begin(), minus_query.end(), [&](const std::string_view word)
		{
			if (word_to_document_freqs_.count(word) != 0)
			{
				for (const auto [document_id, _] : word_to_document_freqs_.at(word))
				{
					document_to_relevance.erase(document_id);
				}
			}
		});

	std::vector<Document> matched_documents;
	for (const auto& [document_id, relevance] : document_to_relevance)
	{
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}

	return matched_documents;
}