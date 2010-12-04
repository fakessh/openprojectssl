#include "common.h"

/* Read from the keyboard and write to the server
   Read from the server and write to the keyboard

*/

extern BIO *SSL_get_rbio();
extern BIO *SSL_get_wbio();
void read_write (SSL *ssl, int sock)
  {

        int r,  c2sl=0;
/*        BIO *SSL_get_rbio(ssl);*/
/*        BIO *SSL_get_wbio(ssl);*/

        /*int shutdown_wait=0;*/
        char c2s[BUFSIZZ], s2c[BUFSIZZ];

        while(1){

            /* Check for input on the console*/
	    BIO *SSL_get_rbio();
            c2sl = SSL_read(0, c2s, BUFSIZZ);
            if(c2sl==0)  goto end;

            /* If we've got data to write then try to write it*/
	  BIO *SSL_get_wbio();
          SSL_write(ssl, c2s, c2sl);

           /* Now check if there's data to read */
           do {
            r = SSL_read(ssl, s2c, BUFSIZZ);

              switch(SSL_get_error(ssl,r)){

                    case SSL_ERROR_NONE:
                      fwrite(s2c,1,r,stdout);
                           break;
                    case SSL_ERROR_ZERO_RETURN:
                      /* End of data */
                           goto end;
                           break;
                    default:
                           berr_exit("SSL read problem");
           }

            } while (SSL_pending(ssl));

          }/* end of while  (1) */
      end:
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return;

 }