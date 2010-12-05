#ifndef _read_writecommon_h
#define _read_writecommon_h

void read_write(SSL *ssl,int sock);
typedef struct 
{
   
     int socket;
     SSL *sslHandle;
     SSL_CTX *sslContext;
} connection;

connection *c sslConnect;

#endif

