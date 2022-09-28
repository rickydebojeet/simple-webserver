#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;

    char buffer[256];

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
        fprintf(stderr, "ERROR opening socket");
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
        fprintf(stderr, "ERROR on binding");
        exit(1);
    }

    // Listen for connections continuously
    while (1)
    {
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);

        // Accept a connection
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            fprintf(stderr, "ERROR on accept");
            exit(1);
        }

        // Read and write to the socket
        bzero(buffer, 256);
        if (read(newsockfd, buffer, 255) < 0)
        {
            fprintf(stderr, "ERROR reading from socket");
            exit(1);
        }
        printf("Here is the message: %s", buffer);

        if (write(newsockfd, buffer, strlen(buffer)) < 0)
        {
            fprintf(stderr, "ERROR writing to socket");
            exit(1);
        }
    }

    return 0;
}