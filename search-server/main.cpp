// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>
#include <set>

using namespace std;

set<int> RangeToSet(int range) { // функция перевода числа в вектор. Будет использоваться в цикле и каждую иттерацию создавать новый вектор

	set<int> range_set; // задаем пустое множество под наше четырехзначное число
	int r1, r2, r3, r4; //задаем единицы, десятки, сотни, тысячи
	if (range < 10) {
		r1 = range;
		range_set.insert(r1);

	} else if (range <= 99) {
		r1 = range % 10; r2 = range / 10;
		range_set.insert(r2); range_set.insert(r1);
		
	} else if (range <= 999) {
		r1 = range % 10; r2 = (range / 10) % 10; r3 = range / 100;
		range_set.insert(r3); range_set.insert(r2); range_set.insert(r1);

	} else if (range <= 9999) {
		r1 = range % 10; r2 = (range / 10) % 10; r3 = (range / 100) % 10; r4 = range / 1000;
		range_set.insert(r4); range_set.insert(r3); range_set.insert(r2); range_set.insert(r1);
	}
	return range_set;
}

int main(){

	// задаем основные переменные которые можно как принудительно задать...
	int range; // диапазон поиска
	int three; // число которое ищем

	// ... так и ввести переменные через консоль, можно искать не только "3" в тысяче, а все возможные варианты поиска однозначного числа в четырёхзначном
	cin >> range;
	cin >> three;

	int result = 0; // задаем переменную-счётчик совпадений

	// делаем цикл по всей длине заданного диапазона поиска
	
	// так как задача найти все числа, в которых встречается хотя бы ОДНА тройка, то ищем одно совпадение в любом числе
	for (int zero = 0; zero < range; ++zero) {
		for (int s : RangeToSet(zero)) {
			if (s == three) {
				/* для проверки того какие цифры берутся
				cout << "zero is -"s << "["s << zero << "]"s << endl;
				cout << "S is -"s << "["s << s << "]"s << endl << endl;
				*/
				++result;
			}
		}
	}
	//результат
	cout << endl << endl << result << endl << endl;

	return 0;
}
// Закомитьте изменения и отправьте их в свой репозиторий.
