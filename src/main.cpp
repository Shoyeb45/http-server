#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <set>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <zlib.h>

// For reading file
#include <fstream>
// For checking if the file exists or not
#include <sys/stat.h>

#include "utils/utils.hpp"

const std::set<std::string> supported_compression_techniques = {"gzip"};
std::thread connections[100];
std::string directory_path = "";

void accept_connection(int client_socket) {
    while (true) {
        std::vector<char> buf(5000);
        std::fill(buf.begin(), buf.end(), 0);

        std::cout << "Reading the request data\n";
        int bytes = recv(client_socket, buf.data(), buf.size(), 0);
        if (bytes <= 0) {
            close(client_socket);
            std::cout << "Reading data failed\n";
            return;
        }
        std::cout << "Successfully read the data, request data:\n" << buf.data() << "\n";

        std::string http_method = get_http_method(buf);
        std::cout << "HTTP method: " << http_method << "\n";

        std::string url = get_target_url(buf, http_method);
        std::cout << "Target URL: " << url << "\n";

        std::string requests(buf.data(), bytes);
        bool should_close = requests.find("Connection: close") != std::string::npos;

        std::cout << "Should close " << should_close << "\n";

        std::cout << "Requests: " << requests << "\n";
        int idx_of_encoding = requests.find("Accept-Encoding");
        std::cout << "Encoding: " << idx_of_encoding << "\n";
        if (url.starts_with("/echo/")) {
            std::cout << "Url with echo\n";
            int idx = url.find_last_of('/');
            std::string temp = url.substr(idx + 1);
            std::cout << "URL Param in echo: " << temp << "\n";

            if (idx_of_encoding != -1) {
                int last_i = requests.find("\r\n", idx_of_encoding + 1);
                int first_i = requests.find(":", idx_of_encoding) + 2;
                std::string enc_typs = requests.substr(first_i, last_i - first_i);
                std::vector<std::string> encoding_types = split_str(enc_typs, ", ");

                std::vector<std::string> valid_encoding;
                for (auto &encoding : encoding_types) {
                    if (supported_compression_techniques.find(encoding) != supported_compression_techniques.end())
                        valid_encoding.push_back(encoding);
                }

                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain";

                if (valid_encoding.size() >= 1) {
                    response += "\r\nContent-Encoding: ";
                    for (int i = 0; i < (int)valid_encoding.size(); i++) {
                        response += valid_encoding[i];
                        if (i < (int)valid_encoding.size() - 1) {
                            response += ", ";
                        }
                    }
                }

                std::string compressed_data = encode_using_gzip(temp);
                response += "\r\nContent-Length: " + std::to_string(compressed_data.size());
                response += "\r\n\r\n" + compressed_data;

                std::cout << "Response for echo in encoding::\n" << response << "\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Connection: keep-alive\r\n"
                                   "Content-Type: text/plain\r\nContent-Length: ";

            response += std::to_string(temp.size());
            response += "\r\n\r\n" + temp;

            std::cout << "Response in echo without encoding: " << response << "\n";
            send(client_socket, response.c_str(), response.size(), 0);
            continue;
        }

        if (url.starts_with("/user-agent")) {
            std::cout << "URL starting with /user-agent\n";

            int idx_of_user = requests.find("User-Agent");
            int idx = requests.find(":", idx_of_user);

            std::string str_prm = "";

            for (int i = idx + 2;; i++) {
                if (requests[i] == '\r' && requests[i + 1] == '\n')
                    break;
                str_prm += requests[i];
            }
            std::cout << "User agent from header: " << str_prm << "\n";

            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Connection: keep-alive\r\n"
                                   "Content-Type: text/plain\r\nContent-Length: ";
            response += std::to_string(str_prm.size());
            response += "\r\n\r\n" + str_prm;
            std::cout << "Response for user-agent:\n" << response << "\n";

            send(client_socket, response.c_str(), response.size(), 0);
            continue;
        }

        if (url == "/") {
            send(client_socket, "HTTP/1.1 200 OK\r\n\r\n", 20, 0);
            continue;
        }

        if (url.starts_with("/files/")) {
            int idx = url.find_last_of('/');
            std::string file_name = url.substr(idx + 1);
            std::string path = directory_path + file_name;

            if (http_method == "POST") {
                std::ofstream file(path);
                file << get_request_body(requests);
                std::string response = "HTTP/1.1 201 Created\r\n\r\n";
                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }

            struct stat md;
            int is_file_exists = stat(path.c_str(), &md);

            std::cout << "File path: " << path << " is file exits: " << is_file_exists << "\n";
            if (is_file_exists == 0) {
                std::ifstream file(path);
                std::string file_content;

                getline(file, file_content);
                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: ";
                response += std::to_string(file_content.size());
                response += "\r\n\r\n" + file_content;

                send(client_socket, response.c_str(), response.size(), 0);
                continue;
            }
        }

        send(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n", 27, 0);
        if (should_close) {
            close(client_socket);
            return;
        }
    }
}

std::string get_directory(char **argv, int argc) {
    int i = 0;

    for (; i < argc; i++) {
        std::string temp = argv[i];
        if (temp == "--directory") {
            break;
        }
    }

    std::string directory = "";

    if (i + 1 < argc) {
        directory = argv[i + 1];
    }
    return directory;
}

int main(int argc, char **argv) {
    directory_path = get_directory(argv, argc);

    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    struct sockaddr_in client_addr;

    int i = 0;
    while (true) {
        std::cout << "Waiting for a client to connect...\n";
        int client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

        if (client_socket < 0) {
            std::cerr << "Cannot accept connection.\n";
            exit(1);
        }
        // inet_ntoa gives ip of the client.
        std::cout << "Client with ip " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port)
                  << " connected on socket : " << client_socket << "\n";
        connections[i++] = std::thread(accept_connection, client_socket);
    }

    // close(server_fd);

    return 0;
}
