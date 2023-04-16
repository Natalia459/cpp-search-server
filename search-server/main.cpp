#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
 
using namespace std;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
 
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
 
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
 
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
 
    return words;
}
 
struct Document {
    int id;
    double relevance;
};
 
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
 
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const int words_size = words.size();
        ++document_count_;
        for (const auto& new_word : words){
            double k = count_if (begin(words), end(words), [new_word](const string& word){
                      if (new_word == word){
                          return true;
                      }
                      return false;
            });
            documents_[new_word].insert({document_id, k/words_size});
        }
    }
 
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);
 
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
 
private:
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
 
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
 
    map<string, map<int, double>> documents_;
    set<string> stop_words_;
    int document_count_ = 0;
 
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
 
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
 
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }
 
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }
 
    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> relevance;
        for (auto word : query.plus_words) {
            if (documents_.count(word) != 0){
                int counter_full_docs = documents_.at(word).size();
                 for (const auto& id : documents_.at(word)) {
                    relevance[id.first] += log(static_cast<double>(document_count_)/counter_full_docs) * id.second;
                }
            }  
        }
        for (auto word : query.minus_words) {
            if (documents_.count(word) != 0)
                for (const auto& id : documents_.at(word)) {
                        relevance.erase(id.first);
                }
        }
        vector<Document> matched_documents;
        for (const auto& rel : relevance) {
                matched_documents.push_back({ rel.first, rel.second });
        }
        return matched_documents;
    } 
};
 
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
 
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}
 
int main() {
    const SearchServer search_server = CreateSearchServer();
 
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
