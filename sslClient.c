/* A simple SSL client.

   It connects and then forwards data from/to the terminal
   to/from the server
*/
#include "commonclient.h"
#include "read_write.h"
#define FAIL -1

int main(int count, char *strings[])
{   SSL_CTX *ctx;
    int server;
    SSL *ssl;
    char *hostname;  
      char *portnum ;

       if ( count != 3 )
    {
           printf("usage: %s <hostname> <portnum>\n", strings[0]);
           exit(0);
     }
    SSL_library_init();
    /* hostname=strings[1];*/
    /* portnum=strings[2];*/

    ctx = InitCTX();
    server = OpenConnection(hostname, atoi(portnum));
    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, server);    /* attach the socket descriptor */
    if ( SSL_connect(ssl) == FAIL )   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else
    {   
       /*char *msg = "Hello???";*/

        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);        /* get any certs */
	/*SSL_write(ssl, msg, strlen(msg));  encrypt & send message */
	/*bytes = SSL_read(ssl, buf, sizeof(buf));  get reply & decrypt */
	/*buf[bytes] = 0;*/
	/*printf("Received: \"%s\"\n", buf);*/
        read_write(ssl,server);
        SSL_free(ssl); /* release connection state */
    }
    close(server); /* close socket */
    SSL_CTX_free(ctx); /* release context */
    return 0;
}
