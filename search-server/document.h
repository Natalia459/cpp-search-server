#pragma once
#include <iostream>

struct Document {
	Document();
	Document(int, double, int);

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED
};

std::ostream& operator<<(std::ostream&, Document);