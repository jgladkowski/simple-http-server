#include <iostream>
#include <filesystem>
#include <algorithm>
#include <set>
#include <map>
#include <fstream>

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/signal.h>

#include "http_parser.h"

constexpr int PORT_DEFAULT = 8080;
constexpr int PORT_MAX = 65535;

namespace {
    // Checks whether the path contains only valid characters
    bool has_correct_path_characters(std::string const& path) {
        for (int i = 0; i < path.size(); ++i) {
            if ((path[i] < '0' || path[i] > '9') &&
                (path[i] < 'a' || path[i] > 'z') &&
                (path[i] < 'A' || path[i] > 'Z') &&
                path[i] != '.' &&
                path[i] != '/' &&
                path[i] != '-')
                return false;
        }
        return true;
    }

    // Checks whether the path exits the main directory at any prefix
    bool exits_server_directory(std::string const& path) {
        int depth = 0;
        for (int i = 0; i < path.size(); ++i) {
            if (path[i] == '/') {
                if (i + 3 < path.size() && path[i + 1] == '.' &&
                    path[i + 2] == '.' && path[i + 3] == '/')
                    --depth;
                else if (i + 1 == path.size() || path[i + 1] != '/')
                    ++depth;

                if (depth < 0)
                    return true;
            }
        }
        return false;
    }

    // Tries to open a file
    FILE *find_file_in_directory(std::string const& file_path) {
        const std::filesystem::path file = file_path;
        if (std::filesystem::exists(file) && !std::filesystem::is_directory(file)) {
            FILE *file_ptr = fopen(file_path.c_str(), "r");
            return file_ptr;
        }
        return nullptr;
    }

    // For sending HTTP 302
    void send_redirect(int sock, std::string const& new_addr) {
        int bytes_sent = 0;
        const std::string redirect = "HTTP/1.1 302 Found\r\nLocation: " + new_addr + "\r\n\r\n";
        while (bytes_sent < redirect.size())
            bytes_sent += write(sock, redirect.c_str() + bytes_sent, redirect.size() - bytes_sent);
    }

    // For sending HTTP 200
    void send_file(int sock, FILE *file, std::string const& method) {
        // Send status line and content type
        int bytes_sent = 0;
        const std::string status_line_content_type = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n";
        while (bytes_sent < status_line_content_type.size())
            bytes_sent += write(sock, status_line_content_type.c_str() + bytes_sent, status_line_content_type.size() - bytes_sent);

        // Find number of bytes to sent in message body
        int file_length = 0;
        if (file != nullptr) {
            fseek(file, 0, SEEK_END);
            file_length = ftell(file);
            rewind(file);
        }
        
        // Send the length of message body
        bytes_sent = 0;
        const std::string content_length_header = "Content-Length: " + std::to_string(file_length) + "\r\n\r\n";
        while (bytes_sent < content_length_header.size())
            bytes_sent += write(sock, content_length_header.c_str() + bytes_sent, content_length_header.size() - bytes_sent);

        // Send the message body if the method is GET
        if (method == "GET") {
            char buffer[file_length];
            fread(buffer, 1, file_length, file);

            bytes_sent = 0;
            while (bytes_sent < file_length)
                bytes_sent += write(sock, buffer + bytes_sent, file_length - bytes_sent);
        }
    }

    // For sending HTTP responses other that 200 and 302
    void send_response(int sock, int status_code, std::string const& reason, bool close_flag) {
        // Send the status line
        int bytes_sent = 0;
        const std::string status_line = "HTTP/1.1 " + std::to_string(status_code) + " " + reason + "\r\n";
        while (bytes_sent < status_line.size())
            bytes_sent += write(sock, status_line.c_str() + bytes_sent, status_line.size() - bytes_sent);

        // Optionally sending Connection: close
        if (close_flag) {
            bytes_sent = 0;
            const std::string close_line = "Connection: close\r\n";
            while (bytes_sent < close_line.size())
                bytes_sent += write(sock, close_line.c_str() + bytes_sent, close_line.size() - bytes_sent);
        }

        // Sending CRLF
        bytes_sent = 0;
        const std::string finish_crlf = "\r\n";
        while (bytes_sent < finish_crlf.size())
            bytes_sent += write(sock, finish_crlf.c_str() + bytes_sent, finish_crlf.size() - bytes_sent);
    }

    // Returns a set with all files in the main directory
    std::set<std::filesystem::path> get_all_files_in_directory(std::filesystem::path const& directory) {
        std::set<std::filesystem::path> all_files;
        for (const auto &file : std::filesystem::directory_iterator(directory)) {
            if (std::filesystem::is_directory(file))
                all_files.merge(get_all_files_in_directory(file));
            else
                all_files.insert(file);
        }
        return all_files;
    }

    // Parses the file with associated servers and returns a map from old path to new path
    std::map<std::filesystem::path, std::string> get_associated_servers(std::string const& filename) {
        std::map<std::filesystem::path, std::string> associated_servers;
        std::ifstream ifs(filename);
        std::string path, server, port;
        while (ifs >> path >> server >> port) {
            std::ostringstream oss;
            oss << "http://" << server << ":" << port << path << std::endl;
            if (associated_servers.count(path) == 0)
                associated_servers[path] = oss.str();
        }
        return associated_servers;
    }
}

// Global sockets to properly handle SIGINT
int sock = -1;
int msg_sock = -1;

// Handling SIGINT
void sigint_handler(int sig) {
    if (msg_sock != -1) {
        send_response(msg_sock, 500, "Internal Server Error", true);
        if (close(msg_sock) == -1)
            std::cerr << "Error while closing socket!" << std::endl;
    }
    if (sock != -1 && close(sock) == -1)
        std::cerr << "Error while closing socket!" << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    // Handling sigint and ignoring sigpipe
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);

    // Checking number of arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Incorrect number of arguments!" << std::endl;
        return EXIT_FAILURE;
    }

    // First argument - files directory
    const std::filesystem::path server_dir_path = argv[1];
    if (!std::filesystem::exists(server_dir_path) || !std::filesystem::is_directory(server_dir_path)) {
        std::cerr << "Specified directory doesn't exist!" << std::endl;
        return EXIT_FAILURE;
    }

    // Second argument - file with associated servers
    const std::filesystem::path servers_file_path = argv[2];
    if (!std::filesystem::exists(servers_file_path) || std::filesystem::is_directory(servers_file_path)) {
        std::cerr << "Associated servers file doesn't exist!" << std::endl;
        return EXIT_FAILURE;
    }

    // Third argument - port
    int port = PORT_DEFAULT;
    if (argc == 4) {
        int port_str_length = strlen(argv[3]);
        for (int i = 0; i < port_str_length; i++) {
            if (argv[3][i] < '0' || argv[3][i] > '9') {
                std::cerr << "Error! Port must be a number!" << std::endl;
                return EXIT_FAILURE;
            }
        }
        port = atoi(argv[3]);
    }
    if (port > PORT_MAX) {
        std::cerr << "Error! Port number too large!" << std::endl;
        return EXIT_FAILURE;
    }

    // Saving all files from the directory
    std::set<std::filesystem::path> all_files = get_all_files_in_directory(server_dir_path);

    // Saving associated servers
    std::map<std::filesystem::path, std::string> associated_servers = get_associated_servers(argv[2]);

    // Creating socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Error while creating socket!" << std::endl;
        return EXIT_FAILURE;
    }

    // Binding the socket to address
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&server, (socklen_t)sizeof(server)) == -1) {
        std::cerr << "Error while binding socket to address!" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Listening for clients
    if (listen(sock, 5) == -1) {
        std::cerr << "Error while listening!" << std::endl;
        return EXIT_FAILURE;
    }

    // Serving clients until ^C is pressed
    while (true) {
        // Accept new client
        msg_sock = accept(sock, (struct sockaddr*)0, (socklen_t*)0);
        if (msg_sock == -1) {
            std::cerr << "Error while accepting connection!" << std::endl;
            return EXIT_FAILURE;
        }
        HttpParser parser(msg_sock);

        // Serving one client in an infinite loop
        while (true) {
            // Info from client
            std::string method;
            std::string path;
            bool close_flag = false;
            int status_code = parser.read_one_request(method, path, close_flag);

            // Status codes that always close the connection
            if (status_code == 500) {
                send_response(msg_sock, status_code, "Internal Server Error", true);
                break;
            }
            if (status_code == 400) {
                send_response(msg_sock, status_code, "Bad Request", true);
                break;
            }

            // Status codes that don't close the connection
            if (method != "GET" && method != "HEAD")
                send_response(msg_sock, 501, "Not Implemented", false);
            else if (path.size() == 0 || path[0] != '/' ||
                !has_correct_path_characters(path) || exits_server_directory(path))
                send_response(msg_sock, 404, "Not Found", false);
            else {
                // Looking for the requested file in the directory
                FILE *requested_file = find_file_in_directory(argv[1] + path);
                if (requested_file != nullptr) {
                    send_file(msg_sock, requested_file, method);
                    fclose(requested_file);
                }
                // Looking in saved files set
                else if (all_files.count(argv[1] + path))
                    send_file(msg_sock, nullptr, method);
                // Looking in the associated servers
                else if (associated_servers.count(path) == 1)
                    send_redirect(msg_sock, associated_servers[path]);
                // Not found
                else
                    send_response(msg_sock, 404, "Not Found", false);
            }

            if (close_flag)
                break;
        }

        if (close(msg_sock) == -1) {
            std::cerr << "Error while closing socket!" << std::endl;
            return EXIT_FAILURE;
        }
        msg_sock = -1;
    }

    return 0;
}