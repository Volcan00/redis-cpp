#include <RdbParser.h>

#include<iostream>

RdbParser::RdbParser(const std::string& filename) : file(filename, std::ios::binary) {
    if(!file.is_open()) {
        throw std::runtime_error("Failed to open file.");
    }
}

void RdbParser::parse() {
    try
    {
        parse_header();
        while(file) {
            parse_entry();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error during parsing: " << e.what() << '\n';
    }
}

const KeyValueStore& RdbParser::get_keys_with_expiration() const {
    return keys_with_expiration;
}

void RdbParser::parse_header() {
    char header[9];
    file.read(header, 9);
    if(file.gcount() != 9 || std::string(header, 5) != "REDIS") {
        throw std::runtime_error("Invalid RDB header.");
    }

    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    std::cout << "RDB Version: " << version << '\n';
}

void RdbParser::parse_entry() {
    uint8_t type;
    file.read(reinterpret_cast<char*>(&type), sizeof(type));
    if(file.gcount() != sizeof(type)) {
        throw std::runtime_error("Failed to read entry type.");
    }

    switch (type)
    {
    case 0xFA:
        parse_metadata();
        break;
    case 0xFE:
        parse_database();
        break;
    case 0xFC:
    case 0xFD:
        parse_key_with_expiration(type);
        break;
    default:
        throw std::runtime_error("Unsupported RDB entry type.");
    }
}

void RdbParser::parse_metadata() {
    uint8_t name_length;
    file.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));
    std::vector<char> name(name_length);
    file.read(name.data(), name_length);

    uint8_t value_length;
    file.read(reinterpret_cast<char*>(&value_length), sizeof(value_length));
    std::vector<char> value(value_length);
    file.read(value.data(), value_length);

    std::string metadata_key(name.begin(), name.end());
    std::string metadata_value(value.begin(), value.end());

    std::cout << "Metadata - " << metadata_key << ": " << metadata_value << '\n';
}

void RdbParser::parse_database() {
    uint8_t db_index;
    file.read(reinterpret_cast<char*>(&db_index), sizeof(db_index));

    uint8_t total_key_num;
    file.read(reinterpret_cast<char*>(&total_key_num), sizeof(total_key_num));

    uint8_t keys_with_expiration_num;
    file.read(reinterpret_cast<char*>(&keys_with_expiration_num), sizeof(keys_with_expiration_num));
    std::cout << "Database index: " << (int)db_index << "\nTotal keys in database: " << (int)total_key_num << "\nNumber of keys with expiration: " << (int)keys_with_expiration_num;
}

void RdbParser::parse_key_with_expiration(uint8_t type) {
    uint8_t length;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    std::vector<char> key(length);
    file.read(key.data(), length);

    std::string key_str(key.begin(), key.end());

    uint64_t expire_timestamp = 0;

    if(type == 0xFC) {
        file.read(reinterpret_cast<char*>(&expire_timestamp), sizeof(expire_timestamp));
        expire_timestamp = le64toh(expire_timestamp);
        int expiration_time_ms = static_cast<int>(expire_timestamp - std::chrono::system_clock::now().time_since_epoch().count() / 1000);
        keys_with_expiration.set(key_str, "", expiration_time_ms);
    }
    else if(type == 0xFC) {
        file.read(reinterpret_cast<char*>(&expire_timestamp), sizeof(expire_timestamp));
        expire_timestamp = le64toh(expire_timestamp);
        int expiration_time_s = static_cast<int>(expire_timestamp - std::chrono::system_clock::now().time_since_epoch().count());
        keys_with_expiration.set(key_str, "", expiration_time_s, false);
    }

    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    file.ignore(length);
}