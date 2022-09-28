#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    int portno, sockfd;

    char buffer[256];

    struct hostent *server;
    struct sockaddr_in serv_addr;

    if (argc < 3)
    {
        fprintf(stderr, "Usage ./%s hostname port\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        exit(0);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error connecting\n");
        exit(0);
    }

    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    if (write(sockfd, buffer, strlen(buffer)) < 0)
    {
        fprintf(stderr, "Error writing to socket\n");
        exit(0);
    }

    if (read(sockfd, buffer, 255) < 0)
    {
        fprintf(stderr, "Error reading from socket\n");
        exit(0);
    }

    printf("Server Response: %s\n", buffer);

    return 0;
}