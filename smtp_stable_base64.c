#include <stdio.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#ifdef W32_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
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
#include <linux/types.h>
#include <assert.h>
#endif
 
#define   CHK_NULL_ASSERT(x)   assert(x != NULL) 

#define   CHK_NULL(x)   do { if   ((x)==NULL)   exit   (1); }while(0) 
#define   CHK_ERR(err,s)   if   ((err)==-1)   {   perror(s);   exit(1);   }
#define   CHK_SSL(err)   if   ((err)==-1)   {   ERR_print_errors_fp(stderr);   exit(2);   }

#define TAILLE_TAMPON    1500
 
void encodeblock( unsigned char in[], char b64str[], int len );
void b64_encode(char *clrstr, char *b64dst);
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
/* ------------------------------------------------------------------------ *
 * file:        base64_stringencode.c v1.0                                  *
 * purpose:     tests encoding/decoding strings with base64                 *
 * author:      02/23/2009 Frank4DD                                         *
 *                                                                          *
 * source:      http://base64.sourceforge.net/b64.c for encoding            *
 *              http://en.literateprograms.org/Base64_(C) for decoding      *
 * ------------------------------------------------------------------------ */


/* ---- Base64 Encoding/Decoding Table --- */
char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";





/* encodeblock - encode 3 8-bit binary bytes as 4 '6-bit' characters */
void encodeblock( unsigned char in[], char b64str[], int len ) {
    char out[5];
    out[0] = b64[ in[0] >> 2 ];
    out[1] = b64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? b64[ ((in[1] & 0x0f) << 2) |
             ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? b64[ in[2] & 0x3f ] : '=');
    out[4] = '\0';
    strncat(b64str, out, sizeof(out));
}

/* encode - base64 encode a stream, adding padding if needed */
void b64_encode(char *clrstr, char *b64dst) {
  unsigned char in[3];
  int i, len = 0;
  int j = 0;

  b64dst[0] = '\0';
  while(clrstr[j]) {
    len = 0;
    for(i=0; i<3; i++) {
     in[i] = (unsigned char) clrstr[j];
     if(clrstr[j]) {
        len++; j++;
      }
      else in[i] = 0;
    }
    if( len ) {
      encodeblock( in, b64dst, len );
    }
  }
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
    	char rbuf[TAILLE_TAMPON] = {0};
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
        #ifdef W32_NATIVE
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        #else
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
        #endif
	if(sockfd < 0)
	{
		fprintf(stderr, "Open sockfd(TCP) error!\n");
		exit(-1);
	}
        #ifdef W32_NATIVE
        retval = connect(sockfd, addr, sizeof(struct sockaddr));
        #else
	retval = connect(sockfd, addr, sizeof(struct sockaddr));
        #endif
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
	char buf[TAILLE_TAMPON] = {0};
	char rbuf[TAILLE_TAMPON] = {0};
	char login[TAILLE_TAMPON] = {0};
	char pass[TAILLE_TAMPON] = {0};

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
        #ifdef W32_NATIVE
        their_addr.sin_addr = *((struct in_addr * ) hent->h_addr);
        #else
	their_addr.sin_addr = *((struct in_addr *)hent->h_addr);
        #endif
        memset(&(their_addr.sin_zero), 0, 8); 
	//connecting mail server and reconnecting if no response in 2 seconds
	sockfd = open_socket((struct sockaddr *)&their_addr);	
	memset(rbuf,0,TAILLE_TAMPON);
	FD_ZERO(&readfds); 
     	FD_SET(sockfd, &readfds);
      	retval = select(sockfd+1, &readfds, NULL, NULL, &timeout);
	while(retval <= 0)
	{
		printf("reconnect...\n");
		sleep(2);
		(void)close(sockfd);
	         memset(&(their_addr.sin_zero), 0, 8);
		sockfd = open_socket((struct sockaddr *)&their_addr);
		memset(rbuf,0,TAILLE_TAMPON);
		FD_ZERO(&readfds); 
     		FD_SET(sockfd, &readfds);
      		retval = select(sockfd+1, &readfds, NULL, NULL, &timeout);
	}

	memset(rbuf, 0, TAILLE_TAMPON);
	recv(sockfd, rbuf, TAILLE_TAMPON, 0);
	printf("%s\n", rbuf);

	//EHLO
	memset(buf, 0, TAILLE_TAMPON);
	sprintf(buf, "EHLO localhost\r\n");
	send(sockfd, buf, strlen(buf), 0);
	memset(rbuf, 0, TAILLE_TAMPON);
	recv(sockfd, rbuf, TAILLE_TAMPON, 0);
	printf("%s\n", rbuf);
	
	//START_TLS with OPENSSL 
    	memset(buf,0, TAILLE_TAMPON);
	sprintf(buf, "STARTTLS\r\n");
	send(sockfd, buf, strlen(buf), 0);
	memset(rbuf, 0, TAILLE_TAMPON);
	recv(sockfd, rbuf, TAILLE_TAMPON, 0);
	printf("%s\n", rbuf);
	
	
	//AUTH LOGIN
	ssl   =   SSL_new(ctx);                                                   
	CHK_NULL(ssl);
        CHK_NULL_ASSERT(ssl);
    	SSL_set_fd   (ssl,   sockfd);
   	err   =   SSL_connect(ssl);                                           
	CHK_SSL(err);
   
        memset(buf,0, TAILLE_TAMPON);
        sprintf(buf, "EHLO localhost\r\n");
        send_line(ssl,buf);
        recv_line(ssl); 
   
	
        memset(buf,0, TAILLE_TAMPON);
        sprintf(buf, "AUTH LOGIN\r\n");
        send_line(ssl,buf);
    	recv_line(ssl); 	

	//USER
	memset(buf, 0, TAILLE_TAMPON);
	sprintf(buf,"fakessh");
	memset(login, 0, TAILLE_TAMPON);
        b64_encode(buf, login);
	sprintf(buf, "%s\r\n", login);
	send_line(ssl,buf);
    	recv_line(ssl); 

	//PASSWORD
	memset(buf, 0, TAILLE_TAMPON);
	sprintf(buf,"-----");
	memset(pass, 0, TAILLE_TAMPON);
        b64_encode(buf, pass);
	sprintf(buf, "%s\r\n", pass);
	send_line(ssl,buf);
    	recv_line(ssl);

	//MAIL FROM
        memset(buf,0, TAILLE_TAMPON);
        sprintf(buf, "MAIL FROM:<fakessh@fakessh.eu>\r\n");
	send_line(ssl,buf);
    	recv_line(ssl);

	//RCPT TO first receiver
	memset(buf, 0, TAILLE_TAMPON);
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
	memset(buf, 0, TAILLE_TAMPON);
	sprintf(buf, "%s\r\n.\r\n", body);
	send_line(ssl,buf);
    	recv_line(ssl);
	printf("mail send!\n");

	//QUIT
	send_line(ssl,"QUIT\r\n");
    	recv_line(ssl);
   
	//free SSL and close socket
	SSL_shutdown (ssl);
        #ifdef W32_NATIVE
        (void)closesocket(sockfd);
        #else
	(void)close(sockfd);
        #endif 
    	SSL_free (ssl);
    	SSL_CTX_free (ctx); 

	#ifdef WIN32
	WSACleanup();
	#endif

	return;
}




