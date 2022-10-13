#include "http_server.hh"

// mutex and condition variable for the thread pool
pthread_mutex_t queueMutex;
pthread_cond_t queueCondFull;
pthread_cond_t queueCondEmpty;

// queue for the thread pool
queue<int> client_queue;
atomic<bool> running = true;

struct sigaction act;

int main(int argc, char *argv[])
{
    int sockfd; // file descriptor for the listen socket
    pthread_t thread_id[THREAD_MAX];

    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    act.sa_handler = int_handler; // set signal handler for parent

    sigaction(SIGINT, &act, 0); // set interrupt signal handler for parent

    // initialize mutex and condition variables
    pthread_mutex_init(&queueMutex, NULL);
    pthread_cond_init(&queueCondFull, NULL);
    pthread_cond_init(&queueCondEmpty, NULL);

    // create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    // ready the socket address for binding
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // bind the socket to the address
    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    cout << "Server started!!" << endl;
    cout << "Listening on port " << PORT << endl;
    cout << "To stop the server, press Ctrl+C" << endl;

    // creating threads
    for (int i = 0; i < THREAD_MAX; i++)
    {
        if (pthread_create(&thread_id[i], NULL, &connection_handler, NULL) != 0)
        {
            cerr << "Error: Could not create thread " << i << endl;
            exit(EXIT_FAILURE);
        }
    }

    do
    {
        // accept a connection
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (!running)
        {
            break;
        }
        else if (newsockfd < 0)
        {
            cerr << "ERROR on accept" << endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            pthread_mutex_lock(&queueMutex);
            while (client_queue.size() == QUEUE_MAX)
            {
                pthread_cond_wait(&queueCondFull, &queueMutex);
            }
            cout << "Connection accepted!!" << endl;
            client_queue.push(newsockfd);
            pthread_cond_signal(&queueCondEmpty);
            pthread_mutex_unlock(&queueMutex);
        }
    } while (running);

    for (int j = 0; j < THREAD_MAX; j++)
    {
        pthread_join(thread_id[j], NULL);
    }

    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&queueCondFull);
    pthread_cond_destroy(&queueCondEmpty);

    close(sockfd);

    cout << "Server stopped!!" << endl;

    return 0;
}

void *connection_handler(void *args)
{
    while (running)
    {
        pthread_mutex_lock(&queueMutex);
        while (client_queue.empty() && running)
        {
            pthread_cond_wait(&queueCondEmpty, &queueMutex);
        }
        if (!running)
        {
            pthread_mutex_unlock(&queueMutex);
            continue;
        }
        int newsockfd = client_queue.front();
        client_queue.pop();
        pthread_cond_signal(&queueCondFull);
        pthread_mutex_unlock(&queueMutex);

        int n;
        string request, response;
        char buffer[HEADER_MAX];

        // read and write to the socket
        n = read(newsockfd, buffer, HEADER_MAX - 1);
        if (n < 0)
        {
            cerr << "ERROR reading from socket" << endl;
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            cout << "Client disconnected!!" << endl;
        }
        else
        {
            /* looking for the end-of-header (eoh) CRLF CRLF substring; if
               not found, then the header size is too large */
            char *eoh = strstr(buffer, "\r\n\r\n");
            if (eoh == NULL)
            {
                cerr << "ERROR: Header size too large" << endl;
                exit(EXIT_FAILURE);
            }
            else
            {
#if SHOW_HEADER
                cout << "Request:" << endl
                     << buffer << endl;
#endif

                request = string(buffer);
                response = handle_request(request);

#if SANITY_CHECK
                cout << "Response: " << endl
                     << response << endl;
#endif

                n = write(newsockfd, response.c_str(), strlen(response.c_str()));
                if (n < 0)
                {
                    cerr << "ERROR writing to socket" << endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
        close(newsockfd);
        cout << "Client disconnected!!" << endl;
    }

    return NULL;
}

void int_handler(int p)
{
    running = false;
    cout << "\nThe server is shutting down!!" << endl;
    pthread_cond_broadcast(&queueCondEmpty);
}