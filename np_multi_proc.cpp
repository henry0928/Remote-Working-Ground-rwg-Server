//#define _CRT_SECURE_NO_WARNINGS
# include <stdio.h>
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
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "npshellforserver3.h"
using namespace std;
#define PORT 7777

vector<command_node>command_list; //extern 
vector<fd_node>fd_list ; //extern 
vector<fifo_node>receive_list ; //extern
char* message ; //extern 
char* loginfo_ptr ; // not extern 
userpipe_info* userpipe_ptr ; //extern
user_info* user ; // extern 
int userlist_index ; //extern 
string c_line ;
sem_t *message_sem ; // semaphore_for_message

void server( int port ) ;
void shell_service( int client_fd, user_info* ptr, int index, sem_t *sem ) ;
void welcome( int fd ) ;
void login( user_info* ptr, char* message_ptr, int index, sem_t *sem  ) ;
void logout( user_info* ptr, char* message_ptr, int index, sem_t *sem  ) ;
void who( int client_fd, user_info* ptr, int index ) ;
void tell( char* str, user_info* ptr, int index ) ;
void yell( char* str, user_info* ptr, int index ) ;
void name( char* str, user_info* ptr, int index ) ;
bool check_name( user_info* ptr, char* str ) ;
bool check_id( user_info* ptr, int id ) ;
int id_depatcher( vector<int>_list ) ;
void check_userlist_id( user_info* ptr, vector<int>&_list ) ;
void clean_userlist( user_info* ptr, int index ) ;
void clean_userpipe( int id ) ;
void sigusr1_handler( int sig ) ;
void sigusr2_handler( int sig ) ;
void sigint_handler( int sig ) ;


int main( int argc, char* argv[], char* envp[] ) {
  char* portnum = (char*)malloc(sizeof(char)*8) ;
  if ( argv[1] != NULL )
    strcpy( portnum, argv[1] ) ;
  server( atoi(portnum) ) ; 
} // main()

void server( int port ) {
  vector<int>userid_list ;
  userid_list.clear() ;
  struct sockaddr_in socket_addr ;
  struct sockaddr_in client_addr ;
  socklen_t alen = sizeof(client_addr); // just copy from slides 
  int server_socket_fd ;
  int client_socket_fd ;
  int client_addr_len ;
  int type ;
  int optval = 1 ;
  pid_t pid ;
  sem_t *sem ; // semaphore
  sem = sem_open( "/userlist_sem", O_CREAT, 0666, 1 ) ;
  message_sem  = sem_open( "/message_sem", O_CREAT, 0666, 1 ) ;
  
  const char* shm_name = "/shm_userlist" ;
  const int shm_size = sizeof(user_info) * 30 ;
  int shm_fd = shm_open( shm_name, O_CREAT|O_RDWR|O_TRUNC, 0666 ) ; // create share memory
  ftruncate( shm_fd, shm_size ) ; // set share memory size
  void * shm_ptr = mmap(NULL, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0 ) ; // shm_ptr point to share memory
  user = (user_info*)shm_ptr ; // the start address of userlist
  
  const char* shm_message = "/shm_message" ;
  const int shm_message_size = sizeof(char) * 15085 ;
  int message_fd = shm_open( shm_message, O_CREAT|O_RDWR|O_TRUNC, 0666 ) ; // create share memory
  ftruncate( message_fd, shm_message_size ) ; // set share memory size
  void * message_ptr = mmap(NULL, shm_message_size, PROT_READ|PROT_WRITE, MAP_SHARED, message_fd, 0 ) ; // message_ptr point to share memory
  message = (char*)message_ptr ; // the start address of message

  const char* shm_userpipe = "/shm_userpipe" ;
  const int shm_userpipe_size = sizeof(userpipe_info) * 900 ;
  int userpipe_fd = shm_open( shm_userpipe, O_CREAT|O_RDWR|O_TRUNC, 0666 ) ; // create share memory
  ftruncate( userpipe_fd, shm_userpipe_size ) ; // set share memory size
  void * up_ptr = mmap(NULL, shm_userpipe_size, PROT_READ|PROT_WRITE, MAP_SHARED, userpipe_fd, 0 ) ; // message_ptr point to share memory
  userpipe_ptr = (userpipe_info*)up_ptr ; // the start address of message

  signal( SIGINT, sigint_handler) ; // for open fifo
  close(shm_fd) ;
  close(message_fd) ;
  close(userpipe_fd) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    user[i].valid = false ; // init the userlist valid
  } // for

  for ( int j = 0 ; j < 900 ; j++ ) {
    userpipe_ptr[j].valid = false ;
    userpipe_ptr[j]._open = false ;
  } // for    
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
    client_socket_fd = accept( server_socket_fd, (struct sockaddr *)&client_addr, &alen) ;
    if ( client_socket_fd == -1 ) {
      fprintf( stderr, "accept() error!" ) ;
      close(server_socket_fd) ;
      exit(0) ; 
    } // if
    else 
      cout << "accept success!\n" ;
    userid_list.clear() ;
    sem_wait(sem) ;
    check_userlist_id( user, userid_list ) ;
    userlist_index = id_depatcher( userid_list ) - 1  ;
    user[userlist_index].id = userlist_index + 1 ;
    user[userlist_index].fd = client_socket_fd ;
    memset( user[userlist_index].name, 0, 20 ) ;
    strcpy(user[userlist_index].name,  "(no name)") ;
    memset( user[userlist_index].ip, 0, 40 ) ;
    strcpy(user[userlist_index].ip, inet_ntoa( client_addr.sin_addr) ) ;
    user[userlist_index].port = ntohs(client_addr.sin_port) ;
    user[userlist_index].valid = true ;
    sem_post(sem) ;
    pid = fork() ;
    if ( pid == -1 ) {
      fprintf( stderr, "fork() error!" ) ;
      wait(NULL) ;
    } // if   
    else if ( pid == 0 ) { // childprocess
      welcome( client_socket_fd ) ;
      close(server_socket_fd) ;
      user[userlist_index].pid = getpid() ;
      dup2( client_socket_fd, STDOUT_FILENO) ;
      dup2( client_socket_fd, STDERR_FILENO) ;
      signal( SIGUSR1, sigusr1_handler) ; // for broadcast message
      signal( SIGUSR2, sigusr2_handler) ; // for open fifo
      login( user, message, userlist_index, sem ) ;
      shell_service( client_socket_fd, user, userlist_index, sem ) ;
      logout( user, message, userlist_index, sem ) ;
      close(client_socket_fd) ;
      exit(0) ; 
    } // else if 
    else { // parentprocess
      close(client_socket_fd) ;
    } // else 
  } // while 
    
} // server()

void welcome( int fd ) {
  send( fd, "****************************************\n", strlen("****************************************\n"), 0 ) ;
  send( fd, "** Welcome to the information server. **\n", strlen("** Welcome to the information server. **\n"), 0 ) ;
  send( fd, "****************************************\n", strlen("****************************************\n"), 0 ) ;
} // welcome()

void login( user_info* ptr, char* message_ptr, int index, sem_t *sem ) {
  char* login_info = (char*)malloc(sizeof(char) * 200) ;
  sprintf( login_info, "*** User '%s' entered from %s:%d. ***\n", ptr[index].name, ptr[index].ip, ptr[index].port ) ;
  sem_wait(sem) ;
  memset( message_ptr, 0, 15085 ) ;
  strcpy( message_ptr, login_info ) ;
  sem_post(sem) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( ptr[i].valid == true ) {
      kill( ptr[i].pid, SIGUSR1 ) ;
    } // if     
  } // for

  free(login_info) ; 
} // login()

void logout( user_info* ptr, char* message_ptr, int index, sem_t *sem ) {
  char* logout_info = (char*)malloc(sizeof(char) * 200) ;
  sprintf( logout_info, "*** User '%s' left. ***\n", ptr[index].name ) ;
  sem_wait(sem) ;
  memset( message_ptr, 0, 15085 ) ;
  strcpy( message_ptr, logout_info ) ;
  sem_post(sem) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( ptr[i].valid == true ) {
      kill( ptr[i].pid, SIGUSR1 ) ;
    } // if     
  } // for

  free(logout_info) ;
} // logout()

void sigusr1_handler( int sig ) { 
  char info[15085] ;
  strcpy( info, message ) ;
  send( STDOUT_FILENO, info, strlen(info), 0 ) ;   
} //sigusr1_handler()

void sigusr2_handler( int sig ) {
  int userpipe_fd = -1 ; 
  int myid = userlist_index + 1 ; 
  fifo_node temp ;
  for ( int i = 0 ; i < 900 ; i++ ) {
    if ( userpipe_ptr[i].valid == true && userpipe_ptr[i].receiveid == myid && userpipe_ptr[i]._open == true ) {
      userpipe_ptr[i]._open = false ;
      userpipe_fd = open( userpipe_ptr[i].file_name, O_RDONLY ) ;
      if ( userpipe_fd != -1 ) {
        temp.fd = userpipe_fd ;
        temp.sender = userpipe_ptr[i].sendid ;
        strcpy( temp.file_name, userpipe_ptr[i].file_name ) ; 
        receive_list.push_back(temp) ;  
      } // if
      else 
        cerr << "open fifo error!\n" ;
      break ;
    } // if 
  } // for 

} //sigusr2_handler()

void sigint_handler( int sig ) {
  shm_unlink( "/shm_userlist" ) ;
  shm_unlink( "/shm_message" ) ;
  shm_unlink( "/shm_userpipe" ) ;
  sem_unlink( "/userlist_sem" ) ;
  sem_unlink( "/message_sem" ) ;
  exit(0) ;
} // sigint_handler() 

void check_userlist_id( user_info* ptr, vector<int>&_list ) {
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( ptr[i].valid == true )
      _list.push_back(ptr[i].id) ;
  } // for 

} // check_userlist_id()

int id_depatcher( vector<int>_list ) { 
  int id = 1 ;
  int i = 0 ;
  while ( id < 31 ) {
    i = 0 ;
    while ( i < _list.size() ) {
      if ( _list[i] == id )
        break ;
      i = i + 1 ;
    } // while

    if ( i == _list.size() )
      return id ;
    id = id + 1 ;
  } // while    

  return -1 ;
} // id_depatcher()

void clean_userlist( user_info* ptr, int index ) {
  ptr[index].valid = false ;  
} // clean_userlist()

void who( int client_fd, user_info* ptr, int index ) {
  char* info = (char*)malloc(sizeof(char)*200) ;
  memset( info, 0, 200 ) ;
  send( client_fd, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n", strlen("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n"), 0 ) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( ptr[i].valid == true ) {
      if ( index == i )
        sprintf( info, "%d\t%s\t%s:%d\t<-me\n", ptr[i].id, ptr[i].name, ptr[i].ip, ptr[i].port ) ;
      else {
        sprintf( info, "%d\t%s\t%s:%d\n", ptr[i].id, ptr[i].name, ptr[i].ip, ptr[i].port ) ;
      } // else      
      send( client_fd, info, strlen(info), 0 ) ;
    } // if 
  } // for

  free(info) ;
} // who()

void name( char* str, user_info* ptr, int index ) {
  char* info = (char*)malloc(sizeof(char)*200) ;
  memset( info, 0, 200 ) ;
  if ( check_name( ptr, str ) == true ) {
    memset( ptr[index].name, 0, 20 ) ;
    strcpy( ptr[index].name, str ) ;
    sprintf( info, "*** User from %s:%d is named '%s'. ***\n", ptr[index].ip, ptr[index].port, str ) ;
    memset( message, 0, 15085 ) ;
    strcpy( message, info ) ;
    for ( int i = 0 ; i < 30 ; i++ ) {
      if ( ptr[i].valid == true )
        kill( ptr[i].pid, SIGUSR1 ) ;     
    } // for
  } // if 
  else {
    sprintf( info, "*** User '%s' already exists. ***\n", str ) ;
    send( ptr[index].fd, info, strlen(info), 0 ) ;
  } // else

  free(info) ;
} // name()

void tell( char* str, user_info* ptr, int index ) {
  const char* del = "\t " ;
  char info[1200] ;
  memset( info, 0, 1200 ) ;
  int id = -1 ;
  str = strtok( str, del ) ;
  if ( str != NULL )
    id = atoi(str) ;
  else 
    send( ptr[index].fd, "str is not number!\n", strlen("str is not number!\n"), 0 ) ;
  if ( id != -1 && check_id( ptr, id ) == true ) {
    str = strtok( NULL, "\0" ) ;
    sprintf( info, "*** %s told you ***: %s\n", ptr[index].name, str ) ;
    memset( message, 0, 15085 ) ;
    strcpy( message, info ) ;
    kill( ptr[id-1].pid, SIGUSR1 ) ;
  } // if 
  else {
    sprintf( info, "*** Error: user #%d does not exist yet. ***\n", id ) ;
    send( ptr[index].fd, info, strlen(info), 0 ) ;
  } // else 

} // tell()

void yell( char* str, user_info* ptr, int index ) {
  char* info = (char*)malloc(sizeof(char)*1200) ;
  memset( info, 0, 1200 ) ;
  sprintf( info, "*** %s yelled ***: %s\n", ptr[index].name, str ) ;
  memset( message, 0, 15085 ) ;
  strcpy( message, info ) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( ptr[i].valid == true )
      kill( ptr[i].pid, SIGUSR1 ) ;     
  } // for

  free(info) ;
} // yell()

bool check_name( user_info* ptr, char* str ) {
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( ptr[i].valid == true ) {
      if ( strcmp( str, ptr[i].name ) == 0 )
        return false ;  
    } // if 
  } // for 

  return true ;
} // check_name()

bool check_id( user_info* ptr, int id ) {
  if ( ptr[id-1].valid == true )
    return true ;
  else 
    return false ;

  return false ;
} // check_id()

void clean_userpipe( int id ) {
  for ( int i = 0 ; i < 900 ; i++ ) {
    if ( id == userpipe_ptr[i].sendid || id == userpipe_ptr[i].receiveid ) {
      userpipe_ptr[i]._open = false ; 
      userpipe_ptr[i].valid = false ;
      unlink( userpipe_ptr[i].file_name ) ;
    } // if 
  } // for 
} // clean_userpipe()

void shell_service( int client_fd, user_info* ptr, int index, sem_t *sem ) {
  init_env() ;
  command_list.clear() ;
  fd_list.clear() ;
  receive_list.clear() ;
  const char* del = "\r\n" ;
  const char* del2 = "\t " ;
  char* str1 = (char*)malloc(sizeof(char) * 15001) ;
  char* input = (char*)malloc(sizeof(char) * 15001);
  char* temp_input ;
  while (1) {
    memset( input, 0, 15001 ) ;
    memset( str1, 0, 15001 ) ;
    send(client_fd, "% ", strlen("% "), 0) ;
    while ( recv(client_fd, input, 15001, 0) == -1 )
      ;
    sem_wait(sem) ;
    if ( input != NULL ){
      if ( strcmp( input, "\r\n") != 0 ) { 
        strcpy( str1, input ) ;
        temp_input = strtok( str1, del ) ;
        c_line = temp_input ;
        temp_input = strtok( temp_input, del2 ) ;
      } // if
      if ( strcmp( input, "\r\n") == 0 ) 
        ;   
      else if ( strcmp( c_line.c_str(), "who" ) == 0 ) {
        //sem_wait(sem) ;
        who( client_fd, ptr, index );
        //sem_post(sem) ;
      } // else if
      else if ( temp_input != NULL && strcmp(temp_input, "name") == 0 ) {
        temp_input = strtok( NULL, "\0" ) ;
        if ( temp_input != NULL ) {
          //sem_wait(sem) ; 
          name( temp_input, ptr, index ) ;
          //sem_post(sem) ;
        } // if   
      } // else if
      else if ( temp_input != NULL && strcmp(temp_input, "tell") == 0 ) {
        temp_input = strtok( NULL, "\0" ) ;
        if( temp_input != NULL ) {
          //sem_wait(sem) ; 
          tell( temp_input, ptr, index ) ;
          //sem_post(sem) ;
        } // if   
      } // else if
      else if ( temp_input != NULL && strcmp(temp_input, "yell") == 0 ) {
        temp_input = strtok( NULL, "\0" ) ;
        if( temp_input != NULL ) {
          //sem_wait(sem) ;
          yell( temp_input, ptr, index ) ;
          //sem_post(sem) ;
        } // if   
      } // else if
      else if ( strcmp( input, "\r\n") != 0 ) {
        adjust_fd_list() ;
        if ( parser(input) == 0 ) {
          clean_userlist( user, userlist_index ) ;
          clean_userpipe( userlist_index ) ;
          sem_post(sem) ;
          break ;
        } // if   
      } // else if 
      else 
        ;
    } // if 
    else { 
      sem_post(sem) ;
      break ;
    } // else
    sem_post(sem) ;
  } // while
  
  free(input) ;
  free(str1) ;
} // shell_service()
