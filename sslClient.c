/* A simple SSL client.

   It connects and then forwards data from/to the terminal
   to/from the server
*/
/*#include "common.h"*/
#include "commonclient.h"
#include "read_writecommon.h"
#include "read_write.h"
#include "client.h"
// Simple structure to keep track of the handle, and
// // of what needs to be freed later.
             
typedef struct {
                int socket;
                SSL *sslHandle;
                SSL_CTX *sslContext;
                       } connection;
        
connection *c;
int sock;
int fd;
int main(int argc,char *argv[])
  {
    BIO *rbio;
    SSL_CTX *ctx;
    SSL *ssl;
    /*connection *c;*/
    /* Build our SSL context*/
    /*c->sslContext=initialize_ctx(ClientKEYFILE,ClientPASSWORD);*/
      sock  = sslConnect();
    /* Connect the TCP socket*/

    /* Connect the SSL socket */
    if ((ctx=SSL_CTX_new(SSLv23_client_method()))==NULL)
      berr_exit("Can't initialize SSLv3 context");

    ssl=SSL_new(ctx);
    rbio=BIO_new_socket((int)sock,BIO_NOCLOSE);
    SSL_set_bio(ssl,rbio,rbio);
    if (!SSL_set_cipher_list(ssl,"ALL"))
        berr_exit("Can't set cipher list");

    if(SSL_connect(ssl)<=0)
      berr_exit("SSL connect error");
    else	  
    check_cert_chain(ssl,ServerHOST);

    /* read and write */

    destroy_ctx(ssl);
    return 0;
  }

