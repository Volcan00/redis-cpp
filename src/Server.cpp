#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Create a server socket using IPv4 (AF_INET) and TCP (SOCK_STREAM)
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1; // Exit if socket creation fails
  }
  
  // Set socket option to reuse the address, allowing quick restart after a crash
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  // Define server address and port
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;         // Use IPv4
  server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any available network interface
  server_addr.sin_port = htons(6379);       // Bind to port 6379, typically used by Redis
  
  // Bind the socket to the specified IP address and port
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  // Set the socket to listen for incoming connections with a backlog of 5
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  // Prepare a struct to store the client's address information
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
   // Indicate server is waiting for a client connection
  std::cout << "Waiting for a client to connect...\n";
  
  // Accept a client connection (this call blocks until a connection is made)
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if(client_fd < 0) {
    std::cerr << "Failed to accept client connection\n";
    close(server_fd);
    return 1;
  }
  std::cout << "Client connected\n";

  // Send the hardcoded response +PONG\r\n to the client
  const char* response = "+PONG\r\n";
  send(client_fd, response, strlen(response), 0);
  
  // Close the client socket and the server socket
  close(client_fd);
  close(server_fd);

  return 0;
}
