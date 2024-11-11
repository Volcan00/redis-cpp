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
#include "KeyValueStore.h"

// Helper function to send responses
void send_response(int client_socket, const std::string& response) {
    send(client_socket, response.c_str(), response.length(), 0);
}

// Helper function to send error messages
void send_error(int client_socket, const char* error_msg) {
    send(client_socket, error_msg, strlen(error_msg), 0);
}

// Function to receive data from the client
ssize_t receive_data(int client_socket, std::string& accumulated_data) {
  char buffer[1024];

    // Receive data from the client
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if(bytes_received > 0) {
      accumulated_data.append(buffer, bytes_received);
    }

    return bytes_received;
}

// Function to process accumulated data with the parser
void process_commands(std::string& accumulated_data, std::vector<std::string>& commands, RESPParser& parser) {
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
    }
}

// Function to handle each command and respond to the client
bool handle_command(int client_socket, const std::vector<std::string>& commands, int argc, char** argv, KeyValueStore& store) {
    if (commands[0] == "PING") {
        send_response(client_socket, "+PONG\r\n");
    }
    else if (commands[0] == "ECHO") {
        if (commands.size() != 2) {
            send_error(client_socket, "(error) ERR wrong number of arguments for command\r\n");
            return false;
        }
        std::string response = "$" + std::to_string(commands[1].size()) + "\r\n" + commands[1] + "\r\n";
        send_response(client_socket, response);
    }
    else if (commands[0] == "SET") {
        if (commands.size() < 3) {
            send_error(client_socket, "(error) ERR wrong number of arguments for command\r\n");
            return false;
        }
        else if (commands.size() == 3) {
            store.set(commands[1], commands[2]);
            send_response(client_socket, "+OK\r\n");
        }
        else if (commands.size() == 5 && commands[3] == "px") {
            int expiry;
            try {
                expiry = std::stoi(commands[4]);
            }
            catch (...) {
                send_error(client_socket, "(error) ERR invalid expiry time in PX\r\n");
                return false;
            }
            store.set(commands[1], commands[2], expiry);
            send_response(client_socket, "+OK\r\n");
        }
        else {
            send_error(client_socket, "(error) ERR syntax error\r\n");
            return false;
        }
    }
    else if (commands[0] == "GET") {
        if (commands.size() != 2) {
            send_error(client_socket, "(error) ERR wrong number of arguments for command\r\n");
            return false;
        }
        if (!store.exists(commands[1])) {
            send_response(client_socket, "$-1\r\n");
        }
        else {
            std::string value = store.get(commands[1]);
            std::string response = "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
            send_response(client_socket, response);
        }
    }
    else if(commands[0] == "CONFIG" && commands[1] == "GET") {
      if(commands.size() < 3) {
        send_error(client_socket, "(error) ERR wrong number of arguments for command\r\n");
        return false;
      }
      // Check if there are sufficient command-line arguments (argc should be at least 5)
      if(argc < 5) {
        // Send an error message to the client
        std::string error_msg = "(error) ERR missing command-line arguments.\n";
        error_msg += "Expected usage: ./your_program.sh --dir <directory> --dbfilename <filename>\n";
        send_error(client_socket, error_msg.c_str());
        return false;
      }

      if(commands[2] == "dir") {
        std::string response = "*2\r\n$3\r\ndir\r\n$" + std::to_string(strlen(argv[2])) + "\r\n" + argv[2] + "\r\n";
        send_response(client_socket, response); 
      }
      else if(commands[2] == "dbfilename") {
        std::string response = "*2\r\n$10\r\n$" + std::to_string(strlen(argv[4])) + "\r\n" + argv[4] + "\r\n";
        send_response(client_socket, response);
      }
    }
    else {
        std::cerr << "(error) unknown command\n";
        return false;
    }
    return true;
}

void handle_client(int client_socket, int argc, char** argv) {
  RESPParser parser;    // Parser instance
  KeyValueStore store;  // Store instance

  std::string accumulated_data;
  std::vector<std::string> commands;

  // Continuously handle client requests
  while(true) {
    ssize_t bytes_received = receive_data(client_socket, accumulated_data);

    if (bytes_received <= 0) {
      std::cerr << "Failed to read from client.\n";
      break;
    }

    // Process data and parse commands
    process_commands(accumulated_data, commands, parser);

    // Execute command and respond
    if (!commands.empty()) {
        if (!handle_command(client_socket, commands, argc, argv, store)) {
            break;
        }
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

    std::thread client_therad(handle_client, client_fd, argc, argv);
    client_therad.detach();
    std::cout << "Client connected\n";
  }

  // Close the server socket
  close(server_fd);

  return 0;
}
