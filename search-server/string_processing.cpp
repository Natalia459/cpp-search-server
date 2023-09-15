#include "string_processing.h"
#include "log_duration.h"

#include <algorithm>
#include <iostream>
#include <string_view>

std::list<std::string_view> SplitIntoWords(std::string_view text)
{
	std::list<std::string_view> words;

	size_t pos = text.find_first_not_of(' ');
	size_t text_end = text.npos;

	while (pos != text_end) {
		size_t space = text.find(' ', pos);
		words.push_back(space == text_end ? text.substr(pos) : text.substr(pos, space - pos));
		pos = text.find_first_not_of(' ', space);
	}

	//int spaces = 0;
	//int length = 0;
	//std::for_each(text.begin(), text.end(),
	//	[&](const char& c) {
	//		if (c == ' ') {
	//			if (length) {
	//				words.push_back(text.substr(spaces, length));
	//				spaces += length;
	//				length = 0;
	//			}
	//			++spaces;

	//		}
	//		else {
	//			++length;
	//		}
	//	});

	//if (length) {
	//	words.push_back(text.substr(spaces, length));
	//}

	return words;
}

//std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const std::vector<std::string_view>& strings)
//{
//	std::set<std::string, std::less<>> non_empty_strings;
//	for (const auto& str : strings)
//	{
//		if (!str.empty())
//		{
//			non_empty_strings.insert(std::string(str));
//		}
//	}
//	return non_empty_strings;
//}