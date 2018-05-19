#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "message.h"
#include "encr_dec.h"

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000
#define PORT 55555

/* some common lengths */
#define IP_HDR_LEN 20
#define ETH_HDR_LEN 14
#define ARP_PKT_LEN 28

int debug;
char *progname;
char key[33] = "01234567890123456789012345678901";
char iv[17] = "*123456789012345";

/**************************************************************************
 *SSL_create_conn: creates SSL connection and waits for a client to conn. *
 *                                                                        *
 **************************************************************************/
void SSL_create_conn()
{

}


/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            needs to reserve enough space in *dev.                      *
 **************************************************************************/
int tun_alloc(char *dev, int flags) {

    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    if (ioctl(fd, TUNSETPERSIST, 1) < 0) {
        perror("enabling TUNSETPERSIST");
        exit(1);
    }

/*
  // DELETE TUN0 interface
  if(ioctl(tap_fd, TUNSETPERSIST, 0) < 0){
      perror("disabling TUNSETPERSIST");
      exit(1);
  }
*/


    strcpy(dev, ifr.ifr_name);

    return fd;
}

/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is    *
 *        returned.                                                       *
 **************************************************************************/
int cread(int fd, char *buf, int n) {

    int nread;

    if ((nread = read(fd, buf, n)) < 0) {
        perror("Reading data");
        exit(1);
    }
    return nread;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is  *
 *         returned.                                                      *
 **************************************************************************/
int cwrite(int fd, char *buf, int n) {

    int nwrite;

    if ((nwrite = write(fd, buf, n)) < 0) {
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}

/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts those into "buf".    *
 *         (unless EOF, of course)                                        *
 **************************************************************************/
int read_n(int fd, char *buf, int n) {

    int nread, left = n;

    while (left > 0) {
        if ((nread = cread(fd, buf, left)) == 0) {
            return 0;
        } else {
            left -= nread;
            buf += nread;
        }
    }
    return n;
}

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)                                *
 **************************************************************************/
void do_debug(char *msg, ...) {

    va_list argp;

    if (debug) {
        va_start(argp, msg);
        vfprintf(stderr, msg, argp);
        va_end(argp);
    }
}

/**************************************************************************
 * my_err: prints custom error messages on stderr.                        *
 **************************************************************************/
void my_err(char *msg, ...) {

    va_list argp;

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

int create_socket(int port)
{
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    return s;
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "server.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void SSL_conn_server()
{
    int TCP_sock;
    SSL_CTX *ctx;

    init_openssl();
    ctx = create_context();

    configure_context(ctx);

    //*******PORT NUMBER*********

    TCP_sock = create_socket(PORT);

    /* Handle connections */
    while(1) {

        struct sockaddr_in addr;
        uint len = sizeof(addr);
        SSL *ssl;
        int is_connected=0;

        const char reply[] = "test\n";

        int client = accept(TCP_sock, (struct sockaddr*)&addr, &len);

        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }
//        printf("connected\n");
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
        }
        else
        {
            char mes[49];
            strcpy(mes,key);
            strcat(mes,iv);
            mes[48]='\0';
            printf("%s\n",mes);
            while (1)
            {
                char reply[49];
                SSL_write(ssl, mes, strlen(mes));

                SSL_read(ssl, reply, 48);
                reply[48]='\0';
//                printf("%s\n",mes);
//                printf("%d\n",strlen(mes));
//                printf("%d\n",strlen(key));
                if(strcmp(reply,mes) == 0)
                {
                    is_connected=1;
                    break;
                }
            }
        }

        SSL_free(ssl);
        close(client);

        if(is_connected == 1)
        {
            break;
        }
    }

    close(TCP_sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    printf("Key exchange is done\n");
}

void SSL_conn_client(char* ext_ip)
{
    char ip_port[50];
    char *port = ":55555";
    BIO * bio;
    SSL * ssl;
    SSL_CTX * ctx;
    SSL_library_init ();
    int p;

    char key[33];

    strcpy(ip_port,ext_ip);
    strcat(ip_port,port);
    printf("%s\n",ip_port);


    /* Set up the library */

    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    /* Set up the SSL context */

    ctx = SSL_CTX_new(SSLv23_client_method());

    /* Load the trust store */
    const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(ctx, flags);

    if(ctx==NULL)
    {
        printf("null\n");
    }

    SSL_CTX_load_verify_locations(ctx, "server.crt", NULL);

    BIO* bo = BIO_new( BIO_s_mem() );
    //BIO_write( bo, mKey,strlen(mKey));

    EVP_PKEY* pkey = 0;
    PEM_read_bio_PrivateKey( bo, &pkey, 0, 0 );

    BIO_free(bo);


    //RSA* rsa = EVP_PKEY_get1_RSA( pkey );

    /*if(! )
    {
        printf("Error loading trust store\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }*/

    /* Setup the connection */

    bio = BIO_new_ssl_connect(ctx);

    /* Set the SSL_MODE_AUTO_RETRY flag */

    BIO_get_ssl(bio, & ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    /* Create and setup the connection */

    BIO_set_conn_hostname(bio, ip_port);

    if(BIO_do_connect(bio) <= 0)
    {
        fprintf(stderr, "Error attempting to connect\n");
        ERR_print_errors_fp(stderr);
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return;
    }

    /* Check the certificate */

    if(SSL_get_verify_result(ssl) != X509_V_OK)
    {
        fprintf(stderr, "Certificate verification error: %li\n", SSL_get_verify_result(ssl));
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return;
    }

    /* Send the request */
/*while(1){
    BIO_write(bio, request, strlen(request));
	printf("%s\n",request);}
BIO_write(bio, request, strlen(request));*/
    /* Read in the response */

    for(;;)
    {
        char temp[49];
        p = BIO_read(bio, temp, 49);

        if(p > 0)
        {
            BIO_write(bio, temp, p);
            strncpy(key,temp,32);
            key[32] = '\0';
            printf("%s\n", key);
            strncpy(iv,temp+32,16);
            iv[16]='\0';
            printf("%s\n", iv);
            break;
        }
    }

    /* Close the connection and free the context */

    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    printf("****client is out****\n");
}

/**************************************************************************
 * usage: prints usage and exits.                                         *
 **************************************************************************/
void usage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s -i <ifacename> [-s <serverIP>] [-p <port>] [-u|-a] [-d]\n", progname);
    fprintf(stderr, "%s -h\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
    fprintf(stderr, "-s <serverIP> (mandatory)\n");
    fprintf(stderr,
            "-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 55555\n");
    fprintf(stderr, "-u|-a: use TUN (-u, default) or TAP (-a)\n");
    fprintf(stderr, "-d: outputs debug information while running\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in sin, sout, from;

    int fd, s, fromlen, soutlen, l;

    int internal_socket;
    int external_socket;

    char c, *p, *ip;
    char buf[BUFSIZE];
    fd_set fdset;

    int tap_fd1, tap_fd2, option;
    int flags = IFF_TUN;
    char if_name[IFNAMSIZ] = "";
    char ifn_name[IFNAMSIZ] = "";
    int header_len = IP_HDR_LEN;

    struct sockaddr_in udp_client;
    char external_ip[16] = "";

    int external_ip_set = 0, is_server =0;

    char internal_ip[16] = "";

    unsigned short int port = PORT;

    unsigned long int tap2net = 0, net2tap = 0;

    progname = argv[0];

    /* Check command line options */
    while ((option = getopt(argc, argv, "i:n:sc:g:e:p:uahd")) > 0) {
        switch (option) {
            case 'e': // external gateway ip
                strncpy(external_ip, optarg, 15);
                external_ip_set = 1;
                break;
            case 'g':
                strncpy(internal_ip, optarg, 15);
                break;
            case 'd':
                debug = 1;
                break;
            case 's':
                is_server = 1;
                break;
            case 'c':
                is_server = 0;
                break;
            case 'h':
                usage();
                break;
            case 'i':
                strncpy(if_name, optarg, IFNAMSIZ - 1);
                break;
            case 'n':
                strncpy(ifn_name, optarg, IFNAMSIZ - 1);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'u':
                flags = IFF_TUN;
                break;
            case 'a':
                flags = IFF_TAP;
                header_len = ETH_HDR_LEN;
                break;
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }

    argv += optind;
    argc -= optind;

    if (*if_name == '\0') {
        my_err("Must specify interface name!\n");
        usage();
    }

    /* initialize tun/tap interface */
    if ((tap_fd1 = tun_alloc(if_name, flags | IFF_NO_PI)) < 0) {
        my_err("Error connecting to tun/tap interface %s!\n", if_name);
        exit(1);
    }

    do_debug("Successfully connected to interface %s\n", if_name);

    if ((tap_fd2 = tun_alloc(ifn_name, flags | IFF_NO_PI)) < 0) {
        my_err("ERROR connecting to tun/tap interface %s!\n", ifn_name);
    }


    do_debug("Successfully connected to interface %s\n", ifn_name);

    memset((char *) &sin, 0, sizeof(sin));

    // this listens network.
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);

    //
    // this will send to external gateway.
    external_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sout.sin_family = AF_INET;
    sout.sin_port = htons(PORT);
    inet_aton(external_ip, &sout.sin_addr);


//    external_socket = socket(AF_INET, SOCK_STREAM, 0);
//    sout.sin_family = AF_INET;
//    sout.sin_addr.s_addr = htonl(remote_ip);
//    sout.sin_port = htons(PORT);
//
//    if (connect(external_socket, (struct sockaddr *)&sout, sizeof(sout)) < 0) {
//
//    }

//    if (external_ip_set == 0) {
//        // bind and listen
//    } else {
//        // connect to
//    }

//    l = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
//
//    if (l < 0) {
//        my_err("error on initialization");
//        exit(1);
//    }
//
//    do_debug("%s packet received from %s:%d", buf, inet_ntoa(from.sin_addr), ntohs(from.sin_port));

    if (is_server) {
        SSL_conn_server();
    } else {
        SSL_conn_client(external_ip);
    }

    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        my_err("error on bind\n");
        exit(1);
    }

    fromlen = sizeof(from);

    char plaintext[1024];
    char cipher[1024];

    while (1) {
        FD_ZERO(&fdset);
        FD_SET(tap_fd1, &fdset);
        FD_SET(s, &fdset);

        if (select(tap_fd1 + s + 1, &fdset, NULL, NULL, NULL) < 0) {
            my_err("error on select");
            exit(1);
        }

        if (FD_ISSET(tap_fd1, &fdset)) { // data coming from tun/tap
            memset(cipher,'\0',1024);
            l = read(tap_fd1, buf, sizeof(buf));

            encrypt(buf, l,key,iv,cipher);
            char hmac_out[33];
            int hmac_len=33;
            memset(hmac_out,'\0',33);
            myhmac_sha256(key,strlen(key),cipher,strlen(cipher),hmac_out,&hmac_len);
            strcat(cipher,hmac_out);
            printf("%s\n",cipher);

            if (l < 0) {
                my_err("error on read tun/tap");
                exit(1);
            }

            tap2net++;

            do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, l);

            l = sendto(s, cipher, l, 0, (struct sockaddr *) &sout, fromlen);
            if (l < 0) {
                my_err("error on sending to the network");
                exit(1);
            }

            do_debug("TAP2NET %lu: Written %d bytes to the network\n", tap2net, l);

        }

        if (FD_ISSET(s, &fdset)) { // data coming from network

            memset(plaintext,'\0',1024);
            l = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &from, &fromlen);
            do_debug("packet received from %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
            char hmac_out[33];
            memset(hmac_out,'\0',33);
            strncpy(hmac_out,plaintext+l-33,33);
            plaintext[l-33]='\0';
            char temp_out[33];
            int temp_out_len=33;
            memset(temp_out,'\0',33);
            myhmac_sha256(key,strlen(key),plaintext,strlen(plaintext),temp_out,&temp_out_len);

            if(strcmp(hmac_out,temp_out) == 0)
            {
                printf("saplaaaaa\n");
            }
            else
            {
                printf("not equal\n");
                continue;
            }

            decrypt(buf, l,key,iv,plaintext);
            printf("%s\n",plaintext);
            if (l < 0) {
                my_err("error on receving from the network");
                exit(1);
            }

            do_debug("packet received from %s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));


            l = cwrite(tap_fd2, plaintext, l);

            if (l < 0) {
                my_err("error on writing to tun/tap interface");
                exit(1);
            }
        }
    }

    return (0);
}