#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <thread>
// For reading file
#include <fstream>
// For checking if the file exists or not
#include <sys/stat.h>

std::thread connections[100];

std::string directory_path = "";

void accept_connection(int client_socket)
{
  std::vector<char> buf(5000);

  int bytes = recv(client_socket, buf.data(), buf.size(), 0);

  std::string url = "";
  for (int i = 4; i < 5000; i++)
  {
    if (buf[i] == ' ')
      break;
    url += buf[i];
  }

  if (url.starts_with("/echo/"))
  {
    int idx = url.find_last_of('/');
    std::string temp = url.substr(idx + 1);
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
    response += std::to_string(temp.size());
    response += "\r\n\r\n" + temp;

    send(client_socket, response.c_str(), response.size() + 1, 0);
  }

  if (url.starts_with("/user-agent"))
  {
    std::string requests = buf.data();

    int idx_of_user = requests.find("User-Agent");
    int idx = requests.find(":", idx_of_user);
    std::string str_prm = "";

    for (int i = idx + 2;; i++)
    {
      if (requests[i] == '\r' && requests[i + 1] == '\n')
        break;
      str_prm += requests[i];
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
    response += std::to_string(str_prm.size());
    response += "\r\n\r\n" + str_prm;

    send(client_socket, response.c_str(), response.size() + 1, 0);
  }
  
  if (url == "/")
  {
    send(client_socket, "HTTP/1.1 200 OK\r\n\r\n", 20, 0);
    return;
  }

  if (url.starts_with("/files/"))
  {
    int idx = url.find_last_of('/');
    std::string file_name = url.substr(idx + 1);
    std::string path = directory_path + file_name;

    struct stat md;
    int is_file_exists = stat(path.c_str(), &md);
    std::cout << "File path: " << path << " is file exits: " << is_file_exists << "\n";
    if (is_file_exists == 0)
    {
      std::ifstream file(path);
      std::string file_content;
      

      getline(file, file_content);
      std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-strean\r\nContent-Length: ";
      response += std::to_string(file_content.size());
      response += "\r\n\r\n" + file_content;
      
      send(client_socket, response.c_str(), response.size() + 1, 0);
      return;
    }
  }
  send(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n", 27, 0);
}

std::string get_directory(char **argv, int argc) {
  int i = 0;
  for (; i < argc; i++) {
    if (argv[i] == "--directory") {
      break;
    }
  }

  std::string directory = "";
  
  if (i + 1 < argc) {
    directory = argv[i + 1];
  }
  return directory;
}

int main(int argc, char **argv)
{
  directory_path = get_directory(argv, argc);

  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;

  int i = 0;
  while (true)
  {
    std::cout << "Waiting for a client to connect...\n";
    int client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

    if (client_socket < 0)
    {
      std::cerr << "Cannot accept connection.\n";
      exit(1);
    }
    // inet_ntoa gives ip of the client.
    std::cout << "Client with ip " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << " connected on socket : " << client_socket << "\n";
    connections[i++] = std::thread(accept_connection, client_socket);
  }

  close(server_fd);

  return 0;
}
