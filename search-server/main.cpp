// search_server_s1_t2_v2.cpp

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
		//УБРАЛ SWITCH
		/*
		Использовал switch так как с ним была теория и со свчом проходит проверка. 
		Изначально думал, что он не нужен ибо просто куча лишних строчек. 
		Это как функция MatchDocemnts - на данном этапе она вообще нигде не применяется, но если её нет система проверки "заворачивала" решение.
		*/
		/* СТАРЫЙ ВАРИАНТ
		switch (status)
		{
		case DocumentStatus::ACTUAL:
			return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
		case DocumentStatus::IRRELEVANT:
			return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
		case DocumentStatus::BANNED:
			return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
		case DocumentStatus::REMOVED:
			return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::REMOVED; });
		default:
			return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
		}
		*/
	}

	template <typename Pred>
	vector<Document> FindTopDocuments(const string& raw_query, Pred pred) const {
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, pred);
		//УБРАЛ В ЛЯМБДЕ "lhs/rhs", ЗАМЕНИЛ НА ПОНЯТНЫЕ НАИМЕНОВАНИЯ ДЛЯ ЛУЧШЕЙ ЧИТАЕМОСТИ СТОРОННИМ КОДЕРОМ
		//Использование "lhs/rhs" обусловлено тем, что так было в теории, потому и не подумал переназвать
		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& first_document, const Document& second_document) {
			if (abs(first_document.relevance - second_document.relevance) < 1e-6) {
				return first_document.rating > second_document.rating;
			}
			else {
				return first_document.relevance > second_document.relevance;
			}
		}); /*
		ДЛЯ СПРАВКИ ПО SORT
		Сортировка выполянется в полученном векторе типа Document, содержащим поля "id", "relevance", "rating"
		Сортировка выполняется по релевантности от большей к меньшей.
		Если релевантности очень близки, тогда по рейтингу также от большего к меньшему.
		*/
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	int GetDocumentCount() const {
		return documents_.size();
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
		//СЧИТАЕМ ЧЕРЕЗ ACCUMULATE
		int rating_sum = accumulate(begin(ratings), end(ratings), 0);

		/* СТАРЫЙ ВАРИАНТ
		int rating_sum = 0;
		for (const int rating : ratings) {
			rating_sum += rating;
		}
		*/
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
			for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {
				/*
				СТАРЫЙ ВАРИАНТ if. Два поисковых запроса "documents_.at(document_id).status" и "documents_.at(document_id).rating"
				if (pred(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
				*/
				/*
				Чтобы сократить количество запросов по document_id вижу решение - создать новую переменую типа DocumentData по текущему document_id.
				Новая переменная doc_id_dat создаётся путем однократного обращения к массиву documents_ по document_id.
				Новая переменная doc_id_dat хранит в себе поля status и rating, которые нам нужны в работе с предикатом pred.
				Новая переменная doc_id_dat создаётся только в цикле и больше нигде не видна, таким образом не обрастает мусором и удаляется сразу после выхода из цикла.
				*/
				DocumentData doc_id_dat = documents_.at(document_id);
				if (pred(document_id, doc_id_dat.status, doc_id_dat.rating)) {
				
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto[document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({
												document_id,
												relevance,
												documents_.at(document_id).rating
				});
		}
		return matched_documents;
	}
};


// ==================== для примера =========================


void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating
		<< " }"s << endl;
}

int main() {

	setlocale(LC_ALL, "Russian");

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
