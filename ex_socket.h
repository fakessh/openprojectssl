#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

 
//recore all IP adderss what are login,it can allow 20 user login
struct socket_list
{	
	
   int socket_s ;
   SSL *ssl;
   char if_login;
   struct  in_addr rec_addr;  //
   struct socket_list *next;
   char user_name[10];  //
}; 

void  msg_send( SSL*, char *msg);

int  msg_recv( SSL*, char *msg);

int find( struct socket_list *login_socket, char *ip_buffer);

void make_socket_nonblocking(int skt);

 
void inster_list(struct socket_list *gHead, int socket, struct in_addr ip_address );

void socket_service( char log_num, struct socket_list  *login_socket, struct socket_list *head);



