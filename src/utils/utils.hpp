#define UTILS_HPP

#include <string>
#include <vector>

std::string encode_using_gzip(std::string &);
std::vector<std::string> split_str(std::string, std::string);
std::string get_target_url(std::vector<char> &, std::string);
std::string get_http_method(std::vector<char> &);
std::string get_request_body(std::string &);