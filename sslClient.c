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
typedef struct {
   int verbose_mode;
   int verify_depth;
   int always_continue;
} mydata_t;
static int mydata_index;

int main(int argc,char *argv[])
  {
    /*connection *c;*/
    /* Build our SSL context*/
    /*c->sslContext=initialize_ctx(ClientKEYFILE,ClientPASSWORD);*/
    sock  = sslConnect(&(*c));
  FILE *fp;
  SSL_CTX *ctx;
  SSL *ssl;
  X509 *x509;
  EVP_PKEY *pkey;
  BIO *cbio,*out;
  int len,verify_depth=1;
  int sock;
  char *cp,*tmpbuf;
  int r;
  mydata_t mydata;
  char *host = "localhost";
  char *password_cb = "";
  SSL_load_error_strings();
  // Initialize the SSL library.
  SSL_library_init();
  OpenSSL_add_all_ciphers();
  // Create an SSL context that will be used to communicate over TLSv1 protocol.
  // See RFC 2818.
  if ((fp = fopen(ClientKEYFILE,"r")) == NULL){
       printf("Can't open certificate %s : %s\n",ClientKEYFILE,strerror(errno));
       exit(EXIT_FAILURE);
  }
  if ((ctx=SSL_CTX_new(SSLv23_client_method()))==NULL)
      berr_exit("Can't initialize SSLv3 context");
  pkey=PEM_read_PrivateKey(fp, NULL,password_cb,(char *)ClientKEYFILE);
  if (pkey == NULL)
     exit(EXIT_FAILURE);
  if ((x509=PEM_read_X509_AUX(fp, NULL, 0, NULL)) == NULL)
      berr_exit("Can't read certificate");
  if (SSL_CTX_use_PrivateKey(ctx,pkey)!= 1)
      berr_exit("Can't use Private Key");
  if (SSL_CTX_use_certificate(ctx,x509) != 1)
      berr_exit("Can't add extra chain cert");
  if (!SSL_CTX_load_verify_locations(ctx,NULL,"./"))
      berr_exit("Can't load verify locations");
  // Connect to remote peer either proxy or https server
  // using classic Socket API. Get the actual host name through host variable.
  sock=tcpConnect(&(*c));
  //Connect SSL to CTX
  ssl=SSL_new(ctx);
  // Associate BIO cbio with socket s 
  cbio=BIO_new_socket((int)sock,BIO_NOCLOSE);
  SSL_set_bio(ssl,cbio,cbio);
  if (!SSL_set_cipher_list(ssl,"ALL"))
      berr_exit("Can't set cipher list");
//  SSL_set_connect_state(ssl);
  mydata_index = SSL_get_ex_new_index(0, "mydata index", NULL, NULL, NULL);
  SSL_set_verify(ssl,SSL_VERIFY_PEER,verify_callback);
  SSL_set_verify_depth(ssl,verify_depth+1);
  mydata.verify_depth = verify_depth;
  mydata.verbose_mode=1;
  mydata.always_continue=0;
  SSL_set_ex_data(ssl, mydata_index, &mydata);

  //SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

  out = BIO_new_fp(stdout, BIO_CLOSE | BIO_FP_TEXT );
  // Establish a link with the remote SSL session using chosen transport.
  if(SSL_connect(ssl) <= 0){
     berr_exit("Error connecting to server's session");
  }
  check_cert_chain(ssl,host); 
  len=(int)strlen(cp);
  r = SSL_write(ssl,cp,len);
  for (;;){
       len = SSL_read(ssl, tmpbuf, 1024);
       if(len <= 0) break;
       BIO_write(out, tmpbuf, len);
  }
  BIO_write(out,"\n",sizeof("\n"));
  SSL_shutdown(ssl);
  BIO_free_all(cbio);

    /* read and write */
    return 0;
  }

