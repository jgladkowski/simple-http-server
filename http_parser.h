#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>

constexpr int BUF_SIZE = 4096;

class HttpParser {
public:
    HttpParser(int sock) : sock(sock), buffer_ptr(buffer), bytes_left(0) {}
    int read_one_request(std::string &method, std::string &path, bool &close_flag);

private:
    int read_until_character(std::string &str, char final_character, bool allow_whitespace);
    int read_until_character_full(std::string &str, char final_character, bool allow_whitespace);
    bool refill_buffer();

    char buffer[BUF_SIZE + 1];
    char *buffer_ptr;
    int bytes_left;
    int sock;
};

#endif // HTTP_PARSER_H