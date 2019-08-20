#include <stdio.h>

#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"

/* Global Enclave ID */
sgx_enclave_id_t global_eid;

/* OCall implementations */
void ocall_print(const char *str)
{
    printf("%s\n", str);
}

int main(int argc, char const *argv[])
{
    int sum_result;
    sgx_status_t status;

    /* Enclave Initialization */
    if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0)
    {
        printf("Fail to initialize enclave.\n");
        return 1;
    }

    /* Call a simple method inside enclave */
    status = get_sum(global_eid, &sum_result, 3, 4);
    if (status != SGX_SUCCESS)
    {
        printf("ECall failed.\n");
        return 1;
    }
    printf("Sum from enclave: %d\n", sum_result);

    return 0;
}
