#include "process.h"
#include "shell.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
//#include<libexplain/waitpid.h>
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
		
		(*p).pid = getpid();
		
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
		
		//Parent process
		if (p->background == 1){
			put_process_in_background(p, 0);
		}
		else{
			put_process_in_foreground(p,0);
		}
		
	}

}	

int mark_process_status(pid_t pid, int status){
	process *p;
	
	if(pid>0){
		for(p = first_process; p; p = p->next){
			if(p->pid == pid){
				p->status = status;
				if(WIFSTOPPED(status))
					p->stopped = 1;
				else
					{
					p->completed = 1;
					if(WIFSIGNALED(status)){
					fprintf(stderr, "%d: Terminated by signal %d.\n",
						(int) pid, WTERMSIG(p->status));
					}
				return 0;
				}
			}
		}
		fprintf(stderr, "No Child process %d\n", pid);
		return -1;
	}
	else if (pid == 0 ||errno == ECHILD)
		return -1;
	else{
		perror("waitpid");
		return -1;
	}
}

void wait_for_process(process *p){
	int status;
	pid_t pid;

	do{
	    pid = waitpid(-1, &status, WUNTRACED);
	if(pid<0){
		if(errno == ECHILD){
			printf("echild error\n");
		}
		else if(errno == EINTR){
			printf("ERROR EINTR\n");
		}
		else{
			printf("error einval\n");
			}
	}	
	}while(!mark_process_status(pid,status) && !WIFEXITED((*p).status) && !WIFSIGNALED((*p).status) && !WIFSTOPPED((*p).status));
}


/* Put a process in the foreground. This function assumes that the shell
 * is in interactive mode. If the cont argument is true, send the process
 * group a SIGCONT signal to wake it up.
 */
void
put_process_in_foreground (process *p, int cont)
{

  pid_t wpid;

  /* give process control of terminal */
  tcsetpgrp(shell_terminal, p->pid);
 

  /* Send the job a continue signal, if necessary. */
  if(cont)
    {
	tcsetattr(shell_terminal, TCSADRAIN, &p->tmodes);
	if(kill(- p->pid, SIGCONT) < 0)
		perror("kill (SIGCOUNT)");
    }


	/* wait for child process to be signaled, stopped of completed*/
	wait_for_process(p);
	/*give terminal control back to shell */
	tcsetpgrp(shell_terminal, shell_pgid);
	tcgetattr(shell_terminal, &p->tmodes);
	tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
	


}

/* Put a process in the background. If the cont argument is true, send
 * the process group a SIGCONT signal to wake it up. */
void
put_process_in_background (process *p, int cont)
{
  if(cont)
	if(kill(-p->pid, SIGCONT) < 0)
		perror("kill (SIGCONT)");

}
