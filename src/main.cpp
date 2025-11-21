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

#define OK 200
#define NOT_FOUND 404
#define CREATED 201

const std::set<std::string> supported_compression_techniques = {"gzip"};
std::thread connections[100];
std::string directory_path = "";

void add_crlf(std::string &response_body) {
    response_body.append("\r\n");
}

void add_status_code(int status_code, std::string &response_body) {
    switch (status_code) {
    case OK:
        response_body.append("200 OK");
        break;

    case NOT_FOUND:
        response_body.append("404 Not Found");
        break;
    case CREATED:
        response_body.append("201 Created");
        break;
    }
    add_crlf(response_body);
}

void add_content_type(std::string &response_body, std::string content_type) {
    response_body.append("Content-Type: ");
    response_body.append(content_type);
    add_crlf(response_body);
}

void add_encodings(std::string &response_body, std::vector<std::string> &encodings) {
    if (encodings.size() < 1)
        return;
    response_body.append("Content-Encoding: ");

    for (int i = 0; i < (int)encodings.size(); i++) {
        response_body.append(encodings[i]);
        if (i < (int)encodings.size() - 1) {
            response_body += ", ";
        }
    }
    add_crlf(response_body);
}

void add_content(std::string &response_body, std::string content) {
    int sz = content.size();
    response_body.append("Content-Length: ");
    response_body.append(std::to_string(sz));

    add_crlf(response_body);
    add_crlf(response_body);
    response_body.append(content);
}

void add_connection(std::string &response_body, bool should_close) {
    if (should_close) {
        response_body.append("Connection: ");
        response_body.append("close");
        add_crlf(response_body);
    }
}

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
        std::string response_body = "HTTP/1.1 ";

        std::cout << "Successfully read the data.\n";

        std::string requests(buf.data(), bytes);
        std::cout << "Request headers and body from the client: \n" << requests << "\n";

        std::string http_method = get_http_method(buf);
        std::cout << "HTTP method: " << http_method << "\n";

        std::string url = get_target_url(buf, http_method);
        std::cout << "Target URL: " << url << "\n";

        bool should_close = requests.find("Connection: close") != std::string::npos;
        std::cout << "Should close " << should_close << "\n";

        int idx_of_encoding = requests.find("Accept-Encoding");
        std::cout << "Encoding: " << idx_of_encoding << "\n";

        if (url.starts_with("/echo/")) {
            std::cout << "Url with echo\n";

            int idx = url.find_last_of('/');
            std::string temp = url.substr(idx + 1);

            add_status_code(OK, response_body);
            add_connection(response_body, should_close);
            std::cout << "URL Param in echo: " << temp << "\n";

            if (idx_of_encoding != -1) {
                int last_i = requests.find("\r\n", idx_of_encoding + 1);
                int first_i = requests.find(":", idx_of_encoding) + 2;
                std::string enc_typs = requests.substr(first_i, last_i - first_i);
                std::vector<std::string> encoding_types = split_str(enc_typs, ", ");

                std::vector<std::string> valid_encodings;

                for (auto &encoding : encoding_types) {
                    if (supported_compression_techniques.find(encoding) != supported_compression_techniques.end())
                        valid_encodings.push_back(encoding);
                }

                // std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain";
                add_content_type(response_body, "text/plain");
                add_encodings(response_body, valid_encodings);

                std::string compressed_data = encode_using_gzip(temp);
                add_content(response_body, compressed_data);

                std::cout << "Response for echo in encoding::\n" << response_body << "\n";
                send(client_socket, response_body.c_str(), response_body.size(), 0);
            } else {
                add_content_type(response_body, "text/plain");
                add_content(response_body, temp);
            }
        }

        else if (url.starts_with("/user-agent")) {
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

            add_status_code(OK, response_body);
            add_connection(response_body, should_close);
            add_content_type(response_body, "text/plain");
            add_content(response_body, str_prm);
        }

        else if (url == "/") {
            add_status_code(OK, response_body);
            add_connection(response_body, should_close);
            add_crlf(response_body);
        }

        else if (url.starts_with("/files/")) {
            int idx = url.find_last_of('/');
            std::string file_name = url.substr(idx + 1);
            std::string path = directory_path + file_name;

            if (http_method == "POST") {
                std::ofstream file(path);
                file << get_request_body(requests);
                add_status_code(CREATED, response_body);
                add_connection(response_body, should_close);
                add_crlf(response_body);
            } else {

                struct stat md;
                int is_file_exists = stat(path.c_str(), &md);

                std::cout << "File path: " << path << " is file exits: " << is_file_exists << "\n";

                if (is_file_exists == 0) {
                    std::ifstream file(path);
                    std::string file_content;

                    getline(file, file_content);

                    add_status_code(OK, response_body);
                    add_connection(response_body, should_close);
                    add_content_type(response_body, "application/octet-stream");
                    add_content(response_body, file_content);
                } else {
                    add_status_code(NOT_FOUND, response_body);
                    add_connection(response_body, should_close);
                    add_crlf(response_body);
                }
            }

        } else {
            add_status_code(NOT_FOUND, response_body);
            add_connection(response_body, should_close);
            add_crlf(response_body);
        }

        std::cout << "Response body: " << response_body << "\n";
        send(client_socket, response_body.c_str(), response_body.size(), 0);

        if (should_close) {
            close(client_socket);
            return;
        }
    }
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

    return 0;
}
