#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"


char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

	
int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_help(tok_t arg[]);

int cmd_cd(tok_t arg[]);



/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_cd, "cd", "change directory"},
};

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int cmd_cd(tok_t arg[]) {
 //printf("%s\n", arg[0]);
 if (arg[0])
  return chdir(arg[0]);
else
  return chdir("/home/vagrant");
  
}

int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){
	
    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }

  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
//  signal(SIGCHLD, SIG_IGN); 
  
}

char* resolve_path(char* inputString){

	char path[1000]; 
	strcpy(path, getenv("PATH"));
	tok_t *path_dirs;
	char file_buf[1000];
	char file[1000];



	path_dirs = getToks(path);
	int i = 0;
	FILE* f;
	struct stat st;
	int num_paths=0;
	while(*(path_dirs+i) != NULL){
	//	printf("in resolve path %s\n", *(path_dirs+i));
		strcpy(file_buf,path_dirs[i]);
		sprintf(file, "%s/%s", file_buf, inputString);
//		printf("file actually %s\n",file);	
		if(f=fopen(file,"r")){
			fclose(f);
	//		printf("returning %s\n",file);
			return file;
		}
		else{
			//printf("%s didnt work \n",file);
		}
		i=i+1;
	}
	if(f=fopen(inputString,"r")){
		fclose(f);	
		return inputString;
	}
	else
		return inputString;
}
		 








/**
 * Add a process to our process list
 */
void add_process(process* p)
{
	process* q;

	q = first_process;
	while(q->next != NULL){
		q = q->next;  
	}
//	q->next = malloc(sizeof(process));
	q->next = p;
//	p->prev = malloc(sizeof(process));
	p->prev = q;
	
	q = first_process;

	while(q != NULL){
		q = q->next;
	}

}


void init_process_list(){	
   	first_process = malloc(sizeof(process));
	(*first_process).stdin = 0;
        (*first_process).stdout = 1;
	(*first_process).stderr = 2;
	(*first_process).argv = (char**)malloc(sizeof(char*)*(2)) ;
	(*first_process).argv[0] = (char*)malloc(sizeof(char)*strlen("first"));
	strcpy((*first_process).argv[0],"first");

	(*first_process).argv[1] = NULL;
  	 (*first_process).argc = 1;
   	(*first_process).completed = 0;
   	(*first_process).stopped = 0;
   	(*first_process).background = 0;
	(*first_process).next = malloc(sizeof(process));
 	(*first_process).next = NULL;
   	(*first_process).prev = NULL;
	(*first_process).pid = -1;    

}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{

	process *p;
   	p = malloc(sizeof(process));
	(*p).stdin = 0;
        (*p).stdout = 1;
	(*p).stderr = 2;
	char so[1000];
	char si[1000];
	char se[1000];
	char *po;
	char *pi;
	char *pe;

	char *ptr;
	int i = 0;
 	char local_s[1000];
  
	strcpy(local_s, inputString); 

	if( *(local_s+strlen(local_s)-2) == '&'){
		(*p).background = 1;
		*(local_s+strlen(local_s)-2) = 0x20;
	}
	else{
		(*p).background = 0;
	}
	  
  	ptr = strstr(local_s, "2>");
  	i=2;
	if(ptr){
  		*(ptr) = 0x20;
  		*(ptr+1) = 0x20;
  		while( (*(ptr+i) != '<') && (*(ptr + i) != '>') && (*(ptr + i) != '\0') && (*(ptr + i) != 10)) {
			se[i-2] = *(ptr + i);
        		*(ptr + i) = 0x20;  
        		i = i + 1;
  		}
  		se[i-2] = NULL;
		pe = trimwhitespace(se);
		printf("stderr %s\n", pe);
		(*p).stderr = open(pe, O_WRONLY|O_CREAT, 0666);
	}


  	ptr = strstr(local_s,">");
 	 i =1;
	if(ptr){
  		*ptr = 0x20;
 		 while( (*(ptr+i) != '<') && (*(ptr + i) != '2') && (*(ptr + i) != '\0') && (*(ptr + i) != 10) ){	
			so[i-1] = *(ptr + i);
        		*(ptr + i) = 0x20;  
       			 i = i + 1;
   		}
 		 so[i-1] = NULL;
		po = trimwhitespace(so);
		(*p).stdout = open(po, O_WRONLY|O_CREAT, 0666);

	}  
  
  	ptr = strstr(local_s,"<");
 	 i =1;
	if(ptr){
  		*ptr = 0x20;
  		while( (*(ptr+i) != '>') && (*(ptr + i) != '2') && (*(ptr + i) != '\0') && (*(ptr + i) != 10) ){
			si[i-1] = *(ptr + i);
		      	*(ptr + i) = 0x20;  
        		i = i + 1;
   	}
  	si[i-1] = NULL;
	pi = trimwhitespace(si);
	(*p).stdin = open(pi, O_RDONLY);
	
	}

  tok_t *t = getToks(local_s);
  int arg_count =0;
   i=0;
  while(t[arg_count] != NULL)
	arg_count = arg_count + 1;
      (*p).argv = (char**)malloc(sizeof(char*)*(arg_count+1)) ;
     //resolve path for first arguement
   (*p).argv[0] = (char*)malloc(sizeof(char)*strlen(resolve_path(t[0])));
   strcpy((*p).argv[0],resolve_path(t[0]));
  //copy over rest of arguments
   for(i=1; i<arg_count; i++){

	(*p).argv[i] = (char*)malloc(sizeof(char)*strlen(t[i]));
	strcpy((*p).argv[i],t[i]);
   }  
   (*p).argv[arg_count] = NULL;
   (*p).argc = arg_count;
   (*p).pid = 0;
   (*p).completed = 0;
   (*p).stopped = 0;
  // (*p).background = 0;
   (*p).next = NULL;
   (*p).prev = NULL;    

   return p;
}

void destroy_process(process* p){
	int i = 0;
	for (i = 0; i<(*p).argc; i++){
		free((*p).argv[i]);	
	}
	free((*p).argv);

	free(p);
}

void purge_proc_list(){

	process *p;
	process *p_prev;
	process *p_next;
	for (p = first_process; p; p=p->next){
		if (p->completed){
			p_prev = p->prev;
			p_next = p->next;

			p_prev->next = p_next;
			if(p_next)
				p_next->prev = p_prev;
			destroy_process(p);
		}
	}
}			

int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
  char *s2 = malloc(INPUT_STRING_SIZE+1);
  tok_t *t;			/* tokens parsed from parsed input */
  tok_t *t_input; 			/* tokesn parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  pid_t cpid, tcpid, cpgid;
  char cwd[1000];
  init_process_list();
  init_shell();
  int pipe_num = 0;
  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);
  process *p;
  process *next_p;
  lineNum=0;
  fprintf(stdout, "%d: ", lineNum);
  while ((s = freadln(stdin))){
	
    p = create_process(s);
    add_process(p);	
    
   process* q;
   q = first_process;

   shell_is_interactive = isatty(shell_terminal);
   if(shell_is_interactive){ 

	next_p = first_process;

	
	while((next_p->next != NULL)){
		
		if(next_p->pid == 0 ){
			break;	
		}
		else
			next_p = next_p->next;
	}



	if(next_p != NULL){
		fundex = lookup(next_p->argv[0]); /* Is first token a shell literal */

		if(fundex >= 0) 
			cmd_table[fundex].fun(&(next_p->argv[1]));
		else {
		  launch_process(p);
		 }
	}

	process *pi;
	
	for (pi = first_process; pi; pi = pi->next){
		if(pi->stopped == 1){
			if(pi->background == 1)
				printf("bg pid: %d stopped\n", pi->pid);
			else
				printf("fg pid: %d stopped\n", pi->pid);

		}
		if(pi->completed == 1){
			if(pi->background == 1)
				printf("bg pid: %d completed\n", pi->pid);
			else{}		
			//	printf("fg pid: %d completed\n", pi->pid);
		}
	}


	purge_proc_list();
	

    getcwd(cwd,1000);
    fprintf(stdout, "%s %d: ", cwd, lineNum); 

   }


  }
  return 0;
}
