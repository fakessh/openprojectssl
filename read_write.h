#ifndef _read_write_h
#define _read_write_h

void read_write(SSL *ssl,int sock);
#define READ BIO *SSL_get_rbio(SSL *ssl)
#define WRITE BIO *SSL_get_wbio(SSL *ssl)
#endif

