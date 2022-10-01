#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>

void int_handler(int);

int sockfd;
struct sigaction act;

int main(int argc, char *argv[])
{
    int portno, n;

    char buffer[256];

    struct hostent *server;
    struct sockaddr_in serv_addr;

    act.sa_handler = int_handler; // set signal handler for parent

    sigaction(SIGINT, &act, 0); // set interrupt signal handler for parent

    if (argc < 3)
    {
        fprintf(stderr, "Usage ./%s hostname port\n", argv[0]);
        exit(0);
    }

    // Create a socket
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        exit(0);
    }

    // Get the server address
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        exit(0);
    }

    // Ready the socket address for connecting
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error connecting\n");
        exit(0);
    }

    // Read and write to the socket
    do
    {
        printf("Please enter the message: ");
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            fprintf(stderr, "Error writing to socket\n");
            exit(0);
        }

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);

        if (n < 0)
        {
            fprintf(stderr, "Error reading from socket\n");
            exit(0);
        }
        else if (n == 0)
        {
            printf("Server closed connection\n");
            break;
        }

        printf("Server Response: %s\n", buffer);

    } while (n > 0);

    close(sockfd);

    return 0;
}

void int_handler(int p)
{
    printf("\nClosing the connection.\n");
    close(sockfd);
    exit(0);
}