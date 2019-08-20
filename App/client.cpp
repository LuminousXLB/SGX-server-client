#include "net_utils/net_utils.h"

void run_client(const char *host, const char *port)
{
    int clientfd = Open_clientfd(host, port);

    char buf[MAXLINE];

    uint32_t length = 2;

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {
        write_socket(clientfd, (uint8_t *)buf, strlen(buf));
        read_socket(clientfd, (uint8_t *)buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
}