#include <stdio.h>

#include "Enclave_u.h"
#include "sgx_dh.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"
#include "sgx_utils/net_utils.h"

void run_server(const char *port);
void run_client(const char *host, const char *port);

/* Global Enclave ID */
sgx_enclave_id_t global_eid;

// /* OCall implementations */
// void ocall_print(const char *str)
// {
//     printf("%s\n", str);
// }

void fprint_usage(FILE *fp, const char *executable)
{
    fprintf(fp, "Usage: \n");
    fprintf(fp, "    %s server <port>", executable);
    fprintf(fp, "    %s client <host> <port>", executable);
}

int main(int argc, char const *argv[])
{
    /* Enclave Initialization */
    if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0)
    {
        printf("Fail to initialize enclave.\n");
        return 1;
    }

    // sgx_status_t status;
    // status = get_sum(global_eid, &sum_result, 3, 4);
    // if (status != SGX_SUCCESS)
    // {
    //     printf("ECall failed.\n");
    //     return 1;
    // }
    // printf("Sum from enclave: %d\n", sum_result);

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

void run_client(const char *host, const char *port)
{
    int clientfd = Open_clientfd(host, port);

    sgx_status_t status;


    printf("[%s: %4d] %s\n", "client", __LINE__, "initiator_init_session ...");
    initiator_init_session(global_eid, &status);
    if (status != SGX_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    sgx_dh_msg1_t msg1;
    sgx_dh_msg2_t msg2;
    sgx_dh_msg3_t msg3;

    /* recv msg1 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "read_socket ...");
    read_socket(clientfd, (uint8_t *)&msg1, sizeof(sgx_dh_msg1_t));

    /* proc msg1, gen msg2 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "initiator_proc_msg1 ...");
    initiator_proc_msg1(global_eid, &status, &msg1, &msg2);
    if (status != SGX_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    /* send msg2 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "write_socket ...");
    write_socket(clientfd, (uint8_t *)&msg2, sizeof(sgx_dh_msg2_t));

    /* read msg3 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "read_socket ...");
    read_socket(clientfd, (uint8_t *)&msg3, sizeof(sgx_dh_msg3_t));

    /* proc msg3 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "initiator_proc_msg3 ...");
    initiator_proc_msg3(global_eid, &status, &msg3);
    if (status != SGX_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    // start echo
    printf("[%s: %4d] %s\n", "client", __LINE__, "started ...");
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

/*

sgx_status_t encrypt(sgx_enclave_id_t eid, sgx_status_t* retval, const uint8_t* plaintext, uint32_t pt_len, uint8_t* ciphertext, uint32_t* nonce);
sgx_status_t decrypt(sgx_enclave_id_t eid, sgx_status_t* retval, uint32_t nonce, const uint8_t* ciphertext, uint32_t ct_len, uint8_t* plaintext);

*/

void handle(int connfd)
{
    sgx_status_t status;

    printf("[%s: %4d] %s\n", "server", __LINE__, "responder_init_session ...");
    responder_init_session(global_eid, &status);
    if (status != SGX_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    sgx_dh_msg1_t msg1;
    sgx_dh_msg2_t msg2;
    sgx_dh_msg3_t msg3;

    printf("[%s: %4d] %s\n", "server", __LINE__, "responder_gen_msg1 ...");
    responder_gen_msg1(global_eid, &status, &msg1);
    if (status != SGX_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    /* send msg1, recv msg2 */
    printf("[%s: %4d] %s\n", "server", __LINE__, "write_socket ...");
    write_socket(connfd, (uint8_t *)&msg1, sizeof(sgx_dh_msg1_t));

    printf("[%s: %4d] %s\n", "server", __LINE__, "read_socket ...");
    read_socket(connfd, (uint8_t *)&msg2, sizeof(sgx_dh_msg2_t));

    printf("[%s: %4d] %s\n", "server", __LINE__, "responder_proc_msg2 ...");
    responder_proc_msg2(global_eid, &status, &msg2, &msg3);
    if (status != SGX_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    /* send msg3 */
    printf("[%s: %4d] %s\n", "server", __LINE__, "write_socket ...");
    write_socket(connfd, (uint8_t *)&msg3, sizeof(sgx_dh_msg3_t));

    printf("[%s: %4d] %s\n", "server", __LINE__, "started ...");
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
        handle(connfd);
        Close(connfd);
    }
}
