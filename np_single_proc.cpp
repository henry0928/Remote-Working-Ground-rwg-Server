//#define _CRT_SECURE_NO_WARNINGS
# include <unistd.h> // getdtablesize()
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
#include <algorithm> // sort()
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "npshell_singleproccess.h"
using namespace std;

vector<command_node>command_list; // extern
vector<user_info>user_list ; // extern
vector<userpipe_node>userpipe_list ; // extern
//char* line_input = (char*)malloc(sizeof(char) * 15001); //extern
char* line_input ;
int user_index = -1 ; //extern 
bool percent = true ; //extern

void server( int portnum, char** envp ) ;
int shell_service( int client_fd ) ;
void set_info( user_info &info, int cli_fd, int id, int port ) ;
int find_user( int fd ) ;
void login( int index ) ;
void logout( int index ) ;
void welcome( int index ) ;
int id_depatcher() ;
void fix_lineinput( char * line ) ;
bool compare( user_info user1, user_info user2 ) ;
void seperate( char** str, char** var, char** value )  ;
void default_environment( char** var, char** value ) ;
void set_environment() ;
void clean_userpipe( int id ) ;


int main( int argc, char* argv[], char* envp[] ) {
  char * port = (char*)malloc(sizeof(char)*8) ;
  strcpy( port, argv[1] ) ;
  server( atoi(port), envp ) ; 
} // main()

void server( int portnum, char** envp ) {
  init_env() ;
  char** default_env = (char**)malloc(sizeof(char*)*100) ;
  char** var = (char**)malloc(sizeof(char*)*100) ;
  char** value = (char**)malloc(sizeof(char*)*100) ;
  for( int h = 0 ; h < 100 ; h++ ) {
    default_env[h] = (char*)malloc(sizeof(char)*900) ;
    var[h] = (char*)malloc(sizeof(char)*50) ;
    value[h] = (char*)malloc(sizeof(char)*850) ;
  } // for   
  int p = 0 ;
  while ( envp[p] != NULL ) {
    strcpy( default_env[p], envp[p] ) ;
    p = p + 1 ;
  } // while
  default_env[p] = NULL ;
  seperate( default_env, var, value ) ;
  user_list.clear() ;
  userpipe_list.clear() ;
  struct sockaddr_in socket_addr ;
  struct sockaddr_in client_addr ; 
  int server_socket_fd ;
  int client_socket_fd ;
  int fd_iterator = 0 ;
  int index = -1 ;
  int optval = 1 ;
  bool escape = false ;
  char* free_pointer ;
  socklen_t alen = sizeof(client_addr); // just copy from slides
  fd_set active_set ;
  fd_set reading_set ;
  user_info temp ;
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
  socket_addr.sin_port = htons(portnum) ;
  setsockopt( server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, 4 ) ;
  if ( bind(server_socket_fd, (const struct sockaddr *)&socket_addr, sizeof(socket_addr)) == -1 ) {
    fprintf( stderr, "bind() error!" ) ;
    cout << "errno_num:" << errno ;
    close(server_socket_fd) ;
    exit(0) ; 
  } // if   
  /* listen the connection request,put it in the queue*/
  if( listen(server_socket_fd, 30) == -1 ) {
    fprintf( stderr, "listen() error!" ) ;
    close(server_socket_fd) ;
    exit(0) ; 
  } // if

  FD_ZERO( &active_set ) ; // let whole set be zero
  FD_SET(server_socket_fd, &active_set) ; // let server_socket put in the set 
  int max_fd = getdtablesize() ;
  /* keep accept connection request*/
  while (1) {
    //cerr << "socket is listening......\n" ;
    memcpy( &reading_set, &active_set, sizeof(active_set) ) ;
    escape = false ;
    //if ( select( max_fd, &reading_set, NULL, NULL, (struct timeval*)0 ) == -1 ) {
      //escape = true ;
      // still need control error!
    //} // if
    while ( select( max_fd, &reading_set, NULL, NULL, (struct timeval*)0 ) == -1 )
      ;
    if ( FD_ISSET(server_socket_fd, &reading_set) == true && escape == false ) {
      client_socket_fd = accept( server_socket_fd, (struct sockaddr *)&client_addr, &alen) ;
      if ( client_socket_fd == -1 ) {
        close(server_socket_fd) ;
        exit(0) ; 
      } // if 
      FD_SET(client_socket_fd, &active_set) ;
      if ( id_depatcher() == -1 )
        fprintf( stderr, "id_depatcher error!" ) ;  
      set_info( temp, client_socket_fd, id_depatcher(), ntohs(client_addr.sin_port) ) ;
      user_list.push_back(temp) ;
      index = find_user(client_socket_fd) ;
      user_list[index].name = (char*)malloc( sizeof(char) * 30 ) ;
      user_list[index].ip = (char*)malloc( sizeof(char) * 30 ) ;
      user_list[index]._var = (char**)malloc( sizeof(char*) * 20 ) ;
      user_list[index]._value = (char**)malloc( sizeof(char*) * 20 ) ;
      for( int j = 0 ; j < 20 ; j++ ) {
        user_list[index]._var[j] = (char*)malloc( sizeof(char) * 256 ) ;
        user_list[index]._value[j] = (char*)malloc( sizeof(char) * 256 ) ;
      } // for
      strcpy( user_list[index].name, "(no name)" ) ;
      strcpy( user_list[index].ip, inet_ntoa( client_addr.sin_addr ) ) ;
      strcpy( user_list[index]._var[0], "PATH" ) ;
      strcpy( user_list[index]._value[0], "bin:." ) ;
      user_list[index].var_index = 1 ;
      sort(user_list.begin(), user_list.end(), compare) ;
      default_environment( var, value ) ;
      welcome(find_user(client_socket_fd)) ;
      login( find_user(client_socket_fd) ) ;
      send(client_socket_fd, "% ", strlen("% "), 0 ) ;
      //free(temp.ip) ;
    } // if
    fd_iterator = 0 ;
    while ( fd_iterator < max_fd && escape == false ) {
      //default_environment( var, value ) ; 
      if ( fd_iterator != server_socket_fd && FD_ISSET(fd_iterator, &reading_set) == true ) {
        default_environment( var, value ) ; 
        dup2( fd_iterator, STDOUT_FILENO ) ;
        dup2( fd_iterator, STDERR_FILENO ) ;
        user_index = find_user(fd_iterator) ; // user_index is extern number
        set_environment() ;
        if ( user_index == -1 ) {
          fprintf( stderr, "user_index error!" ) ;
          exit(0) ;
        } // if
        if ( shell_service(fd_iterator) == 0 ) { // return 0 means client is exit
          logout(find_user(fd_iterator)) ;
          for ( int i = 0 ; i < 20 ; i++ ) {
            free_pointer = user_list[find_user(fd_iterator)]._var[i] ;
            free(free_pointer) ;
            free_pointer = user_list[find_user(fd_iterator)]._value[i] ;
            free(free_pointer) ;
          } // for 
          free( user_list[find_user(fd_iterator)]._var ) ;
          free( user_list[find_user(fd_iterator)]._value ) ;
          clean_userpipe( user_list[find_user(fd_iterator)].id ) ;
          user_list.erase(user_list.begin()+find_user(fd_iterator)) ;
          close(fd_iterator) ; // it means client is exit, so close fd 
          FD_CLR( fd_iterator, &active_set ) ;
          dup2( STDIN_FILENO, STDOUT_FILENO ) ;
          dup2( STDIN_FILENO, STDERR_FILENO ) ;
        } // if
        else {
          send(fd_iterator, "% ", strlen("% "), 0) ;
        } // else      
      } // if 
      fd_iterator = fd_iterator + 1 ; 
    } // while 

  } // while 
    
} // server()

bool compare( user_info user1, user_info user2 ) {
  return (user1.id < user2.id) ;
} // compare()

void seperate( char** str, char** var, char** value ) {
  int i = 0 ;
  while ( str[i] != NULL ) {
    var[i] = strtok( str[i], "=" ) ;
    value[i] = strtok( NULL, "\0" ) ;
    i = i + 1 ;
  } // while 

  var[i] = NULL ;
  value[i] = NULL ;
} // seperate()

void default_environment( char** var, char** value ) {
  int i = 0 ;
  while ( var[i] != NULL ) {
    setenv( var[i], value[i], 1 ) ;
    i = i + 1 ;
  } // while 
} // default_environment()

void set_environment() {
  int i = 0 ;
  while ( i < user_list[user_index].var_index ) {
    setenv( user_list[user_index]._var[i], user_list[user_index]._value[i], 1 ) ;
    i = i + 1 ;
  } // while 
} // set_environment()

void set_info( user_info &info, int cli_fd, int id, int port ) {
  info.id = id ;
  info.user_fd = cli_fd ;
  info.port = port ;
} // set_info()

void fix_lineinput( char * line ) {
  const char* del = "\r\n" ;
  //line_input = (char*)malloc(sizeof(char) * 15001);
  strcpy( line_input, line ) ;
  line_input = strtok( line_input, del);
} // fix_lineinput()

int find_user( int fd ) {
  int i = 0 ;
  while ( i < user_list.size() ) {
    if ( user_list[i].user_fd == fd )
      return i ;
    i = i + 1 ;
  } // while

  return -1 ; 

} // find_user()

int id_depatcher() { 
  int id = 1 ;
  int i = 0 ;
  while ( id < 31 ) {
    i = 0 ;
    while ( i < user_list.size() ) {
      if ( user_list[i].id == id )
        break ;
      i = i + 1 ;
    } // while

    if ( i == user_list.size() )
      return id ;
    id = id + 1 ;
  } // while    

  return -1 ;
} // id_depatcher()

void clean_userpipe( int id ) {
  int i = userpipe_list.size() - 1  ;
  while ( i >= 0 ) {
    if ( id == userpipe_list[i].send_id || id == userpipe_list[i].receive_id ) {
      close(userpipe_list[i].readfd) ;
      close(userpipe_list[i].writefd) ;
      userpipe_list.erase( userpipe_list.begin() + i ) ;
    } // if 
    i = i - 1 ;
  } // while  

} //clean_userpipe()

void welcome( int index ) {
  send( user_list[index].user_fd, "****************************************\n", strlen("****************************************\n"), 0 ) ;
  send( user_list[index].user_fd, "** Welcome to the information server. **\n", strlen("** Welcome to the information server. **\n"), 0 ) ;
  send( user_list[index].user_fd, "****************************************\n", strlen("****************************************\n"), 0 ) ;
} // welcome()

void login( int index ) {
  char* login_info = (char*)malloc(sizeof(char) * 200) ;
  char* temp = (char*)malloc(sizeof(char) * 10) ; 
  strcpy( login_info, "*** User '" ) ;
  strcat( login_info, user_list[index].name ) ;
  strcat( login_info, "' entered from " ) ;
  strcat( login_info, user_list[index].ip ) ;
  strcat( login_info, ":" ) ;
  sprintf( temp, "%d", user_list[index].port ) ;   
  strcat( login_info, temp ) ;
  strcat( login_info, ". ***\n" ) ;
  int i = 0 ;
  int size = user_list.size() ;
  while ( i < size ) {
    send(user_list[i].user_fd, login_info, strlen(login_info), 0) ;
    i = i + 1 ;
  } // while 

  free(login_info) ;
  free(temp) ;

} // login()

void logout( int index ) {
  char* login_info = (char*)malloc(sizeof(char) * 200) ; 
  strcpy( login_info, "*** User '" ) ;
  strcat( login_info, user_list[index].name ) ;
  strcat( login_info, "' left. ***\n" ) ;
  int i = 0 ;
  int size = user_list.size() ;
  while ( i < size ) {
    if ( i != index ) 
      send(user_list[i].user_fd, login_info, strlen(login_info), 0) ;
    i = i + 1 ;
  } // while 
  
  free(login_info) ;
} // logout()

int shell_service( int client_fd ) {
  percent = true ;
  int return_val = 0 ;
  char* input = (char*)malloc(sizeof(char) * 15001);
  line_input = (char*)malloc(sizeof(char) * 15001);
  memset( input , '\0', 15001 ) ;
  recv(client_fd, input , 15001, 0) ;
  if ( input != NULL ){
    if ( strcmp( input , "\r\n") != 0 ) {
      fix_lineinput(input) ; 
      adjust_fd_list() ;
      return_val = parser(input);
    } // if
    else 
      return_val = 1 ; // still not exit, its just a enter
  } // if 
  free(input) ;
  free(line_input) ;
  return return_val ;

} // shell_service()

