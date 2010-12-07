/* A simple SSL echo server */
#include "common.h"
#include "server.h"
#include "echo.h"
#define SA      struct sockaddr


int main(int argc,char *argv[])
{
   int sock,s;
   BIO *sbio;
   SSL_CTX *ctx;
   SSL *ssl;
   int r;
   struct sockaddr_in from;
   socklen_t fromlen;
   struct  hostent *hp, *gethostbyname();
   char *clientHostName;

   SSL_library_init();
   /* Build our SSL context*/
   ctx=InitServerCTX();
   LoadCertificates(ctx, "mycert.pem", "mycert.pem");
   load_dh_params(ctx,DHFILE);
   generate_eph_rsa_key(ctx);


   sock=tcp_listen();

   fromlen = sizeof(from);

   while(1){
      if((s=accept(sock,(SA *)&from,(socklen_t *)fromlen))<0)
         err_exit("Problem accepting");
      printf("Serving %s:%d\n", inet_ntoa(from.sin_addr),
          ntohs(from.sin_port));
      if ((hp = gethostbyaddr((char *)&from.sin_addr.s_addr,
          sizeof(from.sin_addr.s_addr),AF_INET)) == NULL)
         fprintf(stderr, "Can't find host %s\n", inet_ntoa(from.sin_addr));
      else {
         clientHostName = hp->h_name;
         printf("(Name is : %s)\n", clientHostName);
      }
      ssl = SSL_new(ctx) ;
      SSL_set_fd ( ssl,sock );
      Servlet(ssl);
      if (!strcmp(clientHostName, ClientHOST) == 0) {
         printf ("WARNING, clinet coming from non-expected host\n");
      } else {
         printf("client coming  from expected host\n");


         if (fork() == 0)
            echo(ssl);
      }
      close(sock);
      SSL_CTX_free(ctx);
   }
}
