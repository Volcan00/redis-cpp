#ifndef RESPPARSER_H
#define RESPPARSER_H

#include <string>
#include <vector>

class RESPParser
{
private:
    std::string buffer; // Holds the incoming data
    std::vector<std::string> parsed_commands; // Stores parsed commands

    // Parse function for RESP data types
    std::string parse_bulk_string();
    void parse_array();
    
    // Helper method to parse length prefix for bulk strings and arrays
    int parse_length(const std::string& delimiter);

    // Utility method to extract data until a delimiter
    std::string parse_until(const std::string& delimiter, int length);

public:
    RESPParser();

    // Feed data into the parser
    void feed(const std::string& data);

    // Parse the next complete RESP message (if available)
    void parse();

    // Retrieves parsed commands and clears the internal storage
    std::vector<std::string> get_parsed_commands();
};


#endif