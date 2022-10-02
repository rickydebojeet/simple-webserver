#include "http_server.hh"

int sockfd;
struct sigaction act;

int main(int argc, char *argv[])
{
    int newsockfd, thread_data[THREAD_MAX];

    pthread_t thread_id[THREAD_MAX];

    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    act.sa_handler = int_handler; // set signal handler for parent

    sigaction(SIGINT, &act, 0); // set interrupt signal handler for parent

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }

    // Ready the socket address for binding
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the socket to the address
    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    cout << "Server started!!" << endl;
    cout << "Listening on port " << PORT << endl;
    cout << "To stop the server, press Ctrl+C" << endl;

    int i = 0;

    // Listen for connections continuously
    do
    {
        // Accept a connection
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            fprintf(stderr, "ERROR on accept\n");
            exit(1);
        }
        else
        {
            thread_data[i] = newsockfd;
            printf("Client connected!!\n");
            pthread_create(&thread_id[i], NULL, &connection_handler, (void *)&thread_data[i]);
            i++;
        }

        if (i == THREAD_MAX)
        {
            printf("Max clients reached!!\n Waiting for clients to disconnect...\n");
            for (int j = 0; j < THREAD_MAX; j++, i--)
            {
                pthread_join(thread_id[j], NULL);
            }
        }
    } while (TRUE);

    return 0;
}

void *connection_handler(void *args)
{
    int newsockfd = *((int *)args);
    int n;
    string request, response;
    char buffer[HEADER_MAX];

    // Read and write to the socket
    n = read(newsockfd, buffer, HEADER_MAX - 1);
    if (n < 0)
    {
        fprintf(stderr, "ERROR reading from socket\n");
        exit(1);
    }
    else if (n == 0)
    {
        printf("Client closed connection!!\n");
    }
    else
    {
        /* Look for the end-of-header (eoh) CRLF CRLF substring; if
           not found, then the header size is too large */
        char *eoh = strstr(buffer, "\r\n\r\n");
        if (eoh == NULL)
        {
            fprintf(stderr, "Header exceeds 8 KB maximum\n");
            exit(1);
        }
        else
        {
            // cout << "Request: " << buffer << endl;
            request = string(buffer);
            response = handle_request(request);
            char char_array[response.length() + 1];
            strcpy(char_array, response.c_str());
            n = write(newsockfd, char_array, strlen(char_array));
            // cout << "Response: " << response << endl;
            if (n < 0)
            {
                fprintf(stderr, "ERROR writing to socket\n");
                exit(1);
            }
        }
    }
    close(newsockfd);
    cout << "Client disconnected!!" << endl;

    return NULL;
}

void int_handler(int p)
{
    printf("\nThe Server is exiting.\nAll connections will be closed!!\n");
    close(sockfd);
    exit(0);
}