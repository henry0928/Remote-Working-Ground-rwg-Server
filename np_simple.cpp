//#define _CRT_SECURE_NO_WARNINGS
# include <stdio.h>
# include <iostream> // cout, endl
# include <string> // string, find_last_of, substr
# include <string.h> // string, find_last_of, substr
# include <vector> // vector, push_back
# include <stdlib.h> // setenv() getenv()
# include <cstdlib> // system, atoi
# include <ctype.h> // isdigit() 
# include <cstdio> // system, atoi
# include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include "npshell_forserver.h"
using namespace std;
#define PORT 7777

vector<command_node>command_list;
vector<fd_node>fd_list ;

void server1( int port ) ;
void shell_service( int client_fd ) ;

int main( int argc, char* argv[], char* envp[] ) {
  char portnum[8] ;
  if ( argv[1] != NULL )
    strcpy( portnum, argv[1] ) ;
  server1( atoi(portnum) ) ; 
} // main()

void server1( int port ) {
  struct sockaddr_in socket_addr ;
  //struct sockaddr_in cli_addr ; 
  int server_socket_fd ;
  int client_socket_fd ;
  int client_addr_len ;
  int optval = 1 ;
  int type ;
  pid_t pid ;
  /* Allocate socket*/
  server_socket_fd = socket(AF_INET, SOCK_STREAM, 0) ;
  if ( server_socket_fd == -1 ) {
    fprintf( stderr, "socket() error!" ) ;
    exit(0) ;
  } // if
  cout << "server_socket_fd:" << server_socket_fd << "\n" ;   
  /* bind socketaddr to sockfd*/
  socket_addr.sin_family = AF_INET ;
  socket_addr.sin_addr.s_addr = INADDR_ANY ;
  socket_addr.sin_port = htons(port) ;
  setsockopt( server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, 4 ) ;
  if ( bind(server_socket_fd, (const struct sockaddr *)&socket_addr, sizeof(socket_addr)) == -1 ) {
    fprintf( stderr, "bind() error!" ) ;
    cout << errno ;
    close(server_socket_fd) ;
    exit(0) ; 
  } // if   
  /* listen the connection request,put it in the queue*/
  if( listen(server_socket_fd, 30) == -1 ) {
    fprintf( stderr, "listen() error!" ) ;
    close(server_socket_fd) ;
    exit(0) ; 
  } // if
  /* keep accept connection request*/
  while (1) {
    cout << "socket is listening......\n" ;
    client_socket_fd = accept(server_socket_fd, NULL, NULL) ;
    if ( client_socket_fd == -1 ) {
      fprintf( stderr, "accept() error!" ) ;
      close(server_socket_fd) ;
      exit(0) ; 
    } // if
    else 
      cout << "accept success!\n" ;  
    pid = fork() ;
    if ( pid == -1 ) {
      fprintf( stderr, "fork() error!" ) ;
      wait(NULL) ;
    } // if   
    else if ( pid == 0 ) { // childprocess
      close(server_socket_fd) ;
      dup2( client_socket_fd, STDOUT_FILENO) ;
      dup2( client_socket_fd, STDERR_FILENO) ;
      shell_service( client_socket_fd ) ;
      close(client_socket_fd) ; 
      exit(0) ; 
    } // else if 
    else { // parentprocess
      close(client_socket_fd) ;
      waitpid(pid, NULL, 0) ; 
    } // else 
  } // while 
    
} // server1()

void shell_service( int client_fd ) {
  init_env() ;
  command_list.clear() ;
  fd_list.clear() ;
  int num = -99 ;
  char* input = (char*)malloc(sizeof(char) * 15001);
  while (1) {
    memset( input, '\0', 15001 ) ;
    send(client_fd, "% ", strlen("% "), 0) ;
    num = recv(client_fd, input, 15001, 0) ;
    //token = strtok(input, d);
    if ( input != NULL ){
      if ( strcmp( input, "\r\n") != 0 ) {
        adjust_fd_list() ;
        parser(input);
      } // if 
    } // if 
    else { 
      break ;
    } // else   
  
    free(input) ;
    input = (char*)malloc(sizeof(char) * 15001);
  } // while 

} // shell_service()

