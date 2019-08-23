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

    fprint_usage(stdout, argv[0]);
    return -1;

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

/*

sgx_status_t aes_ctr_128_encrypt(uint8_t *buffer, uint32_t length, uint32_t *nonce)
sgx_status_t aes_ctr_128_decrypt(uint8_t *buffer, uint32_t length, uint32_t *nonce)

*/

#define MESSAGE_LENGTH 256

struct message_tuple
{
    uint8_t payload[MESSAGE_LENGTH];
    uint32_t nonce;
    uint32_t length;
};

void write_text_message(int connfd, const char *buf, uint32_t length)
{
    printf("[%4d] %s\n", __LINE__, "write_text_message ...");

    sgx_status_t status;
    message_tuple tuple;

    memset(&tuple, 0, sizeof(message_tuple));
    memcpy((void *)tuple.payload, (void *)buf, length);
    tuple.length = length;

    aes_ctr_128_encrypt(global_eid, &status, (uint8_t *)tuple.payload, tuple.length, &tuple.nonce);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }
    printf("[%4d] nonce is %08x\n", __LINE__, tuple.nonce);
    printf("[%4d] length is %u\n", __LINE__, tuple.length);

    // htobe32(tuple.nonce);
    // htobe32(tuple.length);
    write_socket(connfd, (uint8_t *)&tuple, sizeof(tuple));
}

uint32_t read_text_message(int connfd, char *buf)
{
    printf("[%4d] %s\n", __LINE__, "read_text_message ...");

    sgx_status_t status;
    message_tuple tuple;

    memset(&tuple, 0, sizeof(message_tuple));

    read_socket(connfd, (uint8_t *)&tuple, sizeof(tuple));
    // be32toh(tuple.nonce);
    // be32toh(tuple.length);

    printf("[%4d] nonce is %08x\n", __LINE__, tuple.nonce);
    printf("[%4d] length is %u\n", __LINE__, tuple.length);
    aes_ctr_128_decrypt(global_eid, &status, (uint8_t *)tuple.payload, tuple.length, &tuple.nonce);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }

    uint32_t length = tuple.length;
    memcpy((void *)buf, (void *)tuple.payload, length);

    memset(&tuple, 0, sizeof(message_tuple));

    return length;
}

void client_business(int clientfd)
{
    printf("[%s: %4d] %s\n", "client", __LINE__, "started ...");

    char buf[MESSAGE_LENGTH];

    while (Fgets(buf, MESSAGE_LENGTH, stdin) != NULL)
    {
        write_text_message(clientfd, buf, strlen(buf));
        uint32_t length = read_text_message(clientfd, buf);
        Fputs(buf, stdout);
    }
}

void server_business(int connfd)
{
    printf("[%s: %4d] %s\n", "server", __LINE__, "started ...");
    uint32_t length;
    char buf[MESSAGE_LENGTH];

    while ((length = read_text_message(connfd, buf)) > 0)
    {
        printf("[%4u] > %s\n", length, buf);
        write_text_message(connfd, buf, length);
    }
}

////////////////////////////////////////////////////////////////////////////////

void run_client(const char *host, const char *port)
{
    int clientfd = Open_clientfd(host, port);

    sgx_status_t status;

    printf("[%s: %4d] %s\n", "client", __LINE__, "initiator_init_session ...");
    initiator_init_session(global_eid, &status);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }

    sgx_dh_msg1_t msg1;
    sgx_dh_msg2_t msg2;
    sgx_dh_msg3_t msg3;

    /* recv msg1 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "read_socket ...");
    read_socket(clientfd, (uint8_t *)&msg1, sizeof(sgx_dh_msg1_t));
    // hexdump(stdout, (uint8_t *)&msg1, sizeof(sgx_dh_msg1_t));

    /* proc msg1, gen msg2 */
    printf("[%s: %4d] %s\n", "client", __LINE__, "initiator_proc_msg1 ...");
    initiator_proc_msg1(global_eid, &status, &msg1, &msg2);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
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
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }

    client_business(clientfd);
    Close(clientfd);
}

void handle(int connfd)
{
    sgx_status_t status;

    printf("[%s: %4d] %s\n", "server", __LINE__, "responder_init_session ...");
    responder_init_session(global_eid, &status);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }

    sgx_dh_msg1_t msg1;
    sgx_dh_msg2_t msg2;
    sgx_dh_msg3_t msg3;

    printf("[%s: %4d] %s\n", "server", __LINE__, "responder_gen_msg1 ...");
    responder_gen_msg1(global_eid, &status, &msg1);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }

    // printf("[%s: %4d] %s\n", "server", __LINE__, "msg1 generated");
    // PRINT_BYTE_ARRAY(stdout, (uint8_t *)&msg1, sizeof(sgx_dh_msg1_t));

    /* send msg1, recv msg2 */
    printf("[%s: %4d] %s\n", "server", __LINE__, "write_socket ...");
    write_socket(connfd, (uint8_t *)&msg1, sizeof(sgx_dh_msg1_t));

    printf("[%s: %4d] %s\n", "server", __LINE__, "read_socket ...");
    read_socket(connfd, (uint8_t *)&msg2, sizeof(sgx_dh_msg2_t));

    printf("[%s: %4d] %s\n", "server", __LINE__, "responder_proc_msg2 ...");
    responder_proc_msg2(global_eid, &status, &msg2, &msg3);
    if (status != SGX_SUCCESS)
    {
        fprintf(stdout, "Error[%04x] @ %4d\n", status, __LINE__);
        exit(EXIT_FAILURE);
    }

    /* send msg3 */
    printf("[%s: %4d] %s\n", "server", __LINE__, "write_socket ...");
    write_socket(connfd, (uint8_t *)&msg3, sizeof(sgx_dh_msg3_t));

    server_business(connfd);
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
