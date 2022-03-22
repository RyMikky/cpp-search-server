// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/* ПОПЫТКА ЧЕРЕЗ СРАВНЕНИЕ СИМВОЛОВ ЧЕРЕЗ CHAR
string RangeToString(int range) { // функция перевода числа в строку
	
	string st_range; // задаем новую переменную
	st_range = to_string(range); // переводим число в строку
	return st_range; // возвращаем значение
}

int StringSize(string st_range) { // функция вычисления размера строки
	
	int st_size; // задаем новую переменную
	st_size = st_range.size(); // вычисляем длину строки для внутреннего цикла проверки
	return st_size; // возвращаем значение
}
*/

vector<int> RangeToVec(int range) { // функция перевода числа в вектор. Будет использоваться в цикле и каждую иттерацию создавать новый вектор

	vector<int> range_vec; // задаем пустой вектор под наше четырехзначное число
	int r1, r2, r3, r4; //задаем единицы, десятки, сотни, тысячи
	if (range < 10) {
		r1 = range;
		range_vec.push_back(r1);

	} else if (range <= 99) {
		r1 = range % 10; r2 = range / 10;
		range_vec.push_back(r2); range_vec.push_back(r1);
		
	} else if (range <= 999) {
		r1 = range % 10; r2 = (range / 10) % 10; r3 = range / 100;
		range_vec.push_back(r3); range_vec.push_back(r2); range_vec.push_back(r1);

	} else if (range <= 9999) {
		r1 = range % 10; r2 = (range / 10) % 10; r3 = (range / 100) % 10; r4 = range / 1000;
		range_vec.push_back(r4); range_vec.push_back(r3); range_vec.push_back(r2); range_vec.push_back(r1);
	}
	return range_vec;
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
		for (int s : RangeToVec(zero)) {
			if (s == three) {
				++result;
			}
		}
	}
	//результат
	cout << endl << endl << result << endl << endl;
	/*
	НЕ ПОНИМАЮ, ПОЧЕМУ? ЧТО НЕ ТАК?
	Вроде считает всё верно. Вектор каждый раз обнуляется.
	Вводим диапазон, например 34, ищем 3 -> все отлично считает - 03, 13, 23, 30, 31, 32, 33, 34. Итого 8 раз.
	Вводим диапазон, например 42, ищем 3 -> считаем - 03, 13, 23, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39. Итого 13 раз. Должно быть...
	Должно быть 13, а программа выдает 14.
	*/


	/* Посмотреть вектор по range
	for (int s : RangeToVec(range)) {
		cout << "["s << s << "]"s;
	}
	*/
	/* Выдача через остаток 3 на конце
	int result2 = 0;
	for (int zero = 0; zero < range; ++zero) {
		if ((zero % 10) == three) {
			++result2;
		}
	}
	cout << endl << endl << result2;
	*/
	return 0;
}
// Закомитьте изменения и отправьте их в свой репозиторий.
