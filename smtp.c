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
#include <strings.h>
#endif

#define   CHK_NULL(x)   if   ((x)==NULL)   exit   (1)
#define   CHK_ERR(err,s)   if   ((err)==-1)   {   perror(s);   exit(1);   }
#define   CHK_SSL(err)   if   ((err)==-1)   {   ERR_print_errors_fp(stderr);   exit(2);   }


const int CHARS_PER_LINE = 72;
void send_line(SSL* ssl,char* cmd);
void recv_line(SSL* ssl);
void sendemail(char *email,char *body);
int open_socket(struct sockaddr *addr);
typedef enum
{
	step_A, step_B, step_C
} base64_encodestep;

typedef struct
{
	base64_encodestep step;
	char result;
	int stepcount;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate* state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in);

int base64_encode_blockend(char* code_out, base64_encodestate* state_in);
void base64_init_encodestate(base64_encodestate* state_in)
{
	state_in->step = step_A;
	state_in->result = 0;
	state_in->stepcount = 0;
}

char base64_encode_value(char value_in)
{
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63) return '=';
	return encoding[(int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
	const char* plainchar = plaintext_in;
	const char* const plaintextend = plaintext_in + length_in;
	char* codechar = code_out;
	char result;
	char fragment;
	
	result = state_in->result;
	
	switch (state_in->step)
	{
		while (1)
		{
	case step_A:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_A;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result = (fragment & 0x0fc) >> 2;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x003) << 4;
	case step_B:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_B;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0f0) >> 4;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x00f) << 2;
	case step_C:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_C;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0c0) >> 6;
			*codechar++ = base64_encode_value(result);
			result  = (fragment & 0x03f) >> 0;
			*codechar++ = base64_encode_value(result);
			
			++(state_in->stepcount);
			if (state_in->stepcount == CHARS_PER_LINE/4)
			{
				*codechar++ = '\n';
				state_in->stepcount = 0;
			}
		}
	}
	/* control should not reach here */
	return codechar - code_out;
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
	char* codechar = code_out;
	
	switch (state_in->step)
	{
	case step_B:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		*codechar++ = '=';
		break;
	case step_C:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		break;
	case step_A:
		break;
	}
	*codechar++ = '\n';
	
	return codechar - code_out;
}
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
	buf = base64_encode_value((int)login);
	sprintf(buf, "%s\r\n", login);
	send_line(ssl,buf);
    	recv_line(ssl); 

	//PASSWORD
	memset(buf, 0, 1500);
	sprintf(buf, "-----");
	memset(pass, 0, 128);
	buf = base64_encode_value((int)pass);
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




