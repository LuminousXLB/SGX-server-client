#include "Enclave_t.h"

#include "sgx_dh.h"
#include "sgx_trts.h"
#include "tlibc/mbusafecrt.h"

#define REQUIRED(ptr) \
    if (ptr == NULL)  \
    return SGX_ERROR_INVALID_PARAMETER

sgx_dh_session_t sgx_dh_session;
sgx_key_128bit_t aek;

/* 
 * Encrypt & Decrypt using AES-CTR-128
 */
sgx_status_t aes_ctr_128_encrypt(const uint8_t *plaintext, uint32_t pt_len, uint8_t *ciphertext, uint32_t *nonce)
{
    uint8_t counter[4];
    sgx_status_t status = sgx_read_rand(counter, 4);
    if (status != SGX_SUCCESS)
    {
        return status;
    }

    memcpy_s((void *)nonce, sizeof(uint32_t), counter, 4);

    return sgx_aes_ctr_encrypt(&aek, plaintext, pt_len, counter, 1, ciphertext);
}

sgx_status_t aes_ctr_128_decrypt(uint32_t nonce, const uint8_t *ciphertext, uint32_t ct_len, uint8_t *plaintext)
{
    uint8_t counter[4];
    memcpy_s(counter, 4, (void *)&nonce, sizeof(uint32_t));

    return sgx_aes_ctr_decrypt(&aek, ciphertext, ct_len, counter, 1, plaintext);
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
