/* A simple SSL echo server */
#include "common.h"
#include "server.h"
#include "echo.h"
#define SA      struct sockaddr

static int s_server_session_id_context = 1;

int main(int argc,char **argv)
{
   int sock,s;
   BIO *sbio;
   SSL_CTX *ctx;
   SSL *ssl;
   int r;
   struct sockaddr_in from;
   int fromlen;
   struct  hostent *hp, *gethostbyname();
   char *clientHostName;


   /* Build our SSL context*/
   ctx=initialize_ctx(ServerKEYFILE,ServerPASSWORD);
   load_dh_params(ctx,DHFILE);
   generate_eph_rsa_key(ctx);

   SSL_CTX_set_session_id_context(ctx,(void*)&s_server_session_id_context,
       sizeof s_server_session_id_context);

   sock=tcp_listen();

   fromlen = sizeof(from);

   while(1){
      if((s=accept(sock,(SA *)&from,&fromlen))<0)
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
      if (!strcmp(clientHostName, ClientHOST) == 0) {
         printf ("WARNING, clinet coming from non-expected host\n");
         close (s);
      } else {
         printf("client coming  from expected host\n");




         sbio=BIO_new_socket(s,BIO_NOCLOSE);
         ssl=SSL_new(ctx);
         SSL_set_bio(ssl,sbio,sbio);

         if((r=SSL_accept(ssl)<=0))
            berr_exit("SSL accept error");
         check_cert_chain(ssl,ClientHOST);

         if (fork() == 0)
            echo(ssl);
      }
   }
}
