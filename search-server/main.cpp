// search_server_s2_t12_v1.cpp (называю по принципу Sprint 2, Tema 12, Version 1)

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_THRESHOLD = 0.000001; //порог срабатывания сортировки по релевантности 1e-6, оно же 10 в степени -6

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
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
		}
		else {
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
	int rating;
};

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer {
public:
	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id,
			DocumentData{
					ComputeAverageRating(ratings),
					status
			});
	}


	vector<Document> FindTopDocuments(const string& raw_query) const {
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}


	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus input_status) const {
		return FindTopDocuments(raw_query, [input_status](int document_id, DocumentStatus status, int rating) { return status == input_status; });
	}

	template <typename Pred>
	vector<Document> FindTopDocuments(const string& raw_query, Pred pred) const {
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, pred);
		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs_document, const Document& rhs_document) {
				if (abs(lhs_document.relevance - rhs_document.relevance) < RELEVANCE_THRESHOLD) {
					return lhs_document.rating > rhs_document.rating;
				}
				else {
					return lhs_document.relevance > rhs_document.relevance;
				}
			});

		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	int GetDocumentCount() const {
		return static_cast<int>(documents_.size());
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.push_back(word);
			}
		}
		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.clear();
				break;
			}
		}
		return { matched_words, documents_.at(document_id).status };
	}

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

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

	static int ComputeAverageRating(const vector<int>& ratings) {
		if (ratings.empty()) {
			return 0;
		}
		int rating_sum = accumulate(begin(ratings), end(ratings), 0);
		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord {
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const {
		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-') {
			is_minus = true;
			text = text.substr(1);
		}
		return {
			text,
			is_minus,
			IsStopWord(text)
		};
	}

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const {
		Query query;
		for (const string& word : SplitIntoWords(text)) {
			const QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					query.minus_words.insert(query_word.data);
				}
				else {
					query.plus_words.insert(query_word.data);
				}
			}
		}
		return query;
	}

	// Existence required
	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}


	template <typename Pred>
	vector<Document> FindAllDocuments(const Query& query, Pred& pred) const {

		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {

				const DocumentData& doc_id_data = documents_.at(document_id);
				if (pred(document_id, doc_id_data.status, doc_id_data.rating)) {

					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}
};


// -------------------- для примера -------------------------

void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating
		<< " }"s << endl;
}

// ----------------- макросы для тестирования --------------

template <typename inc>
void RunTestImpl(const inc func, const string& str_func) {
	func();
	cerr << str_func << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

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

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

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

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	// Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
	// находит нужный документ
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	// Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
	// возвращает пустой результат
	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void TestMinusWords() {
	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const string test_request = "cat -neck"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		const auto found_docs_one = server.FindTopDocuments(test_request);
		ASSERT_HINT(found_docs_one.empty(), "Документов быть не должно"s);

		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		const auto found_docs_two = server.FindTopDocuments(test_request);
		ASSERT_EQUAL_HINT(found_docs_two.size(), 1, "Должен найтись один документ"s);
	}
}

void TestMatching() {
	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const string test_request = "sweet good cat -cool"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		const auto found_docs = server.FindTopDocuments(test_request, DocumentStatus::ACTUAL);
		ASSERT_EQUAL_HINT(found_docs.size(), 1, "Документ должен быть всего один - без документа с минус словами"s);

		//достаем вектор слов из тупли с функции MatchDocument
		vector<string> match_vector = get<0>(server.MatchDocument(test_request, doc_id_1));
		ASSERT_EQUAL_HINT(match_vector.size(), 2, "По условию воодных данных должны быть слова sweet и cat"s);
		//так как они прохрдят через парсинг плюс/минус слов то меняются местами 
		//ожидается первое cat, второе sweet, проверим.
		ASSERT_EQUAL(match_vector[0], "cat"s);
		ASSERT_EQUAL(match_vector[1], "sweet"s);
	}
}

void TestRelevanceSort() {
	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const string test_request = "sweet good cat"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		const auto found_docs = server.FindTopDocuments(test_request, DocumentStatus::ACTUAL);
		ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance, "Первый документ в финальной выборке должен иметь бОльшую релевантность чем второй"s);
	}
}

void TestRating() {
	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const string test_request = "sweet good cat"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		const auto found_docs = server.FindTopDocuments(test_request, DocumentStatus::ACTUAL);

		ASSERT_EQUAL_HINT(found_docs[0].rating, 5, "Ожидаемое значение рейтинга первого в выдаче документа (7 + 2 + 7) / 3 = 5"s);
		ASSERT_EQUAL_HINT(found_docs[1].rating, 2, "Ожидаемое значение рейтинга второго в выдаче документа (8 - 3) / 2 = 2"s);
	}
}

void TestPredicate() {

	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const string test_request = "sweet good cat"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::REMOVED, ratings_1);
		const auto found_docs = server.FindTopDocuments(test_request, [](int document_id,
			DocumentStatus status, int rating) { return status == DocumentStatus::REMOVED; });
		ASSERT_EQUAL_HINT(found_docs.size(), 1, "Должен быть только один документ в статусе REMOVED"s);
	}
}

void TestStatusSearch() {

	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const int doc_id_2 = 2;
	const string content_2 = "good dog nice eyes"s;
	const vector<int> ratings_2 = { 5, -12, 2, 1 };

	const int doc_id_3 = 3;
	const string content_3 = "good bird max"s;
	const vector<int> ratings_3 = { 5, -12, 2, 1 };

	const string test_request = "sweet good cat"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::REMOVED, ratings_1);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
		server.AddDocument(doc_id_3, content_3, DocumentStatus::REMOVED, ratings_2);

		const auto found_docs_actual = server.FindTopDocuments(test_request, DocumentStatus::ACTUAL);
		const auto found_docs_irrelevant = server.FindTopDocuments(test_request, DocumentStatus::IRRELEVANT);
		const auto found_docs_removed = server.FindTopDocuments(test_request, DocumentStatus::REMOVED);
		const auto found_docs_banned = server.FindTopDocuments(test_request, DocumentStatus::BANNED);

		ASSERT_EQUAL_HINT(found_docs_actual.size(), 1, "В статусе ACTUAL должен быть один файл"s);
		ASSERT_HINT(found_docs_irrelevant.empty(), "В статусе IRRELEVANT не должно быть файлов"s);
		ASSERT_EQUAL_HINT(found_docs_removed.size(), 2, "В статусе REMOVED должены быть два файла"s);
		ASSERT_EQUAL_HINT(found_docs_banned.size(), 1, "В статусе BANNED должен быть один файл"s);
	}
}

void TestRelevance() {
	const int doc_id_0 = 0;
	const string content_0 = "white cat cool neck"s;
	const vector<int> ratings_0 = { 8, -3 };

	const int doc_id_1 = 1;
	const string content_1 = "sweet cat sweet tail"s;
	const vector<int> ratings_1 = { 7, 2, 7 };

	const int doc_id_2 = 2;
	const string content_2 = "good dog nice eyes"s;
	const vector<int> ratings_2 = { 5, -12, 2, 1 };

	const int doc_id_3 = 3;
	const string content_3 = "good bird evgeniy"s;
	const vector<int> ratings_3 = { 5, -12, 2, 1 };

	const string test_request = "sweet good cat"s;

	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
		server.AddDocument(doc_id_3, content_3, DocumentStatus::BANNED, ratings_2);
		const auto found_docs = server.FindTopDocuments(test_request);

		//ожидаемые расчётные релевантности
		double top_relevance = 0.866433;
		double low_relevance = 0.173286;

		ASSERT(abs(found_docs[0].relevance - top_relevance) < RELEVANCE_THRESHOLD);
		ASSERT(abs(found_docs[1].relevance - low_relevance) < RELEVANCE_THRESHOLD);
	}
}

void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestMinusWords); //тест на минус слова
	RUN_TEST(TestMatching); //тест на матчинг
	RUN_TEST(TestRelevanceSort); //сортировка по релевантности
	RUN_TEST(TestRating); //высчитывание рейтинга документа
	RUN_TEST(TestPredicate); //работа с предикатом
	RUN_TEST(TestStatusSearch); //работа со статусом
	RUN_TEST(TestRelevance); //подсчёт релевантности
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {

	setlocale(LC_ALL, "Russian");

	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;

	SearchServer search_server;
	search_server.SetStopWords("и в на"s);

	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });


	cout << "ACTUAL by default:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
		PrintDocument(document);
	}

	cout << "BANNED [Query + Status].Test:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
		PrintDocument(document);
	}

	cout << "BANNED:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; })) {
		PrintDocument(document);
	}

	cout << "Even ids:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
		PrintDocument(document);
	}

	return 0;
}
