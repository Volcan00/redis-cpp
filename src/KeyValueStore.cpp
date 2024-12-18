#include "KeyValueStore.h"
#include <thread>
#include <chrono>

void KeyValueStore::set(const std::string& key, const std::string& value, int expiry_time, bool is_milliseconds) {
    {
        std::lock_guard<std::mutex> lock(map_mutex);
        data[key] = value;
    }

    // If expiry is set, start a timer thread to remove the key
    if(expiry_time > 0) {
        std::thread([this, key, expiry_time, is_milliseconds]() {
            if(is_milliseconds)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(expiry_time));
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(expiry_time));
            }
            std::lock_guard<std::mutex> lock(map_mutex);
            data.erase(key);
        }).detach();
    }
}

std::string KeyValueStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(map_mutex);
    auto it = data.find(key);
    return (it != data.end()) ? it->second : "";
}

bool KeyValueStore::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(map_mutex);
    return data.find(key) != data.end();
}