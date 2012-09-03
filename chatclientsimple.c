#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sys/time.h>

#define MAX_BUF_SIZE 1024
#define		LINELEN			128
#define		DEFAULT_HOST	"127.0.0.1"	
#define		DEFAULT_PORT	8000

int main(int argc, char *argv[])
{
/*-----------------------------------------------------------------------
	char		*host = DEFAULT_HOST;
	int			port = DEFAULT_PORT;

	switch(argc) {
		case 1:
			host = DEFAULT_HOST;
			break;
		case 3:
			port = atoi(argv[2]);
		case 2:
			host = argv[1];
			break;
		default:
			fprintf(stderr,"usage: tcp_echo [host [port]]\n");
			exit(1);
	}
-----------------------------------------------------------------------
*/
    int sockfd;
    int sin_size;
    struct sockaddr_in dest;
    char buf[MAX_BUF_SIZE + 1];
    fd_set rfds;
    struct timeval tv;
    int retval, maxfd = -1;
	
    if (argc != 3)
    {
        printf("usage: %s ip port\n for example: %s 127.0.0.1 8000\n", argv[0], argv[0]);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "socket error!\n");
        exit(1);
    }
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], (struct in_addr *)&dest.sin_addr.s_addr) == 0)
    {
        fprintf(stderr, "%s error\n", argv[1]);
        exit(1);
    }

    if ( (connect(sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr))) == -1)
    {
        fprintf(stderr, "connect error!\n");
        exit(1);
    }

    printf("Be ready, can chat!\n");

    FD_ZERO(&rfds);
    maxfd = sockfd;

    while (1)
    {
        FD_CLR(sockfd, &rfds);
        FD_SET(sockfd, &rfds);
        FD_SET(0, &rfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if ((retval = select(maxfd + 1, &rfds, NULL, NULL, &tv)) < 0)
        {
            if (errno == EINTR)
            {
                fprintf(stdout, "closed by signal, continue...\n");
                continue;
            }
            else if (errno == EAGAIN)
			{
                    continue;
            }
            else
            {
                fprintf(stderr, "select error!\n");
                break;
            }
        }
        else if (retval == 0)
        {
            continue;
        }

        if (FD_ISSET(sockfd, &rfds))
        {
            bzero(buf, MAX_BUF_SIZE + 1);
            sin_size = recv(sockfd, buf, MAX_BUF_SIZE, 0);
            if (sin_size > 0)
            {
                printf("%s\n", buf);
            }
            else
            {
                if (sin_size < 0)
                    printf("receive message failure! error code is %d, error message is %s\n", errno, strerror(errno));
            }
        }
       
        if (FD_ISSET(0, &rfds))
        {
            bzero(buf, MAX_BUF_SIZE + 1);
            fgets(buf, MAX_BUF_SIZE, stdin);
            if (!strncasecmp(buf, "quit", 4))
            {
                printf("quit chat!\n");
                break;
            }
            sin_size = send(sockfd, buf, strlen(buf) - 1, 0);
            if (sin_size < 0)
            {
                printf("message send failure: [%s], error code is %d, error message is %s\n",
                       buf, errno, strerror(errno));
                break;
            }
            else
            {
//                printf("successfully, total %d bytes\n",sin_size);
            }
        }
    }
    close(sockfd);
    return 0;
}
