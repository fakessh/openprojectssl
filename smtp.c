#include <stdio.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#ifdef WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#endif

#define   CHK_NULL(x)   if   ((x)==NULL)   exit   (1)
#define   CHK_ERR(err,s)   if   ((err)==-1)   {   perror(s);   exit(1);   }
#define   CHK_SSL(err)   if   ((err)==-1)   {   ERR_print_errors_fp(stderr);   exit(2);   }

struct data6
{
	unsigned int d4:6;
	unsigned int d3:6;
	unsigned int d2:6;
	unsigned int d1:6;
};


char con628(char c6);
void base64(char *dbuf,char *buf128, int len); 
void send_line(SSL* ssl,char* cmd);
void recv_line(SSL* ssl);
void sendemail(char *email,char *body);
int open_socket(struct sockaddr *addr);

int main()
{
	char email[] = "fakessh@fakessh.eu";
	char body[] = "From: \"www\"<fakessh@fakessh.eu>\r\n"
		"To: \"w111\"<john.swilting@wanadoo.fr>\r\n"
	"Subject: Hello\r\n\r\n"
	"Hello World, Hello Email!";
	sendemail(email, body);
	return 0;
}

char con628(char c6)
{
	char rtn = '\0';
	if (c6 < 26) rtn = c6 + 65;
	else if (c6 < 52) rtn = c6 + 71;
	else if (c6 < 62) rtn = c6 - 4;
	else if (c6 == 62) rtn = 43;
	else rtn = 47;
	return rtn;
}

//realizing base64
void base64(char *dbuf,char *buf128, int len)
{
	struct data6 *ddd = NULL;
	int i = 0;
	char buf[256] = {0};
        char *tmp = NULL;
	char cc = '\0';
	memset(buf, 0, 256);
	strcpy(buf, buf128);
	for(i = 1; i <= len/3; i++)
	{
		tmp = buf+(i-1)*3;
		cc = tmp[2];
		tmp[2] = tmp[0];
		tmp[0] = cc;
		ddd = (struct data6 *)tmp;
		dbuf[(i-1)*4+0] = con628((unsigned int)ddd->d1);
		dbuf[(i-1)*4+1] = con628((unsigned int)ddd->d2);
		dbuf[(i-1)*4+2] = con628((unsigned int)ddd->d3);
		dbuf[(i-1)*4+3] = con628((unsigned int)ddd->d4);
	}
	if(len%3 == 1)
	{
		tmp = buf+(i-1)*3;
		cc = tmp[2];
		tmp[2] = tmp[0];
		tmp[0] = cc;
		ddd = (struct data6 *)tmp;
		dbuf[(i-1)*4+0] = con628((unsigned int)ddd->d1);
		dbuf[(i-1)*4+1] = con628((unsigned int)ddd->d2);
		dbuf[(i-1)*4+2] = '=';
		dbuf[(i-1)*4+3] = '=';
	}
	if(len%3 == 2)
	{
		tmp = buf+(i-1)*3;
		cc = tmp[2];
		tmp[2] = tmp[0];
		tmp[0] = cc;
		ddd = (struct data6 *)tmp;
		dbuf[(i-1)*4+0] = con628((unsigned int)ddd->d1);
		dbuf[(i-1)*4+1] = con628((unsigned int)ddd->d2);
		dbuf[(i-1)*4+2] = con628((unsigned int)ddd->d3);
		dbuf[(i-1)*4+3] = '=';
	}
	return;
}

//send data
void send_line(SSL* ssl,char* cmd)
{
    	int err;
    	err = SSL_write (ssl, cmd, strlen(cmd));
    	CHK_SSL(err);
}

//receive data
void recv_line(SSL* ssl)
{
    	char rbuf[1500] = {0};
    	int err;
    	err = SSL_read (ssl, rbuf, sizeof(rbuf) - 1);
    	CHK_SSL(err);
    	printf("%s\n", rbuf); 
}

//open TCP Socket connection
int open_socket(struct sockaddr *addr)
{
	int sockfd = 0;
	int retval;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		fprintf(stderr, "Open sockfd(TCP) error!\n");
		exit(-1);
	}
	retval = connect(sockfd, addr, sizeof(struct sockaddr));
	printf("connecting smtp server\n");
	if(retval == -1)
	{
		fprintf(stderr, "Connect sockfd(TCP) error!\n");
		exit(-1);
	}
	return sockfd;
}

// sending email
void sendemail(char *email, char *body)
{
	int sockfd;
	int retval = 0;
	int err;
	char *host_name = "smtp.fakessh.eu";
	struct sockaddr_in their_addr;
	struct hostent *hent;
	char buf[1500] = {0};
	char rbuf[1500] = {0};
	char login[128] = {0};
	char pass[128] = {0};

	//initialize SSL
	SSL_CTX *ctx;
	SSL *ssl;
	SSL_METHOD *meth;
	
	SSLeay_add_ssl_algorithms();
	meth = SSLv23_method();
	SSL_load_error_strings();
   	SSL_library_init();
   	ctx = SSL_CTX_new(meth);
   	CHK_NULL(ctx);
	
	fd_set readfds;
	struct timeval timeout;

        //Define a timeout for resending data.
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	#ifdef WIN32
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	#endif
	
	hent = gethostbyname(host_name);
	memset(&their_addr, 0, sizeof(their_addr));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(587);
	their_addr.sin_addr = *((struct in_addr *)hent->h_addr);

	//connecting mail server and reconnecting if no response in 2 seconds
	sockfd = open_socket((struct sockaddr *)&their_addr);	
	memset(rbuf,0,1500);
	FD_ZERO(&readfds); 
     	FD_SET(sockfd, &readfds);
      	retval = select(sockfd+1, &readfds, NULL, NULL, &timeout);
	while(retval <= 0)
	{
		printf("reconnect...\n");
		sleep(2);
		close(sockfd);
		sockfd = open_socket((struct sockaddr *)&their_addr);
		memset(rbuf,0,1500);
		FD_ZERO(&readfds); 
     		FD_SET(sockfd, &readfds);
      		retval = select(sockfd+1, &readfds, NULL, NULL, &timeout);
	}

	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	printf("%s\n", rbuf);

	//EHLO
	memset(buf, 0, 1500);
	sprintf(buf, "EHLO localhost\r\n");
	send(sockfd, buf, strlen(buf), 0);
	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	printf("%s\n", rbuf);
	
	//START_TLS with OPENSSL 
    	memset(buf,0, 1500);
	sprintf(buf, "STARTTLS\r\n");
	send(sockfd, buf, strlen(buf), 0);
	memset(rbuf, 0, 1500);
	recv(sockfd, rbuf, 1500, 0);
	printf("%s\n", rbuf);
	
	
	//AUTH LOGIN
	ssl   =   SSL_new(ctx);                                                   
	CHK_NULL(ssl); 
    	SSL_set_fd   (ssl,   sockfd);
   	err   =   SSL_connect(ssl);                                           
	CHK_SSL(err);
   
        memset(buf,0, 1500);
        sprintf(buf, "EHLO localhost\r\n");
        send_line(ssl,buf);
        recv_line(ssl); 
   
	
        memset(buf,0, 1500);
        sprintf(buf, "AUTH LOGIN\r\n");
        send_line(ssl,buf);
    	recv_line(ssl); 	

	//USER
	memset(buf, 0, 1500);
	sprintf(buf,"fakessh");
	memset(login, 0, 128);
	base64(login, buf, strlen(buf));
	sprintf(buf, "%s\r\n", login);
	send_line(ssl,buf);
    	recv_line(ssl); 

	//PASSWORD
	memset(buf, 0, 1500);
	sprintf(buf, "----");
	memset(pass, 0, 128);
	base64(pass, buf, strlen(buf));
	sprintf(buf, "%s\r\n", pass);
	send_line(ssl,buf);
    	recv_line(ssl);

	//MAIL FROM
        memset(buf,0, 1500);
        sprintf(buf, "MAIL FROM:<fakessh@fakessh.eu>\r\n");
	send_line(ssl,buf);
    	recv_line(ssl);

	//RCPT TO first receiver
	memset(buf, 0, 1500);
	sprintf(buf, "RCPT TO:<john.swilting@wanadoo.fr>\r\n");
	send_line(ssl,buf);
    	recv_line(ssl);

	//RCPT TO second receiver and more receivers can be added
        //memset(buf, 0, 1500);
        //sprintf(buf, "RCPT TO:<john.swilting@wanadoo.fr>\r\n");
	//send_line(ssl,buf);
    	//recv_line(ssl);

	//DATA ready to send mail content
	send_line(ssl,"DATA\r\n");
    	recv_line(ssl);

	//send mail content£¬"\r\n.\r\n" is the end mark of content
	memset(buf, 0, 1500);
	sprintf(buf, "%s\r\n.\r\n", body);
	send_line(ssl,buf);
    	recv_line(ssl);
	printf("mail send!\n");

	//QUIT
	send_line(ssl,"QUIT\r\n");
    	recv_line(ssl);
   
	//free SSL and close socket
	SSL_shutdown (ssl);
	close(sockfd); 
    	SSL_free (ssl);
    	SSL_CTX_free (ctx); 

	#ifdef WIN32
	WSACleanup();
	#endif

	return;
}




