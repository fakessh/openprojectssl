#include "common.h"

int tcp_connect()
 {
    struct hostent *hp;
    struct sockaddr_in addr;
    int sock;
    
    if(!(hp=gethostbyname(ServerHOST)))
      berr_exit("Couldn't resolve host");
    bzero(&addr,sizeof(addr));
    addr.sin_addr.s_addr = *(long*)(hp->h_addr);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(ServerPORT);

    if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
      err_exit("Couldn't create socket");
    if(connect(sock,(struct sockaddr *)&addr,sizeof(addr))<0)
      err_exit("Couldn't connect socket");
    
    return sock;
  }
