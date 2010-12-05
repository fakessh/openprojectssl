#include "commonclient.h"
#include "client.h"

// Simple structure to keep track of the handle, and
// of what needs to be freed later.

// For this example, we'll be testing on openssl.org
#define SERVER  "127.0.0.1"
#define PORT 1080
typedef struct {
    int socket;
    SSL *sslHandle;
    SSL_CTX *sslContext;
    } connection;

connection *c;

char *keyfile = "mycert.pem";
char *password = "";

BIO *bio_err=0;
static char *pass;
static int password_cb(char *buf,int num,int rwflag,void *userdata);

void destroy_ctx(SSL_CTX *ctx)
  {
    SSL_CTX_free(ctx);
  }

int verify_callback(int ok, X509_STORE_CTX *store)
{
    char data[256];
    if (ok)
    {
        fprintf(stderr, "verify_callback\n");
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int  depth = X509_STORE_CTX_get_error_depth(store);
        int  err = X509_STORE_CTX_get_error(store);

        fprintf(stderr, "certificate at depth: %i\n", depth);
        X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
        fprintf(stderr, "issuer = %s\n", data);
        X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
        fprintf(stderr, "subject = %s\n", data);
        fprintf(stderr, "error status:  %i:%s\n", err, X509_verify_cert_error_string(err)
);
    }

    return ok;
}

/* A simple error and exit routine*/
int err_exit(char *string)
  {
    fprintf(stderr,"%s\n",string);
    exit(0);
  }

/* Print SSL errors and exit*/
int berr_exit(char *string)
  {
    BIO_printf(bio_err,"%s\n",string);
    ERR_print_errors(bio_err);
    exit(0);
  }

/*The password code is not thread safe*/
static int password_cb(char *buf,int num,int rwflag,void *userdata)
  {
    if(num<strlen(pass)+1)
      return(0);

    strcpy(buf,pass);
    return(strlen(pass));
  }

// Establish a regular tcp connection
int tcpConnect ()
{
  int error, handle;
  struct hostent *host;
  struct sockaddr_in server;

  host = gethostbyname (SERVER);
  handle = socket (AF_INET, SOCK_STREAM, 0);
  if (handle == -1)
    {
      perror ("Socket");
      handle = 0;
    }
  else
    {
      server.sin_family = AF_INET;
      server.sin_port = htons (PORT);
      server.sin_addr = *((struct in_addr *) host->h_addr);
      bzero (&(server.sin_zero), 8);

      error = connect (handle, (struct sockaddr *) &server,
                       sizeof (struct sockaddr));
      if (error == -1)
        {
          perror ("Connect");
          handle = 0;
        }
    }

  return handle;
}

// Establish a connection using an SSL layer
int *sslConnect (connection *c)
{
  connection *fd;


  fd = malloc (sizeof (connection));
  fd->sslHandle = NULL;
  fd->sslContext = NULL;

  fd->socket = tcpConnect ();
  if (fd->socket)
    {
      // Register the error strings for libcrypto & libssl
      SSL_load_error_strings ();
      // Register the available ciphers and digests
      SSL_library_init ();

      // New context saying we are a client, and using SSL 2 or 3
      fd->sslContext = SSL_CTX_new (SSLv23_client_method ());
      if (fd->sslContext == NULL)
        ERR_print_errors_fp (stderr);
    /* Load our keys and certificates*/
    if(!(SSL_CTX_use_certificate_file(fd->sslContext,keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Couldn't read certificate file");

    pass=password;
    SSL_CTX_set_default_passwd_cb(fd->sslContext,password_cb);
    if(!(SSL_CTX_use_PrivateKey_file(fd->sslContext,keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Couldn't read key file");

    /* Load the CAs we trust*/
    if(!(SSL_CTX_load_verify_locations(fd->sslContext,CA_LIST,0)))
      berr_exit("Couldn't read CA list");
    if (!(SSL_CTX_check_private_key(fd->sslContext)))
      berr_exit("Private key does match the public certificate");
     
    SSL_CTX_set_verify_depth(fd->sslContext,1);
    SSL_CTX_set_verify(fd->sslContext,
      SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);

    /* Load randomness */
    if(!(RAND_load_file(RANDOM,1024*1024)))
      berr_exit("Couldn't load randomness");


      // Create an SSL struct for the connection
      fd->sslHandle = SSL_new (fd->sslContext);
      if (fd->sslHandle == NULL)
        ERR_print_errors_fp (stderr);

      // Connect the SSL struct to our connection
      if (!SSL_set_fd (fd->sslHandle, c->socket))
        ERR_print_errors_fp (stderr);

      // Initiate SSL handshake
      if (SSL_connect (fd->sslHandle) != 1)
        ERR_print_errors_fp (stderr);
    }
  else
    {
      perror ("Connect failed");
    }

  return fd;
}

// Disconnect & free connection struct
void sslDisconnect (connection *c)
{
  if (c->socket)
    close (c->socket);
  if (c->sslHandle)
    {
      SSL_shutdown (c->sslHandle);
      SSL_free (c->sslHandle);
    }
  if (c->sslContext)
    SSL_CTX_free (c->sslContext);

  free (c);
}

// Read all available text from the connection
char *sslRead (connection *c)
{
  const int readSize = 1024;
  char *rc = NULL;
  int received, count = 0;
  char buffer[1024];

  if (c)
    {
      while (1)
        {
          if (!rc)
            rc = malloc (readSize * sizeof (char) + 1);
          else
            rc = realloc (rc, (count + 1) *
                          readSize * sizeof (char) + 1);

          received = SSL_read (c->sslHandle, buffer, readSize);
          buffer[received] = '\0';

          if (received > 0)
            strcat (rc, buffer);

          if (received < readSize)
            break;
          count++;
        }
    }

  return rc;
}

// Write text to the connection
void sslWrite (connection *c, char *text)
{
  if (c)
    SSL_write (c->sslHandle, text, strlen (text));
}


/* Check that the common name matches the host name*/
void check_cert_chain(SSL *ssl,char *host)
  {

    X509 *peer;
    char  peer_CN[256];

    /*int i;*/

    printf("check certificate was called with  host= %s\n",host);
    if(SSL_get_verify_result(ssl)!=X509_V_OK)
      berr_exit("Certificate doesn't verify");

    /*Check the cert chain. The chain length
      is automatically checked by OpenSSL when we
      set the verify depth in the ctx */

    /*Check the common name*/
    peer=SSL_get_peer_certificate(ssl);
    X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
      NID_commonName, peer_CN, 256);
    printf("Peer CN=%s  & host= %s\n",peer_CN,host);
    printf("strcasecmp(%s,%s)=%d\n",peer_CN,host,strcasecmp(peer_CN,host));


    if(strcasecmp(peer_CN,host))
    	err_exit("Common name doesn't match host name");
  }
