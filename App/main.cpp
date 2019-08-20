#include "csapp.h"
#include <iostream>
#include <string>

using namespace std;

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        //line:netp:echo:eof
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
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

void run_client(const char *host, const char *port)
{
    int clientfd;
    char buf[MAXLINE];
    rio_t rio;

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
}

void fprint_usage(FILE *fp, const char *executable)
{
    fprintf(fp, "Usage: \n");
    fprintf(fp, "    %s server <port>", executable);
    fprintf(fp, "    %s client <host> <port>", executable);
}

int main(int argc, const char **argv)
{
    if (argc == 3 && *argv[1] == 's')
    {
        const char *port = argv[2];
        run_server(port);
        return 0;
    }

    if (argc == 4 && *argv[1] == 'c')
    {
        const char *host = argv[2];
        const char *port = argv[3];
        run_client(host, port);
        return 0;
    }

    fprint_usage(stderr, argv[0]);
    return -1;

    return 0;
}
