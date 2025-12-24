#ifndef _HTTP_SERVER_HH_
#define _HTTP_SERVER_HH_

#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <ctime>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

// Configuration
#define HEADER_MAX 8192
#define THREAD_MAX 32
#define PORT 8080
#define QUEUE_MAX 64
#define SHOW_HEADER 0
#define SHOW_OUTPUT 0
#define SANITY_CHECK 0
#define FAULT_EXIT 0

using namespace std;

void* connection_handler(void* args);
void int_handler(int);

struct HTTP_Request {
    string HTTP_version;

    string method;
    string url;

    // TODO : Add more fields if and when needed

    HTTP_Request(string request);  // Constructor
};

struct HTTP_Response {
    string HTTP_version;  // 1.0 for this assignment

    string status_code;  // ex: 200, 404, etc.
    string status_text;  // ex: OK, Not Found, etc.

    string content_type;
    string content_length;

    string body;

    // TODO : Add more fields if and when needed
    string date;

    string
    get_string();  // Returns the string representation of the HTTP Response
};

string handle_request(string request);  // Function to handle a request

#endif
