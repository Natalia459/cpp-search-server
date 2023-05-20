#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */


/*Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST*/
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
	const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cout << boolalpha;
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
	const string& hint) {
	if (!value) {
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

struct ForTests {
	int document_id;
	string content;
	DocumentStatus status;
	vector<int> ratings;
};

void TestExcludeStopWordsFromAddedDocumentContent() {
	ForTests test1 = { 42, "cat in the city"s,  DocumentStatus::ACTUAL, { 1, 2, 3 } };

	// Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
	// находит нужный документ
	{
		SearchServer server;
		server.AddDocument(test1.document_id, test1.content, test1.status, test1.ratings);
		const vector<Document> found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, test1.document_id);
	}

	// Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
	// возвращает пустой результат
	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(test1.document_id, test1.content, test1.status, test1.ratings);
		ASSERT(server.FindTopDocuments("in"s).empty());
	}
}

void TestFindedDocumentWithWordsFromIt() {

	ForTests test2 = { 42, "cat in the city"s,  DocumentStatus::ACTUAL, { 1, 2, 3 } };

	{
		SearchServer server;
		const int documents = server.GetDocumentCount();
		ASSERT_HINT(documents == 0, "Server isn't empty"s);

		server.AddDocument(test2.document_id, test2.content, test2.status, test2.ratings);
		const vector<Document> found_docs = server.FindTopDocuments("cat"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, test2.document_id);
	}

	{
		SearchServer server;
		server.AddDocument(test2.document_id, test2.content, test2.status, test2.ratings);
		const auto found_docs = server.FindTopDocuments(""s);
		ASSERT(found_docs.empty());
	}

	{
		SearchServer server;
		server.AddDocument(test2.document_id, test2.content, test2.status, test2.ratings);
		const auto found_docs = server.FindTopDocuments("dog"s);
		ASSERT(found_docs.empty());
	}
}

void TestExcludeDocumentsWithMinusWordsFromSearchingResult() {
	vector<ForTests> test3 = {
		{1, "found white cat with the black ear"s, DocumentStatus::ACTUAL, { 1,2,3,4,5,6 }},
		{2, "fluffy cat is looking for an owner. special features: fluffy tail"s, DocumentStatus::ACTUAL, { 1,2,3,4,5,6 }}
	};
	{
		SearchServer server;
		server.SetStopWords("in the with"s);
		for (const auto& test : test3) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);

		}
		//получение результата, с которым потом сравним полученный из FindTopDocuments
		vector<string> query = SplitIntoWords("nice cat with black eyar -fluffy"s);
		set<string> minus_words;
		for (string text : query) {
			if (!text.empty()) {
				if (text[0] == '-') {
					text = text.substr(1);
					minus_words.insert(text);
				}
			}
		}
		vector<int> result;
		for (const auto& test : test3) {
			vector<string> document = SplitIntoWords(test.content);
			for (const string& word : document) {
				if (minus_words.count(word) != 0) {
					break;
				}
				else {
					result.emplace_back(test.document_id);
				}
			}
		}

		const vector<Document> found_docs2 = server.FindTopDocuments("nice cat with black eyar -fluffy"s);
		const Document& doc2 = found_docs2[0];
		ASSERT_EQUAL(found_docs2.size(), 1);
		if (result.size() == 1) {
			ASSERT(result[0] == doc2.id);
		}
	}
}

void TestCountedRatings() {
	{
		ForTests test4 = { 42, "found white cat with the black ear"s, DocumentStatus::ACTUAL, { 1, 2, 3 } };
		int rating_sum = 0;
		for (const int rating : test4.ratings) {
			rating_sum += rating;
		}
		rating_sum /= static_cast<int>(test4.ratings.size());
		SearchServer server;
		server.SetStopWords("in the with"s);
		server.AddDocument(test4.document_id, test4.content, test4.status, test4.ratings);
		const vector<Document> found_docs = server.FindTopDocuments("nice cat with black eyar"s);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.rating, rating_sum);
	}
}

void TestResultOfQueryWithPredicate() {
	vector<ForTests> test5{
		{0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 }},
		{1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 }},
		{2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 }},
		{3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 }}
	};
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		for (const auto& test : test5) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);
		}
		const vector<Document> found_docs = server.FindTopDocuments("fluffy well-groomed cat"s, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; });
		for (const auto& doc : found_docs) {
			ASSERT(doc.id == 0 || doc.id == 2);
		}
	}
}

void TestResultOfQueryWithGivedStatus() {
	vector<ForTests> test6{
		{0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 }},
		{1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 }},
		{2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 }},
		{3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 }}
	};
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		for (const auto& test : test6) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);
		}
		const vector<Document> found_docs = server.FindTopDocuments("fluffy well-groomed cat"s, DocumentStatus::BANNED);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, 3);
		ASSERT(doc0.id != 42);
	}
}

void TestCorrectlyCountedRelevance() {
	vector<ForTests> test7{
		{0, "white cat fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 }},
		{1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 }},
		{2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 }},
		{3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 }}
	};
	{
		SearchServer server;
		for (const auto& test : test7) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);
		}
		vector<string> query = SplitIntoWords("fluffy well-groomed cat"s);
		
		map<string, map<int, double>> word_to_document_freqs_;
		map<int, double> document_to_relevance;
		for (auto& test : test7) {
			vector<string> documents = SplitIntoWords(test.content);
			const double inv_word_count = 1.0 / documents.size();
			for (const string& word : documents) {
				word_to_document_freqs_[word][test.document_id] += inv_word_count;
			}
		}
		for (const auto& word : query) {
			const double inverse_document_freq = log(test7.size() * 1.0 / word_to_document_freqs_[word].size());
			for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}

		const vector<Document> found_docs = server.FindTopDocuments("fluffy well-groomed cat"s);
		const double my_EPSILION = 1e-6;
		for (const auto& [id, relevance] : document_to_relevance) {
			for (const auto& doc : found_docs) {
				if (id == doc.id) {
					ASSERT(abs(doc.relevance - relevance) < my_EPSILION);
				}
			}
		}
	}
}

void TestSortedRelevance() {
	vector<ForTests> test7{
		{0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 }},
		{1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 }},
		{2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 }},
		{3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 }}
	};
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		for (const auto& test : test7) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);
		}
		const vector<Document> found_docs = server.FindTopDocuments("fluffy well-groomed cat"s);
		bool is_first{ true };
		for (int i = 0; i < found_docs.size(); ++i) {
			if (is_first) {
				is_first = false;
				continue;
			}
			ASSERT(found_docs[i - 1].relevance >= found_docs[i].relevance);
		}
	}
}

void TestSortedRelevance2() {
	SearchServer search_server;
	search_server.SetStopWords("and in on"s);
	search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 });
	const vector<Document> found_docs = search_server.FindTopDocuments("fluffy well-groomed cat"s, DocumentStatus::ACTUAL);
	bool is_first{ true };
	for (int i = 0; i < found_docs.size(); ++i) {
		if (is_first) {
			is_first = false;
			continue;
		}
		ASSERT(found_docs[i - 1].relevance >= found_docs[i].relevance);
	}
}

void TestMatchedWords() {
	vector<ForTests> test8 = {
		{0, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 }},
		{1, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 }}
	};
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		for (const auto& test : test8) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);
		}
		const tuple<vector<string>, DocumentStatus> found_docs1 = server.MatchDocument("fluffy well-groomed cat"s, 1);
		const auto& [vector_docs1, status1] = found_docs1;
		ASSERT_EQUAL(vector_docs1[0], "well-groomed"s);
	}
	{
		SearchServer server;
		server.SetStopWords("and in on"s);
		for (const auto& test : test8) {
			server.AddDocument(test.document_id, test.content, test.status, test.ratings);
		}
		const tuple<vector<string>, DocumentStatus> found_docs2 = server.MatchDocument("fluffy well-groomed -cat"s, 0);
		const auto& [vector_docs2, status2] = found_docs2;
		ASSERT(vector_docs2.empty());
	}
}

template <typename Predicate>
void RunTestImpl(Predicate predicate, const string& predicate_name) {
	predicate();
	cerr << predicate_name << " OK"s << endl;
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestFindedDocumentWithWordsFromIt);
	RUN_TEST(TestExcludeDocumentsWithMinusWordsFromSearchingResult);
	RUN_TEST(TestCountedRatings);
	RUN_TEST(TestResultOfQueryWithPredicate);
	RUN_TEST(TestResultOfQueryWithGivedStatus);
	RUN_TEST(TestCorrectlyCountedRelevance);
	RUN_TEST(TestSortedRelevance);
	RUN_TEST(TestMatchedWords);
	// Не забудьте вызывать остальные тесты здесь
}
// --------- Окончание модульных тестов поисковой системы -----------


int main() {
	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;
}
