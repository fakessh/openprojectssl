//===============================================================================================
//
//	Component		:	Communication Module
//  File		    :	sslClient.c 
//  Creation date  	:   25-Nov-2011 
//  Description		:	It contains SSL communication methods to send and receive the data from
//						Host.
//			
//===============================================================================================

/**************************************** System Includes ***************************************/
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
/*************************************************************************************************/

/****************************************** Local  Includes **************************************/
#include "common.h"
#include "sslClient.h"
#include "svc.h"
#include "errno.h"
#include "Logs.h"
/*************************************************************************************************/

/****************************************** Global Declarations **********************************/
SSLData SSLRequestData;
int AppDataIdx;
static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx);
/*************************************************************************************************/


int SSLInit()
{
	/* Initializing SSL Library */
	SSL_library_init(); 
	
	return 0;
}


int SSLConnect(short hostName)
{
	int iRetVal=0; 
	 
	memset(&SSLRequestData, 0, sizeof(SSLRequestData));
	
	if(hostName  == HOST_CHECKIN_CHECKOUT)
	{
		strcpy(SSLRequestData.Config.HostIP, "216.113.169.218");
		SSLRequestData.Config.HostPort = 2012;
	}
	else if(hostName  == HOST_CAPTURE_REFUND)
	{
		strcpy(SSLRequestData.Config.HostIP, "216.113.169.218");
		SSLRequestData.Config.HostPort = 2001;
	}
	
	printf("%s - Connecting to Host %s:%d\n", __FUNCTION__, SSLRequestData.Config.HostIP, SSLRequestData.Config.HostPort);
	LOG_PRINTF("%s - Connecting to Host %s:%d\n", __FUNCTION__, SSLRequestData.Config.HostIP, SSLRequestData.Config.HostPort);
	
	/* Build our SSL context*/
	SSLRequestData.ctx=initialize_ctx(KEYFILE, PASSWORD);
	
	/* Connect the TCP socket*/
	SSLRequestData.sock=tcp_connect(hostName);
	if(SSLRequestData.sock==CONNECTION_FAIL)	
	{
		printf("%s - tcp_connect failed\n", __FUNCTION__);
		LOG_PRINTF("%s - tcp_connect failed\n", __FUNCTION__);
		return SOCK_CONN_FAIL;
	}
	
	iRetVal = DoSSLhandshake();
	printf("%s - DoSSLhandshake returned: %d\n", __FUNCTION__, iRetVal);
	
    return iRetVal;
}

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	return SSL_VERIFY_NONE;
}

int tcp_connect(short hostName)
{
    struct hostent *hp;
    struct sockaddr_in addr;
     
    if(!(hp=gethostbyname(SSLRequestData.Config.HostIP)))
	{
		printf("%s - Couldn't resolve host\n", __FUNCTION__);
		LOG_PRINTF("%s - Couldn't resolve host\n", __FUNCTION__);
		berr_exit("Couldn't resolve host\n");
		printf("%s - host is: %s\n", __FUNCTION__, SSLRequestData.Config.HostIP);
		return CONNECTION_FAIL;
    }
    memset(&addr,0,sizeof(addr));
    addr.sin_addr=*(struct in_addr*)hp->h_addr_list[0];
    addr.sin_family=AF_INET;
    addr.sin_port=htons(SSLRequestData.Config.HostPort);

    if((SSLRequestData.sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
    	printf("%s - Couldn't create socket", __FUNCTION__);
		LOG_PRINTF("%s - Couldn't create socket", __FUNCTION__);
    	err_exit("Couldn't create socket");
    	return CONNECTION_FAIL;
    }
    else
    {
    	printf("%s - Connected to %s:%d using Socket %d\n", __FUNCTION__, SSLRequestData.Config.HostIP, SSLRequestData.Config.HostPort, SSLRequestData.sock);
    	LOG_PRINTF("%s - Connected to %s:%d using Socket %d\n", __FUNCTION__, SSLRequestData.Config.HostIP, SSLRequestData.Config.HostPort, SSLRequestData.sock);
    }
    
    printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
    LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
    if(connect(SSLRequestData.sock,(struct sockaddr *)&addr,sizeof(addr))<0)
    { 
    	printf("%s - Couldn't connect socket\n", __FUNCTION__);
    	LOG_PRINTF("%s - Couldn't connect socket\n", __FUNCTION__);
    	err_exit("Couldn't connect socket");
    	return CONNECTION_FAIL;
    }
    
	return SSLRequestData.sock;
}

int DoSSLhandshake()
{	
	int		iRetVal;
	long	result;
	
    /* Create new SSL object for this connection */
	SSLRequestData.ssl = SSL_new( SSLRequestData.ctx );
	if(SSLRequestData.ssl == NULL)
		printf("%s - Create of new SSL object failed\n", __FUNCTION__);
	
	/*result = SSL_get_verify_result(SSLRequestData.ssl);
	switch(result)
	{
		case X509_V_OK : printf("The verification succeeded or no peer certificate was presented.\n"); break;
		default: printf("default\n"); break;
	}
	
	printf("%s - SSL_get_verify_result returned\n", __FUNCTION__);*/
	
	//SSL_set_verify(SSLRequestData.ssl, SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	
	/* regiest the application data into the ssl object */
	printf("%s - Register the application data into the SSL object\n", __FUNCTION__);
	//AppDataIdx = SSL_get_ex_new_index( 0, "Config index", NULL, NULL, NULL );
	//SSL_set_ex_data( SSLRequestData.ssl, AppDataIdx, &SSLRequestData.Config );

    /* Use BIO object instead of raw Socket Descriptor */
	SSLRequestData.sbio = BIO_new_socket( SSLRequestData.sock,BIO_NOCLOSE);
	if(SSLRequestData.sbio == NULL)
		printf("%s - Unable to create BIO object\n", __FUNCTION__);	
	else
		printf("%s - Created BIO object successfully\n", __FUNCTION__);		

	printf("%s - Connects the BIOs for the r/w operation of the SSL\n", __FUNCTION__);
	SSL_set_bio(SSLRequestData.ssl,SSLRequestData.sbio,SSLRequestData.sbio);

    /* Intitiate SSL Handshake */
	printf("%s - Do SSL Handshake...\n", __FUNCTION__);
    iRetVal = SSL_connect(SSLRequestData.ssl);
    if( iRetVal <= 0 ) 
    { 
        char  ErrBuff[ 120 ];
        memset( ErrBuff, 0, 120 );
        ERR_error_string( ERR_get_error(), ErrBuff );
        printf("%s - %s\n", __FUNCTION__, ErrBuff);
        
        return -1; 
    }
    printf("\n Handshake OK. SSL Connection using %s\n", SSL_get_cipher(SSLRequestData.ssl));
    
    return 0; //Handshake Succeeded
}


int read_write(char *RequestBuffer, int RequestLen, char *ResponseBuffer, int *ResponseLen, short hostName)
{
	    int 		i=0, w=0, r=0;
	    int 		wRetVal=-1, rRetVal=-1;
	    int 		s2cl=0, s2cl_1=0, s2cl_2=0, result=0;
	    char 		s2c_1[S2C_BUFSIZE], s2c_2[S2C_BUFSIZE], s2c[S2C_BUFSIZE], s2c5Chars[10];
	    char		checkoutReqHeader[C2S_BUFSIZE], checkoutReqBody[C2S_BUFSIZE];
	    int 		iRetVal=0;
	    short		shLoopCount;
		
	    printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
	    LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
	    
		iRetVal = SSLConnect(hostName);
		
		printf("%s - SSLConnect returns: %d\n", __FUNCTION__, iRetVal);
		printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
		LOG_PRINTF("%s - SSLConnect returns: %d\n", __FUNCTION__, iRetVal);
		LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
		
		if(iRetVal < 0)
		{
			printf("%s - Unable to establish connection..\n", __FUNCTION__);
			return SOCK_CONN_FAIL;
		}
		else
		{
			printf("%s - Connected to Host Successfully\n", __FUNCTION__);
			LOG_PRINTF("%s - Connected to Host Successfully\n", __FUNCTION__);
		}
		
	    memset(s2c, 0, sizeof(s2c));
	    memset(s2c_1, 0, sizeof(s2c_1));
        memset(s2c_2, 0, sizeof(s2c_2));
        memset(checkoutReqHeader, 0, sizeof(checkoutReqHeader));
        memset(checkoutReqBody, 0, sizeof(checkoutReqBody));

		printf("%s - Request Length: %d\n", __FUNCTION__, RequestLen);
		LOG_PRINTF("%s - Request Length: %d\n", __FUNCTION__, RequestLen);

		printf("\nREQUEST:\n");
		printf("----------------------------------------------------------------------\n");
		for(i=0; i< RequestLen; i++)
			printf("%c", RequestBuffer[i]);
		printf("\n----------------------------------------------------------------------\n");
		
		LOG_PRINTF("\nREQUEST:\n");
		LOG_PRINTF("----------------------------------------------------------------------\n");
		for(i=0; i< RequestLen; i++)
			LOG_PRINTF("%c", RequestBuffer[i]);
		LOG_PRINTF("\n----------------------------------------------------------------------\n");

		printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
		printf("%s - SSL_write returns: %d\n", __FUNCTION__, w);
		
		w = SSL_write(SSLRequestData.ssl, RequestBuffer, RequestLen);
		
		LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
		LOG_PRINTF("%s - SSL_write returns: %d\n", __FUNCTION__, w);
		
   		switch(SSL_get_error(SSLRequestData.ssl,w))
	    {      
			case SSL_ERROR_NONE:
		    	if(RequestLen!=w)
			    {
			    	wRetVal = SSL_INCOMPLETE_WRITE;
			    	printf("%s - SSL Error: SSL_INCOMPLETE_WRITE\n", __FUNCTION__);
			    	LOG_PRINTF("%s - SSL Error: SSL_INCOMPLETE_WRITE\n", __FUNCTION__);
			    }
			    else
			    {
			    	wRetVal = SUCCESS;
			    	printf("%s - SSL: SSL Write SUCCESS\n", __FUNCTION__);
			    	LOG_PRINTF("%s - SSL: SSL Write SUCCESS\n", __FUNCTION__);
			    }
				break;
	  		default:	 
	 			wRetVal = SSL_WRITE_ERROR;
	 			break;
	    }
	    
	    if(wRetVal == SUCCESS)
	    {
	   		printf("%s - SSL Write Complete..Waiting for PayPal Reponse..\n", __FUNCTION__);
	   		printf("%s - Calling SSL_read..\n", __FUNCTION__);
	   		LOG_PRINTF("%s - SSL Write Complete..Waiting for PayPal Reponse..\n", __FUNCTION__);
	   		LOG_PRINTF("%s - Calling SSL_read..\n", __FUNCTION__);
			
			/* read the response from server */
	        do
	    	{
	    		r=SSL_read(SSLRequestData.ssl, &s2c_1, MSG_READ_PACKET_SIZE);
	    		
	    		printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
	    		printf("%s - r is %d [%s]" , __FUNCTION__, r, strerror(errno));
	    		LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
	    		LOG_PRINTF("r is %d [%s]" ,__FUNCTION__, r, strerror(errno));
	    		
	    		switch(SSL_get_error(SSLRequestData.ssl,r))
	           	{
	        		case SSL_ERROR_NONE:
									s2cl_1 = r;
									rRetVal = SUCCESS;
									printf("%s - Length of Response Buffer: %d\n", __FUNCTION__, s2cl);
									LOG_PRINTF("%s - Length of Response Buffer: %d\n", __FUNCTION__,s2cl); 
									break;
		        	case SSL_ERROR_ZERO_RETURN:
		        					printf("%s - Error: SSL_ERROR_ZERO_RETURN ", __FUNCTION__);
		        					LOG_PRINTF("%s - Error: SSL_ERROR_ZERO_RETURN ", __FUNCTION__);
	        						break;
		        	case SSL_ERROR_SYSCALL:
				        			printf("%s - Error: SSL_ERROR_SYSCALL", __FUNCTION__);
				        			LOG_PRINTF("%s - Error: SSL_ERROR_SYSCALL", __FUNCTION__);
				        			rRetVal = SSL_ERROR_SYSCALL;
						        	break;
		        	default:
			        				printf("%s - Error: default", __FUNCTION__);
			        				LOG_PRINTF("%s - Error: default", __FUNCTION__);
			        				rRetVal = SSL_READ_ERROR;
			        				break;
		   	   	}
		   	   	svcWait(20);
			}
			while((rRetVal!=SUCCESS));
			
			printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
			printf("Calling second SSL_read loop...............\n");
			LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
			LOG_PRINTF("Calling second SSL_read loop...............\n");
			
			/* read the remaining response from server */
	        do
	    	{
	    		if(hostName == HOST_CHECKIN_CHECKOUT)
	    			r=SSL_read(SSLRequestData.ssl, s2c5Chars, 10);
	    		else
	    			r=SSL_read(SSLRequestData.ssl, &s2c_2, MSG_READ_PACKET_SIZE);
	    		
	    		switch(SSL_get_error(SSLRequestData.ssl,r))
	           	{
	        		case SSL_ERROR_NONE:
									if(r<10)
									{
										printf("%s - 5 Bytes received in Response\n", __FUNCTION__);
										printf("\n.......5 BYTES RECEIVED DATA.......\n");
										for(shLoopCount=0; shLoopCount<r; shLoopCount++)
											printf("%02x ", s2c5Chars[shLoopCount]);
										printf("\n.......5 BYTES RECEIVED DATA.......\n");
										
										LOG_PRINTF("%s - 5 Bytes received in Response\n", __FUNCTION__);
										LOG_PRINTF("\n.......5 BYTES RECEIVED DATA.......\n");
										for(shLoopCount=0; shLoopCount<r; shLoopCount++)
											LOG_PRINTF("%02x ", s2c5Chars[shLoopCount]);
										LOG_PRINTF("\n.......5 BYTES RECEIVED DATA.......\n");
										
									}
									s2cl_2 = r;
									rRetVal = SUCCESS;
									break;
		        	case SSL_ERROR_ZERO_RETURN:
		        					printf("Error: SSL_ERROR_ZERO_RETURN ");
	        						break;
		        	case SSL_ERROR_SYSCALL:
				        			printf("Error: SSL_ERROR_SYSCALL ");
				        			rRetVal = SSL_ERROR_SYSCALL;
						        	break;
		        	default:
			        				printf("Error: default ");
			        				rRetVal = SSL_READ_ERROR;
			        				break;
		   	   	}
		   	   	svcWait(20);
			}
			while((rRetVal!=SUCCESS));
			
			r=0;
			rRetVal=-1;
			
			memcpy(&s2c, s2c_1, s2cl_1);
			memcpy(&s2c[s2cl_1], s2c_2, s2cl_2);
			
			s2cl = s2cl_1 + s2cl_2;
    		memcpy(&ResponseBuffer[0], &s2c, s2cl);
    		*ResponseLen=(s2cl);
    		
			printf("\n##RESPONSE:\n");
    		printf("----------------------------------------------------------------------\n");
    		for(i=0; i<s2cl; i++)
    			printf("%c", s2c[i]);
			printf("\n----------------------------------------------------------------------\n");
    		
			LOG_PRINTF("\n##RESPONSE:\n");
    		LOG_PRINTF("----------------------------------------------------------------------\n");
    		for(i=0; i<s2cl; i++)
    			LOG_PRINTF("%c", s2c[i]);
			LOG_PRINTF("\n----------------------------------------------------------------------\n");
 
			fflush(stdout);
			
			printf("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);
			LOG_PRINTF("%s - SSLRequestData.sock: %d\n", __FUNCTION__, SSLRequestData.sock);

			printf("%s - Closing SSL socket and releasing the allocated SSL memory...\n", __FUNCTION__);
			LOG_PRINTF("%s - Closing SSL socket and releasing the allocated SSL memory...\n", __FUNCTION__);
			SSL_free(SSLRequestData.ssl);
			SSL_CTX_free(SSLRequestData.ctx);
			BIO_free(SSLRequestData.sbio);
			result = close(SSLRequestData.sock);
			printf("%s - close returned: %d\n", __FUNCTION__, result);
			LOG_PRINTF("%s - close returned: %d\n", __FUNCTION__, result);
			SSLRequestData.sock = -1;
	
			return SSL_READ_SUCCESS;
		}
		else
		{
			return SOCK_CONN_FAIL;
		}
}

