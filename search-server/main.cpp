#include <iostream>

using namespace std;

main() {
	int number = 1, counter = 0;
	string string_number;
	for (int i = 0; i < 1000; i++) {			// Проходим в цикле все числа от 1 до 1000
		string_number = to_string(number);		// Каждое число переводим в тип string
		for (char sign : string_number) {		// Поэлементно ищем символ 3
			if (sign == '3') {
				++counter;
				break;							//Если цифра 3 найдена, выходим из цикла с поиском символа
			}
		}
	}

	cout << "From 1 to 1000 equal " << counter << "numbers have at least one figure 3." << endl;
}