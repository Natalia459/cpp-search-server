#pragma once
#include <map>
#include <vector>
#include <iterator>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
	const size_t size_maps_;
	std::vector<std::pair<std::map<Key, Value>, std::mutex>> bucket_map_;
	std::map<Key, Value> ordinary_map_;

public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	struct Access
	{
		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;

	};

	//на bucket_count подсловарей нужно разбить все пространство ключей
	explicit ConcurrentMap(size_t bucket_count) : size_maps_(bucket_count), bucket_map_(bucket_count)
	{
	}

	Access operator[](const Key& key)
	{
		uint64_t u_key = static_cast<uint64_t>(key);
		auto index = u_key % size_maps_;
		return { std::lock_guard(bucket_map_[index].second),  bucket_map_[index].first[u_key] };
	}

	std::map<Key, Value> BuildOrdinaryMap()
	{
		std::map<Key, Value> result;
		for (auto& [local_map, its_mutex] : bucket_map_)
		{
			std::lock_guard guard(its_mutex);
			result.merge(local_map);
		}
		return result;
	}

	typename std::map<Key, Value>::iterator begin()
	{
		ordinary_map_ = BuildOrdinaryMap();
		return ordinary_map_.begin();
	}

	typename std::map<Key, Value>::iterator end()
	{
		if (ordinary_map_.empty()) {
			ordinary_map_ = BuildOrdinaryMap();
		}
		return ordinary_map_.end();
	}

	size_t erase(const Key& key)
	{
		uint64_t u_key = static_cast<uint64_t>(key);
		auto index = u_key % size_maps_;

		std::lock_guard guard(bucket_map_[index].second);
		if (bucket_map_[index].first.count(key) != 0) {
			bucket_map_[index].first.erase(key);
			return 1;
		}
		return 0;
	}
};