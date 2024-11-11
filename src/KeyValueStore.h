#ifndef KEYVALUESTORE_H
#define KEYVALUESTORE_H

#include <string>
#include <unordered_map>
#include <mutex>

class KeyValueStore {
private:
    std::unordered_map<std::string, std::string> data;
    std::mutex map_mutex;

public:
    // Set a key with optional expiry in milliseconds
    void set(const std::string& key, const std::string& value, int expiry_time_in_milliseconds = -1);

    // Get a value by key
    std::string get(const std::string& key);

    // Check if a key exists
    bool exists(const std::string& key);
};

#endif