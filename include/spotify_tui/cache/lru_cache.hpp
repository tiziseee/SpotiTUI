#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <optional>
#include <mutex>
#include <chrono>

namespace spotify_tui {

template<typename T>
class LruCache {
public:
    explicit LruCache(size_t capacity, std::chrono::seconds ttl = std::chrono::seconds(300))
        : capacity_(capacity), ttl_(ttl) {}

    void put(const std::string& key, T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second.value = std::move(value);
            it->second->second.timestamp = std::chrono::steady_clock::now();
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() >= capacity_) {
            auto& oldest = items_.back();
            map_.erase(oldest.first);
            items_.pop_back();
        }
        items_.emplace_front(key, CacheEntry{std::move(value), std::chrono::steady_clock::now()});
        map_[key] = items_.begin();
    }

    std::optional<T> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        auto age = std::chrono::steady_clock::now() - it->second->second.timestamp;
        if (age > ttl_) {
            items_.erase(it->second);
            map_.erase(it);
            return std::nullopt;
        }
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second.value;
    }

    void invalidate(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            items_.erase(it->second);
            map_.erase(it);
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
        map_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

private:
    struct CacheEntry {
        T value;
        std::chrono::steady_clock::time_point timestamp;
    };

    size_t capacity_;
    std::chrono::seconds ttl_;
    std::list<std::pair<std::string, CacheEntry>> items_;
    std::unordered_map<std::string, typename std::list<std::pair<std::string, CacheEntry>>::iterator> map_;
    mutable std::mutex mutex_;
};

} // namespace spotify_tui
