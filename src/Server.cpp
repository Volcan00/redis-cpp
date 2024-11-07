#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <unordered_map>

#include "RESPParser.h"

void handle_client(int client_socket) {
  std::string accumulated_data;       // To accumulate incoming data
  RESPParser parser;                  // Parser instance
  std::vector<std::string> commands;  // Vector to store parsed commands as strings

  std::unordered_map<std::string, std::string> set_get_map;


  // Continuously handle client requests
  while(true) {
    char buffer[1024];

    // Receive data from the client
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if(bytes_received <= 0) {
      std::cerr << "Failed to read from client.\n"; // Exit loop if there's an error or client disconnects
      break;
    }

    // Null-terminate the received data
    buffer[bytes_received] = '\0';
    accumulated_data.append(buffer, bytes_received); // Accumulate data

    // Feed accumulated data to the parser
    parser.feed(accumulated_data);

    try {
      // Parse all available data into commands
      parser.parse();

      // Retrieve parsed commands and store them
      std::vector<std::string> new_commands = parser.get_parsed_commands();
      commands.insert(commands.end(), new_commands.begin(), new_commands.end());
    }
    catch (const std::exception& e) {
      std::cerr << "Parsing error: " << e.what() << '\n';
      break; 
    }

    if(commands[0] == "PING") {
      const char* response = "+PONG\r\n";
      send(client_socket, response, strlen(response), 0);
    }
    else if(commands[0] == "ECHO") {
      if(commands.size() != 2) {
        std::cerr << "(error) ERR wrong number of arguments for command\n";
        break;
      }
      else {
        std::string response = "$" + std::to_string(commands[1].size()) + "\r\n" + commands[1] + "\r\n";
        send(client_socket, response.c_str(), response.length(), 0);
      }
    }
    else if(commands[0] == "SET") {
      if(commands.size() != 3) {
        std::cerr << "(error) ERR wrong number of arguments for command\n";
        break;
      }
      else {
        set_get_map[commands[1]] = commands[2];

        const char* response = "+OK\r\n";
        send(client_socket, response, strlen(response), 0);
      }
    }
    else if(commands[0] == "GET") {
      if(commands.size() != 2) {
        std::cerr << "(error) ERR wrong number of arguments for command\n";
        break;
      }
      else {
        if(set_get_map.find(commands[1]) == set_get_map.end()) {
          const char* response = "$-1\r\n";
          send(client_socket, response, strlen(response), 0);
        }
        else {
          std::string response = "$" + std::to_string(set_get_map[commands[1]].size()) + "\r\n" + set_get_map[commands[1]] + "\r\n";
          send(client_socket, response.c_str(), response.length(), 0);
        }
      }
    }
    else {
      std::cerr << "(error) unknown command\n";
      break;;
    }
    // After processing, clear accumulated data and parsed commands
      accumulated_data.clear();
      commands.clear();
  }

  // Close the client socket when don
  close(client_socket);
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Create a server socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Allow reuse of the address to avoid "Address already in use" error
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  // Set up server address and port (IPv4, any IP, port 6379)
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  // Bind the server socket to the specified address and port
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }

  // Start listening for incoming connections with a backlog of 5
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  // Prepare to accept a client connection
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for clients to connect...\n";

  // Accept a connection from a client
  while(true)
  {
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if(client_fd < 0) {
      std::cerr << "Failed to accept client connection.\n";
      close(server_fd);
      return 1;
    }

    std::thread client_therad(handle_client, client_fd);
    client_therad.detach();
    std::cout << "Client connected\n";
  }

  // Close the server socket
  close(server_fd);

  return 0;
}
