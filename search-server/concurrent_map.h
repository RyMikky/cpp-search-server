#pragma once
#include <mutex>
#include <map>
#include <vector>
#include <algorithm>


using namespace std::string_literals;


template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Part {
        std::map<Key, Value> data_map_;
        std::mutex mutex_;
    };

    struct Access {
        std::lock_guard<std::mutex> ref_guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : data_massive_(bucket_count) {
    };

    Access operator[](const Key& key) {
        Part& part = PartSelect(key);
        return { std::lock_guard(part.mutex_), part.data_map_[key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        std::for_each(data_massive_.begin(), data_massive_.end(),
            [&result](auto& unit) {
                std::lock_guard defender(unit.mutex_);
                result.merge(unit.data_map_);
            });
        return result;
    };

    void EraseMap() {
        data_massive_.clear();
    }

    void EraseKey(const Key& key) {
        Part& part = PartSelect(key);
        std::lock_guard defender(part.mutex_);
        part.data_map_.erase(key);
    }

private:

    std::vector<Part> data_massive_;

    Part& PartSelect(const Key& key) {
        return data_massive_[static_cast<uint64_t>(key) % data_massive_.size()];
    }
};