/* A simple SSL client.

   It connects and then forwards data from/to the terminal
   to/from the server
*/
/*#include "common.h"*/
#include "commonclient.h"
#include "read_write.h"


// Simple structure to keep track of the handle, and
// // of what needs to be freed later.
typedef struct {
     int socket;
     SSL *sslHandle;
     SSL_CTX *sslContext;
} connection;
             


int main(int argc,char **argv)
  {
    /*BIO *sbio;*/
      /*int sock;*/
    connection *c;
    /* Build our SSL context*/
    /*c->sslContext=initialize_ctx(ClientKEYFILE,ClientPASSWORD);*/
    c = sslConnect ();
    /* Connect the TCP socket*/
    /*c->socket=tcp_connect();*/

    /* Connect the SSL socket */
    /*c->sslHandle=SSL_new(c->sslContext);*/
    /*SSL_set_fd(c->sslContext,c->socket);*/
    /*BIO_s_socket();*/
    /*BIO_set_fd(ssl,sock,BIO_NOCLOSE);*/
    /*BIO_get_fd(ssl,sock);*/
    /*SSL_get_rbio(ssl,sock,BIO_NOCLOSE);*/     
    /*sbio=BIO_new_socket(c->socket,BIO_NOCLOSE);*/
    /*SSL_set_bio(c->sslContext,sbio,sbio);*/
    /*if(SSL_connect(c->sslContext)<=0)*/
    /*  berr_exit("SSL connect error");*/
    /*else*/	  
    /*check_cert_chain(c->sslContext,ServerHOST);*/

    /* read and write */
    read_write(c->sslContext,c->socket);

    destroy_ctx(c->sslContext);
  }

