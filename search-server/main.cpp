// search_server v1_s8

#include <optional>
#include <stdexcept>
#include <vector>
#include "document.h" // структура документ
#include "paginator.h" // постраничный вывод
#include "search_server.h" // поисковый сервер
#include "request_queue.h" // сервер запросов
#include "log_duration.h" // профилировщик
#include "concurrent_map.h" // ускоренная мапа
#include "remove_duplicates.h" // функционал поиска и удаления дубликатов
#include "process_queries.h" // система параллельной обработки запросов
#include "read_input_functions.h" // функции ввода данных
#include "test_example_functions.h" // модули тестовых запусков через try/catch
#include "main_execution_tests.h" // используемые материалы для тестирования системы

using namespace std;



int main() {
    setlocale(LC_ALL, "Russian");
    
    // ВСЕ ТЕСТЫ НАХОДЯТСЯ В "main_execution_tests.h"

    {
        // Старые тесты из 5 спринта
        QueueTests();
        DuplicatesTests();
    }

    {
        // тесты Тема 12. Урок 8. Параллелим запросы поисковой системы
        {
            ProcessQueriesTest();
            BenchMark_QueriesTest();
        }

        {
            ProcessQueriesJoinedTest();
            BenchMark_QueriesJoinedTest();
        }
    }

    {
        // тесты Тема 12. Урок 9. Параллелим методы поисковой системы
        {
            //Задание 1/3 RemoveDocuments
            RemoveDocumentsTest();
            BenchMark_RemoveDocumentsTest();
        }  

        {
            //Задание 2/3 MatchDocuments

            {
                //Внутренний тест для ускоренного ParseQuery
                ParseQueryTest();
                BenchMark_ParseQueryTest();
            }
            MatchDocumentsTest();
            BenchMark_MatchDocumentsTest();
        }
        
        {
            //Задание 3/3 String_View
            StringViewTest();
        }
     
    }

    {
        // тесты Тема 14. Урок 7. Параллелим поиск документов
        FindDocumentsTest();
        BenchMark_FindDocumentsTest();
    }

    return 0;
}