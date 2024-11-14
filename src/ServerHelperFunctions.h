#ifndef SERVER_HELPER_FUNCTIONS_H
#define SERVER_HELPER_FUNCTIONS_H

#include "RESPParser.h"
#include "KeyValueStore.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

// Helper function to send responses
void send_response(int client_socket, const std::string& response);

// Helper function to send error messages
void send_error(int client_socket, const char* error_msg) {
    send(client_socket, error_msg, strlen(error_msg), 0);
}

// Function to receive data from the client
ssize_t receive_data(int client_socket, std::string& accumulated_data);

// Function to process accumulated data with the parser
void process_commands(std::string& accumulated_data, std::vector<std::string>& commands, RESPParser& parser);

// Function to handle each command and respond to the client
bool handle_command(int client_socket, const std::vector<std::string>& commands, int argc, char** argv, KeyValueStore& store);

void handle_client(int client_socket, int argc, char** argv);

#endif