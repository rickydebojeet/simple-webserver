#ifndef _HTTP_SERVER_HH_
#define _HTTP_SERVER_HH_

#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <ctime>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Configuration
#define HEADER_MAX 8192
#define THREAD_MAX 32
#define PORT 8080
#define QUEUE_MAX 64
#define SHOW_HEADER 1
#define SHOW_OUTPUT 1
#define SANITY_CHECK 1
#define FAULT_EXIT 1

#define USE_SENDFILE 0

using namespace std;

struct HTTP_Request {
    string HTTP_version;  // 1.0, 1.1

    string method;  // GET
    string url;

    int error_code;  // For storing any parsing errors

    vector<string> header_lines;
    string range_header;

    HTTP_Request(string request);  // Constructor
};

struct HTTP_Response {
    string HTTP_version;  // 1.0, 1.1

    string status_code;  // ex: 200, 404, etc.
    string status_text;  // ex: OK, Not Found, etc.

    string content_type;
    string content_length;
    string content_range;

    string body;

    string date;
    string connection;  // "keep-alive" or "close"

    // Sendfile fields
    bool use_sendfile;
    int file_fd;
    off_t offset;
    size_t length;

    HTTP_Response();  // Constructor
    string
    get_string();  // Returns the string representation of the HTTP Response
};

struct FILE_range {
    off_t start;
    off_t end;
    bool valid;
};

HTTP_Response* handle_request(string request);  // Function to handle a request

#endif
