#include "ServerHelperFunctions.h"

void send_response(int client_socket, const std::string& response) {
    send(client_socket, response.c_str(), response.length(), 0);
}

void send_error(int client_socket, const char* error_msg) {
    send(client_socket, error_msg, strlen(error_msg), 0);
}

ssize_t receive_data(int client_socket, std::string& accumulated_data) {
  char buffer[1024];

    // Receive data from the client
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if(bytes_received > 0) {
      accumulated_data.append(buffer, bytes_received);
    }

    return bytes_received;
}

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
    else if(commands[0] == "KEY") {
        if(commands.size() != 2) {
        send_error(client_socket, "(error) ERR wrong number of arguments for command\r\n");
        return false;
        }
        // Check if there are sufficient command-line arguments (argc should be at least 5)
        if(argc < 5) {
        std::string error_msg = "(error) ERR missing command-line arguments.\n";
        error_msg += "Expected usage: ./your_program.sh --dir <directory> --dbfilename <filename>\n";
        send_error(client_socket, error_msg.c_str());
        return false;
        }

        std::string filename = argv[4];
        
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