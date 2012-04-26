#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include "ex_socket.h"
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>



struct socket_list *gHead=NULL;

int main(int argc, char *argv[]) 
{ 

  int sockfd,socket_client; 
	struct sockaddr_in server_addr; 
	struct sockaddr_in client_addr; 
	SSL_CTX *ctx;

	char log_num=0 ;
	struct  in_addr *ip_address;
	int sin_size,portnumber;  
	struct socket_list *p1;
	pid_t pid;
	 //struct  in_addr  all_addr[20];

	
  
	if(argc!=2) 
	{ 
		fprintf(stderr,"Usage:%s portnumber\a\n",argv[0]); 
		exit(1); 
	} 
	
	if((portnumber=atoi(argv[1]))<0) 
	{ 
		fprintf(stderr,"Usage:%s portnumber\a\n",argv[0]); 
		exit(1); 
	} 
	
		 /* */
    SSL_library_init();
    /* */
    OpenSSL_add_all_algorithms();
    /* */ 

    SSL_load_error_strings();
    /* SSL V2  SSL Content Text */
    ctx = SSL_CTX_new(SSLv23_server_method());
    /* */
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* */
    if (SSL_CTX_use_certificate_file(ctx, "cacert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* */
    if (SSL_CTX_use_PrivateKey_file(ctx, "privkey.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* */
    if (!SSL_CTX_check_private_key(ctx)) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    

  /* */
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) 
	{ 
		fprintf(stderr,"Socket error:%s\n\a",strerror(errno)); 
		exit(1); 
	} 
	

	
	bzero(&server_addr,sizeof(struct sockaddr_in)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY); 
	server_addr.sin_port=htons(portnumber); 
	 
	if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1) 
	{ 
		fprintf(stderr,"Bind error:%s\n\a",strerror(errno)); 
		exit(1); 
	} 
	
	make_socket_nonblocking(sockfd); 
	
	if(listen(sockfd,5)==-1) 
	{ 
		fprintf(stderr,"Listen error:%s\n\a",strerror(errno)); 
        	exit(1); 
	} 
  
  
 
  
	while(1) 
	{ 	
		
	  //	login_socket[log_num].socket_s=0;
		
	  sin_size=sizeof(struct sockaddr_in); 
	  SSL *ssl;
		  //printf("%d\n",login_socket[log_num].socket_s);
		if((socket_client = accept(sockfd,(struct sockaddr *)(&client_addr),&sin_size))>0) 
		{ 
		 
			make_socket_nonblocking(socket_client);
			
			        /* SSL */
                        ssl = SSL_new(ctx);
              /*SSL */
	     
                 SSL_set_fd(ssl, socket_client);
		
              /**/
                  printf("noblocking sucess !!!\n");
		  while (SSL_accept(ssl) == -1)
     /* {  
          perror("accept");
          close(socket_client);
          break;
      }*/
     // printf("ssl sucess !!!\n");
      
      inster_list( gHead, socket_client, client_addr.sin_addr); 
      p1=(struct socket_list*)malloc(sizeof(struct socket_list));
			p1->socket_s=socket_client;
      p1->ssl = ssl;
	    p1->if_login = 1;
	  //
	    p1-> rec_addr = client_addr.sin_addr;
		  p1->next=gHead;	
			//
			p1->next=gHead;			
			gHead=p1;
			
		  fprintf(stdout,"Server get connection from %s\n",inet_ntoa(client_addr.sin_addr)); 
				log_num++; 		
				      
		 
        
		}		       
	 
	   printf("%d\n",log_num);
	
	     p1=gHead;
	      
		   while(p1)	        
		   { 
		   	// printf("the socket id: %d\n",gHead->socket_s); 
                         socket_service( log_num, p1, gHead);      
                         p1=p1->next;
         
                   }	 
                   sleep(1);
		
   }
   
 /*SSL */
  //SSL_shutdown(ssl);
        /* */
  //SSL_free(ssl);
        /* socket */      
	close(sockfd); 
	exit(0); 
} 
