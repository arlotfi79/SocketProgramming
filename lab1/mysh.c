// Concurrent server example: simple shell using fork() and execlp().

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
int k;
char buf[30];
int status;
int len;

  while(1) {

	// print prompt
  	fprintf(stdout,"%d %% ",getpid());

	// read command from stdin
	fgets(buf, 30, stdin);
	len = strlen(buf);
	if(len == 1) 				// case: only return key pressed
	  continue;
	buf[len-1] = '\0';			// case: command entered

  	k = fork();
  	if (k==0) {
  	// child code
    	  if(execlp(buf,buf,NULL) == -1)	// if execution failed, terminate child
	  	exit(1);
  	}
  	else {
  	// parent code
	  waitpid(k, &status, 0);		// block until child process terminates
  	}
  }
}
