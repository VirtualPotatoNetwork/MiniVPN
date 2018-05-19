#include "openssl/ssl.h"
#include "openssl/bio.h"
#include "openssl/err.h"

#include "stdio.h"
#include "string.h"

int main()
{
    BIO * bio;
    SSL * ssl;
    SSL_CTX * ctx;

    SSL_library_init ();
    int p;

    char * request = "naber";
    char r[1024];

    /* Set up the library */

    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();


    /* Set up the SSL context */

    ctx = SSL_CTX_new(SSLv23_client_method());

    /* Load the trust store */
    const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(ctx, flags);

    SSL_CTX_load_verify_locations(ctx, "server.crt", NULL);

    BIO* bo = BIO_new( BIO_s_mem() );
//    BIO_write( bo, mKey,strlen(mKey));

    EVP_PKEY* pkey = 0;
    PEM_read_bio_PrivateKey( bo, &pkey, 0, 0 );

    BIO_free(bo);


    RSA* rsa = EVP_PKEY_get1_RSA( pkey );

    /*if(! )
    {
        printf("Error loading trust store\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }*/

    /* Setup the connection */

    bio = BIO_new_ssl_connect(ctx);
;
    /* Set the SSL_MODE_AUTO_RETRY flag */

    BIO_get_ssl(bio, & ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    /* Create and setup the connection */

    BIO_set_conn_hostname(bio, "127.0.0.1:8888");

    if(BIO_do_connect(bio) <= 0)
    {
        fprintf(stderr, "Error attempting to connect\n");
        ERR_print_errors_fp(stderr);
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return 0;
    }

    /* Check the certificate */

    if(SSL_get_verify_result(ssl) != X509_V_OK)
    {
        fprintf(stderr, "Certificate verification error: %i\n", SSL_get_verify_result(ssl));
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return 0;
    }

    /* Send the request */
    while(1)
    {
        BIO_write(bio, request, strlen(request));
        printf("%s\n",request);
    }

    /* Read in the response */

    for(;;)
    {
        p = BIO_read(bio, r, 1023);
      //  if(p <= 0) break;
       // r[p] = 0;
        printf("%s", r);
    }

    /* Close the connection and free the context */

    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    return 0;
}



