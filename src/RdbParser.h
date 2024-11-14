#ifndef RDBPARSER_H
#define RDBPARSER_H

#include "KeyValueStore.h"
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

class RdbParser {
public:
    // Constructor that accepts the RDB file name.
    RdbParser(const std::string& filename);

    // Method to parse the RDB file.
    void parse();

    // Getter to retrieve keys with their expiration timestamps.
    const KeyValueStore& get_keys_with_expiration() const;

private:
    std::ifstream file;  // Input file stream to read the RDB file.
    KeyValueStore keys_with_expiration ;  // Key -> Expiration timestamp.

    // Helper method to parse the RDB header.
    void parse_header();

    // Helper method to parse an individual entry in the RDB file.
    void parse_entry();

    // Helper method to parse metadata (e.g., Redis version).
    void parse_metadata();

    // Helper method to parse database index.
    void parse_database();

    // Helper method to parse key-value pairs with expiration.
    void parse_key_with_expiration(uint8_t type);
};

#endif // RDBPARSER_H