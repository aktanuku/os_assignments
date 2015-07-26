#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include <dirent.h>
#include "libhttp.h"

#define LIBHTTP_REQUEST_MAX_SIZE 8192

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

/*
 * Sends file by name line by line
 */
void send_file_by_name(char * fname, int fd){
	char *buffer= 0;
	long length;
	FILE * f = fopen(fname,"rb");

	if(f)
	{
	  fseek(f,0, SEEK_END);
	  length = ftell(f);
	  fseek(f, 0, SEEK_SET);
	  buffer = malloc(length);
          if (buffer)
	   {
            fread(buffer, 1, length, f);
           }
           fclose(f);

	}

	if(buffer){
	   http_start_response(fd, 200);
  	   http_send_header(fd, "Content-type", http_get_mime_type(fname) );
  	   http_end_headers(fd);
	   http_send_data(fd, buffer, length);
	   free(buffer);
        }
	
  	


}


void list_files_in_dir(char * dir_name, int fd){

	http_start_response(fd, 200);
  	http_send_header(fd, "Content-type", "text/html");
  	http_end_headers(fd);
	char send_buf[2048];
	http_send_string(fd, "<!DOCTYPE html>\n");
	http_send_string(fd, "<html>\n");
	http_send_string(fd, "<body>\n");
	DIR *dir = opendir(dir_name);
		struct dirent *entry;
		while((entry = readdir(dir)) != NULL)
		{
			sprintf(send_buf,"<br> <a href=\"http://192.168.162.162:8000/%s\"> %s </a>",entry->d_name, entry->d_name); 
			http_send_string(fd, send_buf);
		}
	http_send_string(fd,"</body>\n");
	http_send_string(fd,"</html>\n");
	closedir(dir);
}

void send_404(int fd){

  http_start_response(fd, 200);
  http_send_header(fd, "Content-type", "text/html");
  http_end_headers(fd);
  http_send_string(fd, "<center><h1>http Error 404.</h1><hr><p>File not found.</p></center>");
}
/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd)
{
  int found = 0;
  char working_path[2048];
 

  struct http_request *request = http_request_parse(fd);
   
  if(request->path[0] == '/'){
	printf("method: %s, path: %s\n", request->method, request->path);
  	printf("serving out of %s\n", server_files_directory); 
	sprintf(working_path, "%s%s", server_files_directory, request->path);
	printf("constructed path: %s\n",working_path); 	
	struct stat st;
	stat(working_path, &st);
	if(S_ISDIR(st.st_mode)){
		found = 0;
		DIR *dir = opendir(working_path);
		struct dirent *entry;
		while((entry = readdir(dir)) != NULL)
		{
			if (strcmp(entry->d_name, "index.html") == 0){
				found = 1;
				char file_to_send[2048];
				sprintf(file_to_send, "%s/%s",working_path,"index.html");
				send_file_by_name(file_to_send, fd);	
			}
		}
		closedir(dir);
		if(found == 0){
			list_files_in_dir(working_path, fd);
		}			
	}
	else if (S_ISREG(st.st_mode)){
		send_file_by_name(working_path, fd);
	}
	else{
		send_404(fd);
	}
  }


//  http_start_response(fd, 200);
//  http_send_header(fd, "Content-type", "text/html");
//  http_end_headers(fd);
//  http_send_string(fd, "<center><h1>Welcome to httpserver!</h1><hr><p>Nothing's here yet.</p></center>");

}

int proxy_request(int fd, int fd_send){
 
 char *read_buffer = malloc(LIBHTTP_REQUEST_MAX_SIZE + 1);
  if (!read_buffer) http_fatal_error("Malloc failed");

  int bytes_read = read(fd, read_buffer, LIBHTTP_REQUEST_MAX_SIZE);
 
  int bytes_sent = write(fd_send, read_buffer, bytes_read);

  

  if(bytes_read == bytes_sent)
	printf("everything sent\n");
  else
	printf("bytes read: %d, bytes recieved %d\n", bytes_read, bytes_sent);


  return bytes_sent;
}
 
void init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port){
	memset(name, 0, sizeof(*name));

	struct hostent *hostinfo;
	
	name->sin_family = AF_INET;
	
	name->sin_port = htons(port);
	
	hostinfo = gethostbyname(hostname);
	
	if(hostinfo == NULL)
		{
		  fprintf(stderr, "Unknown host %s.\n", hostname);
		  exit( EXIT_FAILURE);
		}

	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
	
}



/*			
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd)
{
  printf("printing\n");
  char *p;
  /* YOUR CODE HERE */
//  p = dns_lookup(server_proxy_hostname);
  struct sockaddr_in name;
  //name = (struct sockaddr_in *)malloc(sizeof(struct sockadd_in)); 
  init_sockaddr(&name, server_proxy_hostname, server_proxy_port);
  
  int* socket_number_proxy;
  socket_number_proxy = (int *) malloc(sizeof(int));
  *socket_number_proxy = socket(PF_INET, SOCK_STREAM, 0);
  
  printf("proxy socket number %d\n", *socket_number_proxy);
  if(*socket_number_proxy == -1){
	fprintf(stderr, "failed to create new proxy socket: error %d: %s\n", errno, strerror(errno));
  	exit(errno);
   }
  
  printf("made proxy socket\n");
   int socket_option = 1;

   if(connect(*socket_number_proxy, (struct sockaddr*)&name, sizeof(name))){
	printf("error");
   }
  printf("Listening on port %d...\n", server_proxy_port);
  
  //get max file descriptor for select call
  int maxfd = (fd> *socket_number_proxy ? fd : *socket_number_proxy);

  //initalize read and write sets to pass to select call and clear them 
  fd_set readset;
  fd_set writeset;
  FD_ZERO(&readset);
  FD_ZERO(&writeset);

  //add the client file descriptors to read and write sets for select
  FD_SET(fd, &readset);
  FD_SET(fd, &writeset);

  //do same for remote server file descriptor
  FD_SET(*socket_number_proxy, &readset);
  FD_SET(*socket_number_proxy, &writeset);

  
  //temporary sets for polling i guess
  fd_set readtemp;
  fd_set writetemp;

  int result;

  printf("max fd %d\n", maxfd);
  int socketsopen = 1;
  int bytes_sent = 0;
  while(socketsopen){
//	printf("inside outer while loop\n");
        memcpy(&readtemp, &readset, sizeof(readtemp));
        memcpy(&writetemp, &writeset, sizeof(writetemp));

  	result = select(maxfd + 1, &readtemp, NULL, NULL, NULL);
	
		
	if(result == -1){
		printf("something bad happened\n");
	}
	else{
		printf("whats up doc?\n");
		if(FD_ISSET(fd, &readtemp)){
			printf("case 1\n");
			bytes_sent = proxy_request(fd, *socket_number_proxy);
		//	proxy_request(*socket_number_proxy, fd);

		}
		else if(FD_ISSET(*socket_number_proxy, &readtemp)){
			printf("case 2\n");
			bytes_sent = proxy_request(*socket_number_proxy, fd);
		//	proxy_request(fd, *socket_number_proxy);
		}
	}
	if (bytes_sent < 0){
		socketsopen = 0;
	}
	

  }
		
		

}

int socket_isalive(int fd){
	char c;
	int alive = 0;
	int x = 4;
    //    ssize_t x = recv(fd, &c, 1, MSG_PEEK);
	if(x > 0){
		alive = 1;
	}
	else if (x == 0){
		alive = 0;
	}
	else{
		//pretend the socket is dead for now 
		//i don't know how else this can be handled yet
		alive = 0;
	}
	
	return alive;
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int* socket_number, void (*request_handler)(int))
{

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;
  pid_t pid;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    fprintf(stderr, "Failed to set socket options: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr*) &server_address,
        sizeof(server_address)) == -1) {
    fprintf(stderr, "Failed to bind on socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    fprintf(stderr, "Failed to listen on socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  while (1) {

    client_socket_number = accept(*socket_number, (struct sockaddr*) &client_address,
        (socklen_t*) &client_address_length);
    if (client_socket_number < 0) {
      fprintf(stderr, "Error accepting socket: error %d: %s\n", errno, strerror(errno));
      continue;
    }

    printf("Accepted connection from %s on port %d\n", inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    pid = fork();
    if (pid > 0) {
      close(client_socket_number);
    } else if (pid == 0) {
      signal(SIGINT, SIG_DFL); // Un-register signal handler (only parent should have it)
      close(*socket_number);
      request_handler(client_socket_number);
      close(client_socket_number);
      exit(EXIT_SUCCESS);
    } else {
      fprintf(stderr, "Failed to fork child: error %d: %s\n", errno, strerror(errno));
      exit(errno);
    }
  }

  close(*socket_number);

}

int server_fd;
void signal_callback_handler(int signum)
{
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(signum);
}

char *USAGE = "Usage: ./httpserver --files www_directory/ --port 8000\n"
              "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{

  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  server_files_directory = malloc(1024);
  getcwd(server_files_directory, 1024);
  server_proxy_hostname = "inst.eecs.berkeley.edu";
  server_proxy_port = 80;

  void (*request_handler)(int) = handle_files_request;

  int i;
  for (i = 1; i < argc; i++)
  {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;

}
