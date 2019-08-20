#include "net_utils/net_utils.h"

void echo(int connfd)
{
    int64_t n;
    char buf[MAXLINE];

    while ((n = read_socket(connfd, (uint8_t *)buf, MAXLINE)) > 0)
    {
        printf("server received %d bytes\n", (int)n);
        write_socket(connfd, (uint8_t *)buf, n);
    }
}

void run_server(const char *port)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
}
