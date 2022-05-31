#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicats_ = {};
    size_t total_documents = search_server.GetDocumentCount();

    // удаление дубликатов реализовано путем циклической проверки
    // количество циклов проверки равно количеству документов, обновляется при нахождении и удалении дубликата
    
    bool NextCheck = false; // для перехода на следующую итерацию проверки
    for (int i = 0; i < total_documents; i++) {
        NextCheck = false;
        for (auto it_left = search_server.ToDuplicateBegin(); it_left != search_server.ToDuplicateEnd(); it_left++) {
            if (NextCheck) {
                break;
            }
            else {
                for (auto it_right = search_server.ToDuplicateBegin(); it_right != search_server.ToDuplicateEnd(); it_right++) {

                    if (it_left == it_right) {
                        continue;
                    }
                    else {
                        //проверяем совпадение
                        if (WordsEqualCheck(it_left->second, it_right->second)) {
                            duplicats_.insert(it_right->first);
                            search_server.RemoveDocument(it_right->first);

                            // обновляем длину основного цикла и переходим к следующей проверке
                            total_documents = search_server.GetDocumentCount();
                            NextCheck = true;
                            break;
                        }
                    }
                }
            }
        } 
    }
    // вывод id ранее найденых дубликатов
    for (auto duplicat : duplicats_) {
        std::cout << "Found duplicate document id " << duplicat << std::endl;
    }
}

// то же самое только с использованием GetWordFrequencies();
void RemoveDuplicatesTwo(SearchServer& search_server) {
    std::set<int> duplicats_ = {};
    size_t total_documents = search_server.GetDocumentCount();

    for (int i = 1; i <= total_documents; i++) {
        for (int j = 1; j <= total_documents; j++) {
            if (duplicats_.count(i) || duplicats_.count(j)) {
                continue;
            }
            else {
                auto& left = search_server.GetWordFrequencies(i);
                auto& right = search_server.GetWordFrequencies(j);
                if (i == j) {
                    continue;
                }
                else {
                    //проверяем совпадение
                    if (WordsEqualCheck(left, right)) {
                        duplicats_.insert(j);
                        search_server.RemoveDocument(j);
                    }
                }
            }
        }
    }
    // вывод id ранее найденых дубликатов
    for (auto duplicat : duplicats_) {
        std::cout << "Found duplicate document id " << duplicat << std::endl;
    }
}

std::set<std::string> WordsSetCreator(const std::map<std::string, double>& element) {
    std::set<std::string> result = {};
    for (const auto& e : element) {
        result.insert(e.first);
    }
    return result;
}

bool WordsEqualCheck(const std::map<std::string, double>& first, const std::map<std::string, double>& second) {
    //собираем мапы слов в двух документах сравнения
    std::set<std::string> left = WordsSetCreator(first);
    std::set<std::string> right = WordsSetCreator(second);

    if (left == right) {
        return true;
    }
    else {
        return false;
    }
}
