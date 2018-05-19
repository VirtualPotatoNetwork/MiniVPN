#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <string.h>

void digest_message(const unsigned char *message, size_t message_len, unsigned char **digest, unsigned int *digest_len)
{
    EVP_MD_CTX *mdctx;
    printf("1\n");
    if((mdctx = EVP_MD_CTX_create()) == NULL)
        printf("error\n");
    printf("2\n");
    if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
        printf("error\n");
    printf("3\n");
    if(1 != EVP_DigestUpdate(mdctx, message, message_len))
        printf("error\n");
    printf("4\n");
    if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
        printf("error\n");
    printf("5\n");
    if(1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len))
        printf("error\n");
    printf("%s\n",digest);
    EVP_MD_CTX_destroy(mdctx);
}


unsigned char* myhmac_sha256(const unsigned char *key, int keylen,const unsigned char *data, int datalen,unsigned char *result, unsigned int* resultlen)
{
    return HMAC(EVP_sha256(), key, keylen, data, datalen, result, resultlen);
}


int main (void)
{
    /* Set up the key and iv. Do I need to say to not hard code these in a
     * real application? :-)
     */

    /* A 256 bit key */
    unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
    /* A 128 bit IV */
    unsigned char *iv = (unsigned char *)"0123456789012345";

    /* Message to be encrypted */
    unsigned char *plaintext =
            (unsigned char *)"coldreyiz";

    /* Buffer for ciphertext. Ensure the buffer is long enough for the
     * ciphertext which may be longer than the plaintext, dependant on the
     * algorithm and mode
     */
    unsigned char ciphertext[128];

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[128];

    unsigned int decryptedtext_len, ciphertext_len;
    ciphertext_len=128;



    /*hmac_sha256(key, strlen(keylen),
                plaintext, strlen(plaintext),
    unsigned char *result, unsigned int* resultlen)*/

    //digest_message(plaintext,  strlen ((char *)plaintext),ciphertext, &ciphertext_len);

    myhmac_sha256("01234567890123456789012345678901", 32,"coldreyiz2", 10,ciphertext, &ciphertext_len);

    int size=strlen(ciphertext);
    printf("size= %d",size);
    for(int i=0;i<size;i++)
    {
        printf("%d,%d\n",ciphertext[i],i);
    }


    return 0;
}
//int hmac_it(const byte* msg, size_t mlen, byte** val, size_t* vlen, EVP_PKEY* pkey)
//{
//    /* Returned to caller */
//    int result = -1;
//
//    if(!msg || !mlen || !val || !pkey) {
//        assert(0);
//        return -1;
//    }
//
//    if(*val)
//        OPENSSL_free(*val);
//
//    *val = NULL;
//    *vlen = 0;
//
//    EVP_MD_CTX* ctx = NULL;
//
//    do
//    {
//        ctx = EVP_MD_CTX_create();
//        assert(ctx != NULL);
//        if(ctx == NULL) {
//            printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        const EVP_MD* md = EVP_get_digestbyname("SHA256");
//        assert(md != NULL);
//        if(md == NULL) {
//            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        int rc = EVP_DigestInit_ex(ctx, md, NULL);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestInit_ex failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestSignInit failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestSignUpdate failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        size_t req = 0;
//        rc = EVP_DigestSignFinal(ctx, NULL, &req);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestSignFinal failed (1), error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        assert(req > 0);
//        if(!(req > 0)) {
//            printf("EVP_DigestSignFinal failed (2), error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        *val = OPENSSL_malloc(req);
//        assert(*val != NULL);
//        if(*val == NULL) {
//            printf("OPENSSL_malloc failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        *vlen = req;
//        rc = EVP_DigestSignFinal(ctx, *val, vlen);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestSignFinal failed (3), return code %d, error 0x%lx\n", rc, ERR_get_error());
//            break; /* failed */
//        }
//
//        assert(req == *vlen);
//        if(req != *vlen) {
//            printf("EVP_DigestSignFinal failed, mismatched signature sizes %ld, %ld", req, *vlen);
//            break; /* failed */
//        }
//
//        result = 0;
//
//    } while(0);
//
//    if(ctx) {
//        EVP_MD_CTX_destroy(ctx);
//        ctx = NULL;
//    }
//
//    /* Convert to 0/1 result */
//    return !!result;
//}

//int verify_it(const byte* msg, size_t mlen, const byte* val, size_t vlen, EVP_PKEY* pkey)
//{
//    /* Returned to caller */
//    int result = -1;
//
//    if(!msg || !mlen || !val || !vlen || !pkey) {
//        assert(0);
//        return -1;
//    }
//
//    EVP_MD_CTX* ctx = NULL;
//
//    do
//    {
//        ctx = EVP_MD_CTX_create();
//        assert(ctx != NULL);
//        if(ctx == NULL) {
//            printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        const EVP_MD* md = EVP_get_digestbyname("SHA256");
//        assert(md != NULL);
//        if(md == NULL) {
//            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        int rc = EVP_DigestInit_ex(ctx, md, NULL);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestInit_ex failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestSignInit failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestSignUpdate failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        byte buff[EVP_MAX_MD_SIZE];
//        size_t size = sizeof(buff);
//
//        rc = EVP_DigestSignFinal(ctx, buff, &size);
//        assert(rc == 1);
//        if(rc != 1) {
//            printf("EVP_DigestVerifyFinal failed, error 0x%lx\n", ERR_get_error());
//            break; /* failed */
//        }
//
//        assert(size > 0);
//        if(!(size > 0)) {
//            printf("EVP_DigestSignFinal failed (2)\n");
//            break; /* failed */
//        }
//
//        const size_t m = (vlen < size ? vlen : size);
//        result = !!CRYPTO_memcmp(val, buff, m);
//
//        OPENSSL_cleanse(buff, sizeof(buff));
//
//    } while(0);
//
//    if(ctx) {
//        EVP_MD_CTX_destroy(ctx);
//        ctx = NULL;
//    }
//
//    /* Convert to 0/1 result */
//    return !!result;
//}