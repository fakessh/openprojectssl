#include "common.h"

void echo(SSL *ssl)
  {
    char buf[BUFSIZZ];
    int r,len,offset;
    
    while(1){
      /* First read data */
      r=SSL_read(ssl,buf,BUFSIZZ);
      write(1,buf,r);
      switch(SSL_get_error(ssl,r)){
        case SSL_ERROR_NONE:
          len=r;
          break;
        case SSL_ERROR_ZERO_RETURN:
          goto end;
        default:
          berr_exit("SSL read problem");
      }

      /* Now keep writing until we've written everything*/
      offset=0;
      
      while(len){
        r=SSL_write(ssl,buf+offset,len);
        switch(SSL_get_error(ssl,r)){
          case SSL_ERROR_NONE:
            len-=r;
            offset+=r;
            break;
          default:
            berr_exit("SSL write problem");
        }
      }
    }
  end:
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }      
    
