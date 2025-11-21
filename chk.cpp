#include <bits/stdc++.h>
#include <zlib.h>

using namespace std;

std::string encode_using_gzip(std::string &data)
{
    z_stream zs{};
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        throw runtime_error("deflateInit2 failed");

    zs.next_in = (Bytef*)data.data();
    zs.avail_in = data.size();

    std::string out_buffer;
    out_buffer.resize(128);

    std::string result;

    int ret;
    do {
        zs.next_out = (Bytef*)out_buffer.data();
        zs.avail_out = out_buffer.size();

        ret = deflate(&zs, Z_FINISH);

        result.append(out_buffer.data(), out_buffer.size() - zs.avail_out);

    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END)
        throw runtime_error("deflate failed");

    return result;   // compressed gzip data (binary)
}

int main()
{
    string s = "abc";
    string t = encode_using_gzip(s);
    // Print hex so you can SEE it
    for (unsigned char c : t) printf("%02X ", c);
    printf("\n");

    return 0;
}
