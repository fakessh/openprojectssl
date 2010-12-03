#include "common.h"
#include "client.h"

BIO *bio_err=0;
static char *pass;
static int password_cb(char *buf,int num,int rwflag,void *userdata);
static void sigpipe_handle(int x);

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

static void sigpipe_handle(int x){
}

SSL_CTX *initialize_ctx(char *keyfile,char *password)
  {
    SSL_METHOD *meth;
    SSL_CTX *ctx;

    if(!bio_err){
      /* Global system initialization*/
      SSL_library_init();
      SSL_load_error_strings();

      /* An error write context */
      bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);
    }

    /* Set up a SIGPIPE handler */
    signal(SIGPIPE,sigpipe_handle);

    /* Create our context*/
    meth=SSLv3_method();
    ctx=SSL_CTX_new(meth);

    /* Load our keys and certificates*/
    if(!(SSL_CTX_use_certificate_file(ctx,keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Couldn't read certificate file");

    pass=password;
    SSL_CTX_set_default_passwd_cb(ctx,password_cb);
    if(!(SSL_CTX_use_PrivateKey_file(ctx,keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Couldn't read key file");

    /* Load the CAs we trust*/
    if(!(SSL_CTX_load_verify_locations(ctx,CA_LIST,0)))
      berr_exit("Couldn't read CA list");
    if (!(SSL_CTX_check_private_key(ctx)))
      berr_exit("Private key does match the public certificate");
     
    SSL_CTX_set_verify_depth(ctx,1);
    SSL_CTX_set_verify(ctx,
      SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);

    /* Load randomness */
    if(!(RAND_load_file(RANDOM,1024*1024)))
      berr_exit("Couldn't load randomness");

    return ctx;
  }

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


