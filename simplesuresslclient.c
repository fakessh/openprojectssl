#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void ShowCerts(SSL * ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        printf("__:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("__: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("__: %s\n", line);
        free(line);
        X509_free(cert);
    } else
        printf("__\n");
}

/**********************************************/
void make_socket_nonblocking(int sockfd)
{
    int flag;
    flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
}

int main(int argc, char *argv[]) 
{ 
	int sockfd; 
	char buffer[1024]; 
	struct sockaddr_in server_addr; 
	struct hostent *host; 
	int portnumber,nbytes; 
	pid_t pid;
  SSL_CTX *ctx;
  SSL *ssl;
	if(argc!=3) 
	{ 
		fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]); 
		exit(1); 
	} 
	if((host=gethostbyname(argv[1]))==NULL) 
	{ 
		fprintf(stderr,"Gethostname error\n"); 
		exit(1); 
	} 
	if((portnumber=atoi(argv[2]))<0) 
	{ 
		fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]); 
		exit(1); 
	}
   
   
    /* */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }



	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) 
	{ 
		fprintf(stderr,"Socket Error:%s\a\n",strerror(errno)); 
		exit(1); 
	} 
        make_socket_nonblocking(sockfd);
  
  
	bzero(&server_addr,sizeof(server_addr)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_port=htons(portnumber); 
	server_addr.sin_addr=*((struct in_addr *)host->h_addr); 
	
	
	if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1) 
	{ 
		fprintf(stderr,"Connect Error:%s\a\n",strerror(errno)); 
		exit(1); 
	} 
	printf("connected!\n");
	
	 /*  */
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    /*  */
    if (SSL_connect(ssl) == -1)
        ERR_print_errors_fp(stderr);
    else {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);
	printf("\n");
    }
    
    
	////////////////////////////////
	if((pid=fork())==-1)
		{ 
			fprintf(stderr,"Fork error:%s\n\a",strerror(errno)); 
			exit(1); 
		}
		if(pid==0)
		{
		  	while(1)
		  	{
		  		  nbytes=SSL_read(ssl, buffer, 1024) ;
            if (nbytes > 0)
            {	 
            	   buffer[nbytes]='\0'; 
		             printf("%s\n",buffer);
	        	     
            }
             
        }
		}
	/////////////////////////////////////////	
	while(1)
	{  	
	  if((nbytes=read(0,buffer,1024))==-1) 
		{ 
	      
			fprintf(stderr,"Read Error:%s\n",strerror(errno)); 
			exit(1); 
		}
		if(nbytes > 1 )
		   buffer[nbytes-1]='\0'; 
		
	//	scanf("%s",buffer); 
	  if(strcmp ("exit",buffer)== 0 )
	  {
	  	 printf( "Chat logout!\n" ); 
	  	 SSL_shutdown(ssl);
       SSL_free(ssl);
       SSL_CTX_free(ctx);
	  	 close(sockfd);
	  	 exit(0);
	  }
	  
		nbytes=SSL_write(ssl, buffer, strlen(buffer));
   /* if (nbytes < 0)
        printf("'%s'·'%s'\n",buffer, errno, strerror(errno));
    else
        printf("", buffer, nbytes);*/
		
	}
	close(sockfd); 
	exit(0); 
} 
