#ifndef _HTTP_SERVER_HH_
#define _HTTP_SERVER_HH_

#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctime>
#include <signal.h>
#include <algorithm>
#include <string>
#include <queue>
#include <atomic>

// Configuration
#define HEADER_MAX 8192
#define THREAD_MAX 16
#define PORT 8080
#define QUEUE_MAX 32
#define SHOW_HEADER 1
#define SANITY_CHECK 0

using namespace std;

void *connection_handler(void *args);
void int_handler(int);

struct HTTP_Request
{
    string HTTP_version;

    string method;
    string url;

    // TODO : Add more fields if and when needed

    HTTP_Request(string request); // Constructor
};

struct HTTP_Response
{
    string HTTP_version; // 1.0 for this assignment

    string status_code; // ex: 200, 404, etc.
    string status_text; // ex: OK, Not Found, etc.

    string content_type;
    string content_length;

    string body;

    // TODO : Add more fields if and when needed
    string date;

    string get_string(); // Returns the string representation of the HTTP Response
};

string handle_request(string request); // Function to handle a request

#endif
