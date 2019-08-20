#include "csapp.h"
#include <iostream>
#include <string>

using namespace std;

void run_server(const char *port);
void run_client(const char *host, const char *port);

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
}
