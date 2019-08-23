#include "Enclave_t.h"

#include "sgx_dh.h"
#include "sgx_trts.h"
#include "tlibc/mbusafecrt.h"

#define REQUIRED(ptr) \
    if (ptr == NULL)  \
    return SGX_ERROR_INVALID_PARAMETER

sgx_dh_session_t sgx_dh_session;
sgx_key_128bit_t aek;

#define COUNTER_LENGTH_IN_BYTES 16
/* 
 * Encrypt & Decrypt using AES-CTR-128
 */
sgx_status_t aes_ctr_128_encrypt(uint8_t *buffer, uint32_t length, uint8_t nonce[COUNTER_LENGTH_IN_BYTES])
{
    uint8_t counter[COUNTER_LENGTH_IN_BYTES];

    sgx_status_t status = sgx_read_rand(counter, COUNTER_LENGTH_IN_BYTES);
    if (status != SGX_SUCCESS)
    {
        return status;
    }

    memcpy_s((void *)nonce, COUNTER_LENGTH_IN_BYTES, counter, COUNTER_LENGTH_IN_BYTES);
    uint8_t *ciphertext = (uint8_t *)malloc(length);

    status = sgx_aes_ctr_encrypt(&aek, buffer, length, counter, COUNTER_LENGTH_IN_BYTES * 8, ciphertext);
    if (status != SGX_SUCCESS)
    {
        return status;
    }

    memcpy_s((void *)buffer, length, ciphertext, length);
    memset((void *)ciphertext, 0, length);

    return status;
}

sgx_status_t aes_ctr_128_decrypt(uint8_t *buffer, uint32_t length, uint8_t nonce[COUNTER_LENGTH_IN_BYTES])
{
    uint8_t counter[COUNTER_LENGTH_IN_BYTES];
    memcpy_s(counter, COUNTER_LENGTH_IN_BYTES, (void *)nonce, COUNTER_LENGTH_IN_BYTES);

    uint8_t *plaintext = (uint8_t *)malloc(length);

    sgx_status_t status = sgx_aes_ctr_decrypt(&aek, buffer, length, counter, COUNTER_LENGTH_IN_BYTES * 8, plaintext);
    if (status != SGX_SUCCESS)
    {
        return status;
    }

    memcpy_s((void *)buffer, length, plaintext, length);
    memset((void *)plaintext, 0, length);

    return status;
}

/*
 * DH Key Exchange (Insecure)
 */
sgx_status_t initiator_init_session()
{
    return sgx_dh_init_session(SGX_DH_SESSION_INITIATOR, &sgx_dh_session);
}

sgx_status_t responder_init_session()
{
    return sgx_dh_init_session(SGX_DH_SESSION_RESPONDER, &sgx_dh_session);
}

sgx_status_t responder_gen_msg1(sgx_dh_msg1_t *msg1)
{
    REQUIRED(msg1);

    return sgx_dh_responder_gen_msg1(msg1, &sgx_dh_session);
}

sgx_status_t initiator_proc_msg1(const sgx_dh_msg1_t *msg1, sgx_dh_msg2_t *msg2)
{
    REQUIRED(msg1);
    REQUIRED(msg2);

    sgx_dh_msg1_t msg1_cpy;
    memcpy_s(&msg1_cpy, sizeof(sgx_dh_msg1_t), msg1, sizeof(sgx_dh_msg1_t));

    sgx_dh_msg1_t msg2_cpy;
    memcpy_s(&msg2_cpy, sizeof(sgx_dh_msg2_t), msg2, sizeof(sgx_dh_msg2_t));

    return sgx_dh_initiator_proc_msg1(msg1, msg2, &sgx_dh_session);
}

sgx_status_t responder_proc_msg2(const sgx_dh_msg2_t *msg2, sgx_dh_msg3_t *msg3)
{
    REQUIRED(msg2);
    REQUIRED(msg3);
    sgx_dh_session_enclave_identity_t initiator_identity;

    sgx_status_t status = sgx_dh_responder_proc_msg2(msg2, msg3, &sgx_dh_session, &aek, &initiator_identity);

    // TODO: Do something with `initiator_identity`
    sgx_measurement_t MRENCLAVE = initiator_identity.mr_enclave;
    return status;
}

sgx_status_t initiator_proc_msg3(const sgx_dh_msg3_t *msg3)
{
    REQUIRED(msg3);
    sgx_dh_session_enclave_identity_t responder_identity;

    sgx_status_t status = sgx_dh_initiator_proc_msg3(msg3, &sgx_dh_session, &aek, &responder_identity);

    // TODO: Do something with `initiator_identity`
    sgx_measurement_t MRENCLAVE = responder_identity.mr_enclave;
    return status;
}
