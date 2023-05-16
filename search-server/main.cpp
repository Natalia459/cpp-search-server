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

//#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

//#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

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

//#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

//#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))



// -------- Начало модульных тестов поисковой системы ----------
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*Разместите код остальных тестов здесь*/
void TestFindDocumentWithFordsFromIt() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("cat"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments(""s);
		ASSERT_EQUAL(found_docs.size(), 0);
	}

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("dog"s);
		ASSERT_EQUAL(found_docs.size(), 0);
	}
}

void TestExcludeDocumentsWithMinusWordsFromSearchingResult() {
	const int doc_id = 42;
	const string content = "found white cat with the black ear"s;
	const vector<int> ratings = { 1,2,3,4,5,6 };
	{
		SearchServer server;
		server.SetStopWords("in the with"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("nice cat with black eyar -white"s);
		ASSERT_EQUAL(found_docs.size(), 0);
	}
}

void TestCountRatings() {
	const int doc_id = 42;
	const string content = "found white cat with the black ear"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.SetStopWords("in the with"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("nice cat with black eyar"s);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.rating, 2);
	}

}

void TestCheckPredicate() {

	{
		SearchServer search_server;
		search_server.SetStopWords("and in on"s);
		search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		search_server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		search_server.AddDocument(3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 });
		const auto found_docs = search_server.FindTopDocuments("fluffy well-groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
		for (const auto& doc : found_docs) {
			ASSERT(doc.id == 0 || doc.id == 2);
		}
	}
}

void TestCheckGivedStatus() {
	{
		SearchServer search_server;
		search_server.SetStopWords("and in on"s);
		search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::REMOVED, { 8, -3 });
		search_server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
		search_server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		search_server.AddDocument(3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 });
		const auto found_docs = search_server.FindTopDocuments("fluffy well-groomed cat"s, DocumentStatus::BANNED);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, 3);
	}
}

void TestCheckCorrectlyCountedAndSortedRelevance() {
	{
		SearchServer search_server;
		search_server.SetStopWords("and in on"s);
		search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
		search_server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		search_server.AddDocument(3, "well-groomed starling evgeny"s, DocumentStatus::BANNED, { 9 });
		const auto found_docs = search_server.FindTopDocuments("fluffy well-groomed cat"s);
		const vector<double> right_relevance = { 0.866434, 0.173287, 0.173287 };
		const double my_EPSILION = 1e-6;
		for (int i = 0; i < found_docs.size(); ++i) {
			ASSERT(abs(found_docs[i].relevance - right_relevance[i]) < my_EPSILION);
		}
	}
}

void TestCheckMatchedWords() {
	{
		SearchServer search_server;
		search_server.SetStopWords("and in on"s);
		search_server.AddDocument(0, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(1, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		const auto found_docs1 = search_server.MatchDocument("fluffy well-groomed cat"s, 1);
		const auto [vector_docs1, status1] = found_docs1;
		ASSERT_EQUAL(vector_docs1[0], "well-groomed"s);

		const auto found_docs2 = search_server.MatchDocument("fluffy well-groomed -cat"s, 0);
		const auto [vector_docs2, status2] = found_docs2;
		ASSERT(vector_docs2.empty());
	}
}


template <typename Predicate>
void RunTestImpl(Predicate predicate, const string& predicate_name) {
	predicate;
	cerr << predicate_name << " OK"s << endl;
}

//#define RUN_TEST(func) RunTestImpl(func, #func)

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestFindDocumentWithFordsFromIt);
	RUN_TEST(TestExcludeDocumentsWithMinusWordsFromSearchingResult);
	RUN_TEST(TestCountRatings);
	RUN_TEST(TestCheckPredicate);
	RUN_TEST(TestCheckGivedStatus);
	RUN_TEST(TestCheckCorrectlyCountedAndSortedRelevance);
	RUN_TEST(TestCheckMatchedWords);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
