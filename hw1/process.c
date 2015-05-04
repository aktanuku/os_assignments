#include "process.h"
#include "shell.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
/**
 * Executes the process p.
 * If the shell is in interactive mode and the process is a foreground process,
 * then p should take control of the terminal.
 */
void launch_process(process *p)
{
	pid_t wpid, pid;

	pid = fork();
	
	if(pid == 0){
		printf("this is what is being executed %s\n", (*p).argv[0]);
		(*p).pid = getpid();
		printf("process id in alternate reality %d\n", (*p).pid);
		/* Put process in its own process group */
   		 if(setpgid((*p).pid,(*p).pid ) < 0){
      			perror("Couldn't put the process in its own process group");
      			exit(1);
    		}	
		
		/* setting signals to default*/
		signal (SIGINT, SIG_DFL);
     		signal (SIGQUIT, SIG_DFL);
     		signal (SIGTSTP, SIG_DFL);
      		signal (SIGTTIN, SIG_DFL);
      		signal (SIGTTOU, SIG_DFL);
      		signal (SIGCHLD, SIG_DFL);

		/*set up redirects*/
		if( (*p).stdin == -1 ){
			printf("no way\n");
			perror("lsh");
		}
		else if((*p).stdin != 0){
			dup2((*p).stdin, 0);
			close((*p).stdin);
		}
		if( (*p).stdout ==-1){
			printf("what\n");
			perror("lsh");
		}
		else if ((*p).stdout != 1){

			dup2((*p).stdout,1);
			close((*p).stdout);
		}
		if( (*p).stderr == -1){
			printf("woops\n");
			perror("lsh");
		}
		else if( (*p).stderr != 2){
			dup2((*p).stderr, 2);
			close((*p).stderr);
		}

		/*execute command*/
		if(execv((*p).argv[0], &(*p).argv[0])==-1){
			printf("something bad happened\n");
			perror("lsh");
		}	
		exit(EXIT_FAILURE);
	}else if(pid<0){
		printf("something else bad happened\n");
		perror("lsh");
	}else{
		p->pid = pid;
		printf("process id in the current reality %d\n", p->pid);
	//	printf("process group in control %d, process group that we just launched %d\n", tcgetpgrp(shell_terminal), pid);
		//Parent process
		put_process_in_foreground(p, 0);
		printf("process group in control %d, process group that we just launched %d\n", tcgetpgrp(shell_terminal), pid);


		do{
		  wpid = waitpid(WAIT_ANY, &((*p).status), WUNTRACED);
		  printf("hello\n");
		}while(!WIFEXITED((*p).status) && !WIFSIGNALED((*p).status) && !WIFSTOPPED((*p).status));	
		printf("done waiting bro\n");	
		tcsetpgrp(shell_terminal, shell_pgid);
		tcgetattr(shell_terminal, &p->tmodes);
		tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
		printf("process group in control %d, process group that we just launched %d\n", tcgetpgrp(shell_terminal), pid);

	}

}	

/* Put a process in the foreground. This function assumes that the shell
 * is in interactive mode. If the cont argument is true, send the process
 * group a SIGCONT signal to wake it up.
 */
void
put_process_in_foreground (process *p, int cont)
{
  /* give process control of terminal */
  tcsetpgrp(shell_terminal, p->pid);
 printf("%d\n", p->pid); 
  /* Send the job a continue signal, if necessary. */
  if(cont)
    {
	tcsetattr(shell_terminal, TCSADRAIN, &p->tmodes);
	if(kill(- p->pid, SIGCONT) < 0)
		perror("kill (SIGCOUNT)");
    }

}

/* Put a process in the background. If the cont argument is true, send
 * the process group a SIGCONT signal to wake it up. */
void
put_process_in_background (process *p, int cont)
{
  
}
