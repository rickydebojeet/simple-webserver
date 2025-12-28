#include "http_server.hh"

// mutex and condition variable for the thread pool
pthread_mutex_t queueMutex;
pthread_cond_t queueCondFull;
pthread_cond_t queueCondEmpty;

// queue for the thread pool
queue<int> client_queue;
atomic<bool> running = {true};

struct sigaction act;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        cout << "Starting server with default port " << PORT << endl;
    } else if (argc == 2) {
        int port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            cerr << "Invalid port number. Please provide a port between 1 and "
                    "65535."
                 << endl;
            exit(EXIT_FAILURE);
        }
        cout << "Starting server with port " << port << endl;
    } else {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        exit(EXIT_FAILURE);
    }

    int sockfd;  // file descriptor for the listen socket
    pthread_t thread_id[THREAD_MAX];

    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    act.sa_handler = int_handler;  // set signal handler for parent
    sigaction(SIGINT, &act, 0);    // set interrupt signal handler for parent

    // initialize mutex and condition variables
    pthread_mutex_init(&queueMutex, NULL);
    pthread_cond_init(&queueCondFull, NULL);
    pthread_cond_init(&queueCondEmpty, NULL);

    // create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // ready the socket address for binding
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(sockfd, QUEUE_MAX);
    clilen = sizeof(cli_addr);
    cout << "Server started!! "
         << "pid: " << getpid() << endl
         << "Listening on port " << PORT << endl
         << "To stop the server, press Ctrl+C" << endl;

    // creating threads
    for (int i = 0; i < THREAD_MAX; i++) {
        if (pthread_create(&thread_id[i], NULL, &connection_handler, NULL) !=
            0) {
            cerr << "Error: Could not create thread " << i << endl;
            exit(EXIT_FAILURE);
        }
    }

    do {
        // accept a connection
        int newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (!running) {
            break;
        } else if (newsockfd < 0) {
            cerr << "ERROR on accept" << endl;
            exit(EXIT_FAILURE);
        } else {
            pthread_mutex_lock(&queueMutex);
            while (client_queue.size() == QUEUE_MAX) {
                pthread_cond_wait(&queueCondFull, &queueMutex);
            }
#if SANITY_CHECK
            cout << "Connection accepted!!" << endl;
#endif
            client_queue.push(newsockfd);
            pthread_cond_signal(&queueCondEmpty);
            pthread_mutex_unlock(&queueMutex);
        }
    } while (running);

    for (int j = 0; j < THREAD_MAX; j++) {
        pthread_join(thread_id[j], NULL);
    }

    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&queueCondFull);
    pthread_cond_destroy(&queueCondEmpty);

    close(sockfd);

    cout << "Server stopped!!" << endl;

    return 0;
}

void* connection_handler(void* args) {
    (void)args;  // unused parameter

    while (running) {
        pthread_mutex_lock(&queueMutex);
        while (client_queue.empty() && running) {
            pthread_cond_wait(&queueCondEmpty, &queueMutex);
        }
        if (!running) {
            pthread_mutex_unlock(&queueMutex);
            continue;
        }

        int newsockfd = client_queue.front();
        client_queue.pop();
        pthread_cond_signal(&queueCondFull);
        pthread_mutex_unlock(&queueMutex);

#if SANITY_CHECK
        cout << newsockfd << "FD: " << "Accepted connection!!" << endl;
#endif

        struct timeval tv;
        tv.tv_sec = 60;  // 60 Secs Timeout
        tv.tv_usec = 0;
        if (setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,
                       sizeof tv)) {
            perror("setsockopt");
            // in error case, continue without timeout
        }

        while (true) {
            int n;
            string request;
            HTTP_Response* response = nullptr;
            char buffer[HEADER_MAX];

            // read request from the socket
            n = read(newsockfd, buffer, HEADER_MAX - 1);
            if (n < 0) {
#if FAULT_EXIT
                cerr << newsockfd << "FD: " << "ERROR reading from socket"
                     << endl;
                exit(EXIT_FAILURE);
#endif
                break;
            } else if (n == 0) {
#if SANITY_CHECK
                cout << newsockfd << "FD: " << "Client disconnected!!" << endl;
#endif
                break;
            } else {
                buffer[n] = '\0';
                /* looking for the end-of-header (eoh) CRLF CRLF substring; if
                   not found, then the header size is too large */
                char* eoh = strstr(buffer, "\r\n\r\n");
                if (eoh == NULL) {
                    cerr << "ERROR: Header size too large" << endl;
                    exit(EXIT_FAILURE);
                } else {
#if SHOW_HEADER
                    cout << "Request:" << endl << buffer << endl;
#endif

                    request = string(buffer);
#if SANITY_CHECK
                    cout << "Processing request on FD: " << newsockfd
                         << " URL: " << request.substr(0, request.find('\n'))
                         << endl;
#endif
                    response = handle_request(request);

#if SHOW_OUTPUT
                    cout << "Response: " << endl
                         << response->get_string() << endl;
#endif

                    // Send Headers
                    string headers = response->get_string();
                    n = write(newsockfd, headers.c_str(), headers.length());
                    if (n < 0) {
#if FAULT_EXIT
                        cerr << "ERROR writing headers to socket" << endl;
#endif
                        delete response;
                        break;
                    }

                    if (response->use_sendfile && response->file_fd != -1) {
#ifdef __linux__
                        off_t offset = response->offset;
                        size_t remaining = response->length;
                        ssize_t sent;

                        while (remaining > 0) {
                            sent = sendfile(newsockfd, response->file_fd,
                                            &offset, remaining);
                            if (sent <= 0) {
                                // Error or disconnect
                                break;
                            }
                            remaining -= sent;
                        }
#else
                        // Simple fallback for Mac/other (non-zero copy)
                        char buf[8192];
                        lseek(response->file_fd, response->offset, SEEK_SET);
                        size_t remaining = response->length;
                        while (remaining > 0) {
                            int to_read = (remaining > sizeof(buf))
                                              ? sizeof(buf)
                                              : remaining;
                            int r = read(response->file_fd, buf, to_read);
                            if (r <= 0) break;
                            int w = write(newsockfd, buf, r);
                            if (w < 0) break;
                            remaining -= r;
                        }
#endif
                        close(response->file_fd);
                    } else {
                        // Standard write for body
                        if (!response->body.empty()) {
                            n = write(newsockfd, response->body.c_str(),
                                      response->body.length());
                            if (n < 0) {
#if FAULT_EXIT
                                cerr << "ERROR writing body to socket" << endl;
#endif
                                delete response;
                                break;
                            }
                        }
                    }

                    // Check if connection should be closed
                    if (response->connection == "close") {
                        delete response;
                        break;
                    }

                    delete response;
                }
            }
        }
        close(newsockfd);
#if SANITY_CHECK
        cout << "Client disconnected!!" << endl;
#endif
    }

    return NULL;
}

void int_handler(int p) {
    (void)p;  // unused parameter
    running = false;
    cout << "\nThe server is shutting down!!" << endl;
    pthread_cond_broadcast(&queueCondEmpty);
}