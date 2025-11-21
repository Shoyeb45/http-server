#include "utils.hpp"
#include <iostream>
#include <zlib.h>

/// @brief Function to compress the data using gun zip algorithm, it uses zlib.c to compress the data
/// @param data The string that needs to be compressed
/// @return The compressed data in binary
std::string encode_using_gzip(std::string &data) {
    z_stream zs{};
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        throw std::runtime_error("deflateInit2 failed");

    zs.next_in = (Bytef *)data.data();
    zs.avail_in = data.size();

    std::string out_buffer;
    out_buffer.resize(128);

    std::string result;

    int ret;
    do {
        zs.next_out = (Bytef *)out_buffer.data();
        zs.avail_out = out_buffer.size();

        ret = deflate(&zs, Z_FINISH);

        result.append(out_buffer.data(), out_buffer.size() - zs.avail_out);

    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END)
        throw std::runtime_error("deflate failed");

    return result;
}

/// @brief Utility function to split the string by giving the delimiter
/// @param src The word that needs to be splitted
/// @param split_word  The delimiter
/// @return vector of strings after splitting using delimiter
std::vector<std::string> split_str(std::string src, std::string split_word) {
    std::vector<std::string> answers;
    int len = (int)split_word.size();

    std::string word_so_far = "";

    for (int i = 0; i < src.size(); i++) {
        word_so_far += src[i];

        if (i + len - 1 > src.size())
            continue;

        std::string temp = src.substr(i, len);
        if (temp == split_word) {
            if (!word_so_far.empty())
                word_so_far.pop_back();
            answers.push_back(word_so_far);
            word_so_far = "";
            i = i + len - 1;
        }
    }

    if (!word_so_far.empty())
        answers.push_back(word_so_far);
    return answers;
}


/// @brief Function to extract the target url
/// @param buf The buffer received from the client
/// @param http_method The Http method, GET/POST/PATCH
/// @return The target url, example /api/v1/user
std::string get_target_url(std::vector<char> &buf, std::string http_method) {
    std::string target_url = "";
    for (int i = http_method.size() + 1; i < buf.size(); i++) {
        if (buf[i] == ' ')
            break;
        target_url += buf[i];
    }
    return target_url;
}

/// @brief Function tot get http method
/// @param buf The buffer received from the client
/// @return The http method, exmaple: GET, POST, PATCH ...
std::string get_http_method(std::vector<char> &buf) {
    std::string method = "";
    for (int i = 0; i < buf.size(); i++) {
        if (buf[i] == ' ')
            break;
        method += buf[i];
    }
    return method;
}

/// @brief Method to get the body from the request
/// @param req_str The request data from the client
/// @return The body of the request
std::string get_request_body(std::string &req_str) {
    int idx = req_str.find("\r\n\r\n");  // \r\n\r\n\Hi
    std::string body = req_str.substr(idx + 4);
    return body;
}
