#include <algorithm>
#include <vector>

#include <string.h>
#include <unistd.h>

#include "http_parser.h"

namespace {
    void make_lowercase(std::string &str) {
        for (int i = 0; i < str.size(); ++i) {
            if (str[i] >= 'A' && str[i] <= 'Z') {
                str[i] -= 'A';
                str[i] += 'a';
            }
        }
    }
}

// Returns true if there was a read error
bool HttpParser::refill_buffer() {
    if (bytes_left == 0) {
        memset(buffer, 0, BUF_SIZE + 1);
        bytes_left = read(sock, buffer, BUF_SIZE);
        if (bytes_left == -1)
            return true;
        buffer_ptr = buffer;
    }
    return false;
}

// One read from buffer to string
// Returns -1 if an incorrect character was found
int HttpParser::read_until_character(std::string &str, char final_character, bool allow_whitespace) {
    int i;
    for (i = 0; i < bytes_left; ++i) {
        if (buffer_ptr[i] == final_character)
            break;
        if (buffer_ptr[i] <= 0 || (!allow_whitespace && isspace(buffer_ptr[i])))
            return -1;
    }
    str.append(buffer_ptr, i);
    return i;
}

// Reading from buffer to string in a loop until the final character is found
// May return HTTP status codes 500 and 400, otherwise 0
int HttpParser::read_until_character_full(std::string &str, char final_character, bool allow_whitespace) {
    if (refill_buffer())
        return 500;
    int bytes_considered;
    while ((bytes_considered = read_until_character(str, final_character, allow_whitespace)) > 0) {
        buffer_ptr += bytes_considered;
        bytes_left -= bytes_considered;
        if (refill_buffer())
            return 500;
    }
    if (bytes_considered == -1)
        return 400;
    ++buffer_ptr;
    --bytes_left;
    if (refill_buffer())
        return 500;
    return 0;
}

// May return HTTP status codes 500 or 400
// Otherwise returns 200
int HttpParser::read_one_request(std::string &method, std::string &path, bool &close_flag) {
    // Reading the method until first space
    int status = read_until_character_full(method, ' ', false);
    if (status == 400 || status == 500)
        return status;
    
    // Reading the path until first space
    status = read_until_character_full(path, ' ', false);
    if (status == 400 || status == 500)
        return status;

    // Checking if the HTTP version is correct
    std::string http_version;
    status = read_until_character_full(http_version, '\r', false);
    if (status == 400 || status == 500)
        return status;
    if (http_version != "HTTP/1.1" || *buffer_ptr != '\n')
        return 400;
    ++buffer_ptr;
    --bytes_left;

    bool connection_flag = false;
    bool content_length_flag = false;
    while (true) {
        // Checking for CRLF
        if (refill_buffer())
            return 500;
        if (*buffer_ptr == '\r') {
            ++buffer_ptr;
            --bytes_left;
            if (refill_buffer())
                return 500;
            if (*buffer_ptr == '\n')
                break;
            else
                return 400;
        }

        // Reading header name until a colon
        std::string header_name;
        status = read_until_character_full(header_name, ':', false);
        if (status == 400 || status == 500)
            return status;
        if (header_name.empty())
            return 400;

        // Reading header value until CRLF
        std::string header_value;
        status = read_until_character_full(header_value, '\n', true);
        if (status == 400 || status == 500)
            return status;
        if (header_value.empty() || header_value.back() != '\r')
            return status;

        // Removing the spaces from header_value
        header_value.back() = ' ';
        int value_start = header_value.find_first_not_of(" ");
        int value_end = header_value.find_last_not_of(" ");
        header_value = header_value.substr(value_start, value_end - value_start + 1);

        // Connection and Content-Length - checking for repeats and incorrect values
        make_lowercase(header_name);
        if (header_name == "connection") {
            if (connection_flag)
                return 400;
            connection_flag = true;
            if (header_value == "close")
                close_flag = true;
            else if (header_value != "keep-alive")
                return 400;
        }
        else if (header_name == "content-length") {
            if (content_length_flag)
                return 400;
            content_length_flag = true;
            if (header_value.empty())
                return 400;
            for (int i = 0; i < header_value.size(); ++i) {
                if (header_value[i] != '0')
                    return 400;
            }
        }
    }

    ++buffer_ptr;
    --bytes_left;
    return 200;
}
