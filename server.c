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

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000
#define PORT 55555

/* some common lengths */
#define IP_HDR_LEN 20
#define ETH_HDR_LEN 14
#define ARP_PKT_LEN 28

int debug;
char *progname;

SSL_CTX *sslctx;
SSL *cSSL;

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

void InitializeSSL() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void DestroySSL() {
    ERR_free_strings();
    EVP_cleanup();
}

void ShutdownSSL() {
    SSL_shutdown(cSSL);
    SSL_free(cSSL);
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

    int external_ip_set = 0;

    char internal_ip[16] = "";

    unsigned short int port = PORT;

    unsigned long int tap2net = 0, net2tap = 0;

    progname = argv[0];

    /* Check command line options */
    while ((option = getopt(argc, argv, "i:n:g:e:p:uahd")) > 0) {
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

    if (argc > 0) {
        my_err("Too many options!\n");
        usage();
    }

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



    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        my_err("error on bind");
        exit(1);
    }

    fromlen = sizeof(from);


//    l = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
//
//    if (l < 0) {
//        my_err("error on initialization");
//        exit(1);
//    }
//
//    do_debug("%s packet received from %s:%d", buf, inet_ntoa(from.sin_addr), ntohs(from.sin_port));

    while (1) {
        FD_ZERO(&fdset);
        FD_SET(tap_fd1, &fdset);
        FD_SET(s, &fdset);

        if (select(tap_fd1 + s + 1, &fdset, NULL, NULL, NULL) < 0) {
            my_err("error on select");
            exit(1);
        }

        if (FD_ISSET(tap_fd1, &fdset)) { // data coming from tun/tap

            l = read(tap_fd1, buf, sizeof(buf));

            if (l < 0) {
                my_err("error on read tun/tap");
                exit(1);
            }

            tap2net++;

            do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, l);

            l = sendto(s, buf, l, 0, (struct sockaddr *) &sout, fromlen);
            if (l < 0) {
                my_err("error on sending to the network");
                exit(1);
            }

            do_debug("TAP2NET %lu: Written %d bytes to the network\n", tap2net, l);

        }

        if (FD_ISSET(s, &fdset)) { // data coming from network

            l = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &from, &fromlen);

            if (l < 0) {
                my_err("error on receving from the network");
                exit(1);
            }

            do_debug("packet received from %s:%d", inet_ntoa(from.sin_addr), ntohs(from.sin_port));


            l = cwrite(tap_fd2, buf, l);

            if (l < 0) {
                my_err("error on writing to tun/tap interface");
                exit(1);
            }
        }
    }

    return (0);
}