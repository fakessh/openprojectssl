/* A simple SSL client.

   It connects and then forwards data from/to the terminal
   to/from the server
*/
#include "common.h"
#include "client.h"
#include "read_write.h"

int main(int argc,char **argv)
  {
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *sbio;
    int sock;

    /* Build our SSL context*/
    ctx=initialize_ctx(ClientKEYFILE,ClientPASSWORD);

    /* Connect the TCP socket*/
    sock=tcp_connect();

    /* Connect the SSL socket */
    ssl=SSL_new(ctx);
    sbio=BIO_new_socket(sock,BIO_NOCLOSE);
    SSL_set_bio(ssl,sbio,sbio);
    if(SSL_connect(ssl)<=0)
      berr_exit("SSL connect error");
    check_cert_chain(ssl,ServerHOST);

    /* read and write */
    read_write(ssl,sock);

    destroy_ctx(ctx);
  }

