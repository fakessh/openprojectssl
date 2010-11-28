#ifndef _common_h
#define _common_h

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

//#define CA_LIST "root.pem"
#define CA_LIST "cacert.pem"
#define ServerHOST	"localhost"
#define ClientHOST	"localhost"
#define RANDOM  "/dev/urandom"
#define ServerPORT	1080
#define BUFSIZZ 1024

extern BIO *bio_err;
int berr_exit (char *string);
int err_exit(char *string);
int RAND_load_file(const char *filename,long max_bytes);
void ERR_print_errors(BIO *bp);
SSL_CTX *initialize_ctx(char *keyfile, char *password);
void destroy_ctx(SSL_CTX *ctx);
char * inet_ntoa ( struct in_addr in );
#endif

#define THREAD_CC   *
#define THREAD_TYPE                    pthread_t
#define THREAD_CREATE(tid, entry, arg) pthread_create(&(tid), NULL, \
                                                      (entry), (arg))
void THREAD_CC server_thread(void *arg);

int verify_callback(int ok, X509_STORE_CTX *store);

void check_cert_chain(SSL *ssl,char *host);
