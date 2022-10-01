#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#define TRUE 1

void *connection_handler(void *args);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, thread_data[5];

    pthread_t thread_id[5];

    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Create a socket
    portno = atoi(argv[1]);
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
    serv_addr.sin_port = htons(portno);

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

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
        }
        i++;

        if (i == 5)
        {
            printf("Max clients reached!!\n Waiting for clients to disconnect...\n");
            for (int j = 0; j < 5; j++, i--)
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
    char buffer[256];

    // Read and write to the socket
    do
    {
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0)
        {
            fprintf(stderr, "ERROR reading from socket\n");
            exit(1);
        }
        else if (n == 0)
        {
            printf("Client disconnected!!\n");
        }
        else
        {
            printf("Here is the message: %s", buffer);

            n = write(newsockfd, buffer, strlen(buffer));
            if (n < 0)
            {
                fprintf(stderr, "ERROR writing to socket\n");
                exit(1);
            }
        }
    } while (n > 0);
    close(newsockfd);

    return NULL;
}