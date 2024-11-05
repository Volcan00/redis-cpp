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

void handle_client(int client_socket) {
  std::string accumulated_data; // To accumulate incoming data

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
    accumulated_data.append(buffer); // Accumulate data

    // Check if we have a complete message
    while(true) {
      // Find the start of the message (should start with '*')
      if(accumulated_data.empty() || accumulated_data[0] != '*') {
        break; // Not a valid message, wait for more data
      }

      size_t end_pos = accumulated_data.find("\r\n");
      if(end_pos == std::string::npos) {
        break; // Not enough data to process the message
      }
      
      // Extract the number of elements in the array
      int num_elements = atoi(accumulated_data.substr(1, end_pos - 1).c_str());

      // Move past the initial \r\n
      size_t current_pos = end_pos + 2;

      for(int i = 0; i < num_elements; ++i) {
        // Check for the bulk string prefix '$'
        if(accumulated_data[current_pos] != '$') {
          break; // Invalid command format
        }

        // Find the end of the length line for the bulk string
        end_pos = accumulated_data.find("\r\n", current_pos);
        if(end_pos == std::string::npos) {
          break; // Not enough data to read length
        }

        // Extract bulk length
        int bulk_length = atoi(accumulated_data.substr(current_pos + 1, end_pos - current_pos - 1).c_str());
        current_pos = end_pos + 2; // Move past the $length\r\n

        end_pos = current_pos + bulk_length + 2;  // Include \r\n
        if(accumulated_data.length() < end_pos) {
          break;  // Not enough data to read the full bulk string
        }
        
        // Extract the command
        std::string command = accumulated_data.substr(current_pos, bulk_length);

        // Move the pointer to the end of the bulk string
        current_pos = end_pos;

        // Respond to PING command
        if(command == "PING") {
          const char* response = "+PONG\r\n";
          send(client_socket, response, strlen(response), 0);
        }
        // Respond to ECHO command
        else if(command == "ECHO") {
          // Get the message to echo
          end_pos = accumulated_data.find("\r\n", current_pos);
          if(end_pos == std::string::npos) {
            break; // Not enough data for the echo message
          }

          bulk_length = atoi(accumulated_data.substr(current_pos + 1, end_pos - current_pos - 1).c_str()); // Get length
          current_pos = end_pos + 2; // Move past the $length\r\n

          std::string message = accumulated_data.substr(current_pos, bulk_length); // Extract the message

          // Send the echoed message back
          std::string response = "$" + std::to_string(message.length()) + "\r\n" + message + "\r\n";
          send(client_socket, response.c_str(), response.length(), 0);
        }
        // Handle unknown command
        else {
          const char* error_response = "-ERR uknown command\r\n";
          send(client_socket, error_response, strlen(error_response), 0);
        }
      }

      // Remove the processed part from accumulated_data
      accumulated_data = accumulated_data.erase(0, current_pos);
    }
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
