#pragma once
#include <vector>
#include <iterator>
#include <iostream>

template <typename Iterator>
class IteratorRange {
public:
	IteratorRange(Iterator range_begin, Iterator range_end)
		:page_begin(range_begin), page_end(range_end)
	{
	}

	Iterator begin() const {
		return page_begin;
	}

	Iterator end() const {
		return page_end;
	}

	size_t size() const {
		return distance(page_begin, page_end);
	}

private:
	Iterator page_begin;
	Iterator page_end;
};

template <typename Iterator>
class Paginator {
public:
	Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
		Iterator it;
		for (it = range_begin; static_cast<size_t>(distance(it, range_end)) > page_size; advance(it, page_size)) {
			Iterator next_it = it;
			advance(next_it, page_size);
			diapason.push_back(IteratorRange(it, next_it));
		}
		diapason.push_back(IteratorRange(it, range_end));
	}

	auto begin() const {
		return diapason.begin();
	}

	auto end() const {
		return diapason.end();
	}

private:
	std::vector<IteratorRange<Iterator>> diapason;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, IteratorRange<Iterator> page) {
	if (page.size() != 0) {
		for (auto it = page.begin(); it < page.end(); ++it) {
			output << *it;
		}
	}
	return output;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(c.begin(), c.end(), page_size);
}
