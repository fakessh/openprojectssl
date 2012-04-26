#include "ex_socket.h"
#include <fcntl.h>


#define FAILE_LOGIN 0;
#define NEVER_LOGIN 1;
#define LOGIN    2;


void inster_list(struct socket_list *gHead, int socket, struct in_addr ip_address )
{
		struct socket_list *p1;
	  p1=(struct socket_list*)malloc(sizeof(struct socket_list));
	  p1->socket_s=socket;
	  p1->if_login = 1;
	  //
	  p1-> rec_addr = ip_address; 
		p1->next=gHead;			
	  gHead=p1;

}

void  msg_send(SSL *ssl, char *msg)
{
	  int nbytes;   	 

		nbytes = SSL_write(ssl, msg, strlen(msg));

    if ( nbytes <= 0 )
    {
       fprintf(stderr,"send Error:%s\n",strerror(errno)); 
    } 

}	  
                             
int msg_recv(SSL *ssl, char *msg)
{
    int nbytes;
   
	   //		printf("recv :%d",nbytes);	   exit(0);
    nbytes = SSL_read(ssl, msg, 1024);
 
		if(nbytes == -1)//
		{
			 return 0;
		}
		else        //
		{
			*(msg+nbytes)='\0';
			return nbytes;
	  } 	
	  
}

 //
void error_test() 
{
		 switch (errno) 
	   {
	      case EAGAIN:
	                    /* do nothing, no data to be read. */
	                   break;
	      case EPIPE:     // connection broke
	      case ENOTCONN:      // shut down
	      case ECONNRESET:     
	                   break;
	      default:
	           fprintf(stderr,"problems reading from socket (%d)\n", errno);
	                   break;
	   }
}


/*
int find( struct socket_list , char *ip_buffer)
{
	 int i;
	 for(i=0; i<10; i++)
	 {
	   if(login_socket->rec_addr.s_addr == inet_addr(ip_buffer))
	   {
	      return  login_socket->socket_s;
	   }
     else 
        login_socket++;
	}
	 return 0;
	   
}*/


//
int find_by_name ( struct socket_list *head, char *name)
{
	 int i;
	 struct socket_list *p;
	 p = head; 
	 while(p)
	 {
	   if(strcmp(p->user_name, name) == 0)
	   {
	      return  p->ssl;
	   }
     p = p->next;
	}
	 return 0;
	   
}

//
void make_socket_nonblocking(int skt)
{
    int flag;
    flag = fcntl(skt, F_GETFL, 0);
    fcntl(skt, F_SETFL, flag | O_NONBLOCK);
}


//socket客户端服务程序
void socket_service( char log_num, struct socket_list  *login_socket, struct socket_list *head)
{   
	  char num,i;
	  char buffer[1024]; 
	  int sockfd;
	  SSL *ssl;
	  char str[20];
	  struct socket_list *p; 
   // printf("login_socket ->socket_s is: %d \n",  login_socket ->socket_s );
	     
	  if( login_socket->if_login== 1)
	  {
	  	 printf("login!!!\n");
	  	 msg_send(login_socket ->ssl , "FreeChat login:");
	  	 login_socket ->if_login = FAILE_LOGIN;
	  }
	  if( login_socket ->if_login == 0)
	  {
	  	  if( msg_recv( login_socket ->ssl, buffer))
	  	  {
	  	  	strcpy( login_socket ->user_name,buffer);
	  	  	login_socket ->if_login = LOGIN;
	  	  	 msg_send( login_socket ->ssl , "Login Success!!");
	  	  }
	  	   
	  }
    
	       
    if( login_socket ->if_login == 2 && msg_recv( login_socket ->ssl, buffer)) 
		{
			   printf("I have received:%s\n",buffer);
				 if(strcmp(buffer,"who")==0) 
		     {
		     	     
		  	       msg_send( login_socket ->ssl, "the login user is:");
		  	       p = head;
		  	       while(p)
		  	        {
		  	     	   //*ip_address = login_socket[num].rec_addr;
		  	     	   
		  	     	   strcpy(buffer," IP address is: ");
		  	     	   strcat(buffer, inet_ntoa( p ->rec_addr));
                 strcat(buffer,", User name is: ");
	  	  	       strcat(buffer, p ->user_name);
	  	  	       msg_send( login_socket ->ssl,buffer);
	  	  	       p = p->next;
		  	        }//msg_send(&log_num);
		      }
		      else
		      {
		       sscanf(buffer,"%[^ ]",str); 
         //  find_by_name ( struct socket_list *login_socket, char *name)
		  	  // printf("  %s",str); 	 
		  	    ssl = find_by_name(head, str);    //
		  	    if(ssl == 0)                         //
                msg_send( login_socket ->ssl,"Can no find the IP address!");
              else 
             {
                    //msg_send((login_socket + courrent_num)->socket_s,"Connect success!!"); 
                    sscanf(buffer,"%*s%s",buffer);  
                    bzero(str,20);
                    strcpy(str,login_socket ->user_name);
                    strcat(str," says:");
                    msg_send(ssl ,str);    //printf ** says:
		    i=0;
		    while(buffer[i]!=' ')
		       i++;

		       i++;
                    msg_send(ssl , buffer);
                     
                          
 	         	 }  
                
               
         } 
    }

	   
}
	 
	 
 
