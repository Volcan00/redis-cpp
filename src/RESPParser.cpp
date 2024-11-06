#include "RESPParser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

RESPParser::RESPParser() {}

void RESPParser::feed(const std::string& data) {
    buffer += data; // Append incoming data to the buffer
}

void RESPParser::parse() {
    while (!buffer.empty()) {
        if (buffer[0] == '$') {
            parsed_commands.push_back(parse_bulk_string());
        } else if (buffer[0] == '*') {
            parse_array();
        } else {
            std::cerr << "Error: Unknown RESP type at buffer: " << buffer << std::endl;
            throw std::runtime_error("Unknown RESP type");
        }
    }
}

std::vector<std::string> RESPParser::get_parsed_commands() {
    std::vector<std::string> commands = parsed_commands;
    parsed_commands.clear();
    return commands;
}

std::string RESPParser::parse_bulk_string() {
    int length = parse_length("\r\n");
    if (length == -1) return "(null)";   // Represents a null bulk string
    if (length == 0) return "";          // Represents an empty bulk string
    return parse_until("\r\n", length);   // Extract the bulk string data
}

void RESPParser::parse_array() {
    int count = parse_length("\r\n");

    for (int i = 0; i < count; ++i) {
        if (buffer[0] == '$') {
            parsed_commands.push_back(parse_bulk_string());
        } else {
            throw std::runtime_error("Unexpected format in array.");
        }
    }
}

int RESPParser::parse_length(const std::string& delimiter) {
    size_t end = buffer.find(delimiter);
    
    if (end == std::string::npos) {
        throw std::runtime_error("Incomplete data.");
    }

    std::string lengthStr = buffer.substr(1, end - 1); // Skip '$' and read length
    buffer.erase(0, end + delimiter.length()); // Remove the length and "\r\n" from the buffer

    if (lengthStr.empty()) {
        throw std::runtime_error("Invalid length format.");
    }

    try {
        return std::stoi(lengthStr); // Convert the length to an integer
    } catch (const std::invalid_argument& e) {
        throw std::runtime_error("Invalid length value.");
    }
}

std::string RESPParser::parse_until(const std::string& delimiter, int length) {
    size_t end = buffer.find(delimiter);

    if (end == std::string::npos) {
        throw std::runtime_error("Invalid data.");
    }

    std::string result = buffer.substr(0, length);
    buffer.erase(0, length + delimiter.length());

    return result;
}