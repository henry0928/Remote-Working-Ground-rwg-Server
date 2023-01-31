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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include "npshell_singleproccess.h"

using namespace std;

void init_env() {
  setenv( "PATH", "bin:.", 1 ) ;
} // init_env()

void who() {
  int i = 0 ;
  char* info = (char*)malloc( sizeof(char) * 100 ) ;
  send( user_list[user_index].user_fd, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n",
        strlen("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n"), 0 ) ;
  while ( i < user_list.size() ) {
    if ( i == user_index )
      sprintf( info, "%d\t%s\t%s:%d\t<-me\n", user_list[i].id, user_list[i].name, user_list[i].ip, user_list[i].port ) ;   
    else 
      sprintf( info, "%d\t%s\t%s:%d\n", user_list[i].id, user_list[i].name, user_list[i].ip, user_list[i].port ) ;   
    send( user_list[user_index].user_fd, info, strlen(info), 0 ) ;
    i = i + 1 ;
  } // while 

  free(info) ;
} // who()

void name( char* name_str ) {
  int i = 0 ;
  char* info = (char*)malloc( sizeof(char) * 100 ) ;
  if ( checkname(name_str) == true ) {
    strcpy( user_list[user_index].name, name_str ) ;
    sprintf( info, "*** User from %s:%d is named '%s'. ***\n", user_list[user_index].ip, user_list[user_index].port, name_str ) ;
    while ( i < user_list.size() ) {
      send( user_list[i].user_fd, info, strlen(info), 0 ) ;
      i = i + 1 ;
    } // while  
  } // if
  else {
    sprintf( info, "*** User '%s' already exists. ***\n", name_str ) ;
    send( user_list[user_index].user_fd, info, strlen(info), 0 ) ;
  } // else 

  free(info) ;

} // name()

bool checkname( char* str ) {
  int i = 0 ;
  while ( i < user_list.size() ) {
    if ( strcmp( user_list[i].name, str ) == 0 )
      return false ;
    i = i + 1 ;
  } // while

  return true ;
} // checkname()

void tell( char ** message ) {
  char* info = (char*)malloc(sizeof(char)* 1200) ;
  char* message_str = (char*)malloc(sizeof(char)*1024) ;
  memset( message_str, 0, sizeof(message_str) ) ;
  if ( line_input != NULL )
    strcpy( message_str, line_input ) ;
  int id = atoi(message[1]) ;
  const char* del = " \t" ;
  const char* del2 = "\0" ;
  char * token = strtok( message_str, del);
  token = strtok( NULL, del);
  token = strtok( NULL, del2) ;
  int i = 2 ;
  int index ;
  if ( checktell(id) == true ) {
    sprintf( info, "*** %s told you ***: %s\n", user_list[user_index].name, token ) ;
    index = useid_find_user( id ) ;
    send( user_list[index].user_fd, info, strlen(info), 0 ) ;
  } // if
  else {
    sprintf( info, "*** Error: user #%d does not exist yet. ***\n", id ) ;
    send( user_list[user_index].user_fd, info, strlen(info), 0 ) ;  
  } // else 

  free(info) ;
  free(message_str) ;
} // tell()

int useid_find_user( int id ) {
  int i = 0 ;
  while ( i < user_list.size() ) {
    if ( id == user_list[i].id )
      return i ; 
    i = i + 1 ;
  } // while 

  return -1 ; 

} // useid_find_user()

bool checktell( int id ) {
  int i = 0 ;
  while ( i < user_list.size() ) {
    if ( id == user_list[i].id )
      return true ;
    i = i + 1 ;
  } // while 

  return false ;
} // checktell()

void yell( char ** message ) {
  int i = 1 ;
  char * message_str = (char*)malloc(sizeof(char) * 1024) ;
  memset( message_str, 0, sizeof(message_str) ) ;
  if ( line_input != NULL )
    strcpy( message_str, line_input ) ;
  char * info = (char*)malloc(sizeof(char)* 1200) ;
  const char* del = " \t" ;
  const char* del2 = "\0" ;
  char * token = strtok( message_str, del);
  token = strtok( NULL, del2);
  sprintf( info, "*** %s yelled ***: %s\n", user_list[user_index].name, token ) ;
  i = 0 ;
  while ( i < user_list.size() ) {
    send( user_list[i].user_fd, info, strlen(info), 0 ) ;
    i = i + 1 ;
  } // while 

  free(message_str) ;
  free(info) ;
} // yell()

int parser(char* linestr) {
    const char* del = "\f\r\v\n\t ";
    char** argv = (char**)malloc(sizeof(char*) * 4000 ); // 4000隨便給的
    int i = 0;
    int j = 0;
    int pipe_num = 0;
    char * token = strtok(linestr, del);
    while (token != NULL) {
        argv[i] = token;
        token = strtok(NULL, del);
        i = i + 1;
    } // while

    argv[i] = token; // 給予最後一個NULL
    if ( argv[0] == NULL )
      return 0 ;
    else if ( is_build_in(argv[0]) == 1 )
      Setenv( argv[1], argv[2] ) ;
    else if ( is_build_in(argv[0]) == 2 )
      printenv( argv[1] ) ;
    else if ( is_build_in(argv[0]) == 3 ) {
      return 0 ;
    } // else if   
    else if ( is_build_in(argv[0]) == 4 )
      who() ;
    else if ( is_build_in(argv[0]) == 5 )
      tell(argv) ;
    else if ( is_build_in(argv[0]) == 6 )
      yell(argv) ; 
    else if ( is_build_in(argv[0]) == 7 )
      name( argv[1] ) ;    
    else if ( is_build_in(argv[0]) == 0 ) 
      depatcher(argv);   
    else
      ;
    free(argv) ;
    return 1 ;

} // parser

void depatcher(char** input) {
    int i = 0 ;
    int j = 0;
    int k = 0 ;
    int pipe_num = 0 ;
    int command_num = 0;
    int singlepipe_num = 0;
    int user_id = -1 ;
    bool mark = false ;
    command_node temp ;
    char** commands = (char**)malloc(sizeof(char*) * 8);
    command_list.clear();
    while (input[i] != NULL) {
        while ( input[i] != NULL && is_pipe(input[i], pipe_num, mark) == false
                && is_rediration(input[i]) == false && is_userpipe(input[i], user_id) == false ) {
            commands[j] = input[i];
            i = i + 1;
            j = j + 1;
        } // while
        
        if (input[i] == NULL) {
            commands[j] = input[i];
            temp.cstr = commands ;
            temp.type = 's' ;
            temp.num = 0 ;
            command_list.push_back(temp);
            command_num = command_num + 1;
        } // if 
        else if (is_pipe(input[i], pipe_num, mark) == true && pipe_num == 0) {
            singlepipe_num = singlepipe_num + 1 ;
            commands[j] = NULL;
            temp.cstr = commands ;
            temp.type = 'p' ;
            temp.num = 0 ;
            command_list.push_back(temp);
            command_num = command_num + 1;
            i = i + 1;
        } // else if 
        else if (is_pipe(input[i], pipe_num, mark) == true && mark == false && pipe_num > 0) {
            commands[j] = NULL;
            temp.cstr = commands ;
            temp.type = 'n' ;
            temp.num = pipe_num ;
            command_list.push_back(temp);
            command_num = command_num + 1;
            i = i + 1;
        } // else if
        else if (is_pipe(input[i], pipe_num, mark) == true && mark == true && pipe_num > 0) {
            commands[j] = NULL;
            temp.cstr = commands ;
            temp.type = 'e' ;
            temp.num = pipe_num ;
            command_list.push_back(temp);
            command_num = command_num + 1;
            i = i + 1;
        } // else if
        else if ( is_rediration(input[i]) == true ) {
          i = i + 1 ; // let ">" get the fuckoff
          if ( input[i] != NULL ) {
            commands[j] = input[i] ;
            commands[j+1] = NULL ;
            temp.cstr = commands ;
            temp.type = 'f' ;
            temp.num = 0 ;
            command_list.push_back(temp);
            command_num = command_num + 1 ;
            i = i + 1 ;
          } // if 
          else 
            fprintf( stderr, "input[i] is NULL!" ) ; 
        } // else if 
        else if ( is_userpipe(input[i], user_id) == true ) {
            i = i + 1 ;
            if ( input[i] != NULL ) {
              commands[j] = input[i] ;
              commands[j+1] = NULL ;
              i = i + 1;  
            } // if   
            else 
              commands[j] = NULL;
            temp.cstr = commands ;
            temp.type = 'u' ;
            temp.num = user_id ;
            command_list.push_back(temp);
            command_num = command_num + 1;
        } // else if 
        
        j = 0;
        commands = (char**)malloc(sizeof(char*) * 8);
    } // while

    execute() ;
    while ( k < command_list.size() ) {
      char ** to_del = command_list[k].cstr ;
      free(to_del) ; 
      k = k + 1 ;
    } // while

    free(commands) ;
    command_list.clear();

} // depatcher()

void execute(){
  int pipenum = 0 ;
  int i = 0 ;
  int j = 0 ;
  pid_t pid  ;
  bool newpipe = true ;
  fd_node temp ;
  int totalnum = command_list.size() ;
  while ( i < totalnum ) {
    if ( command_list[i].type == 'p' && i != totalnum - 1 )
      pipenum = pipenum + 1 ;
    i = i + 1 ;
  } // while 
  int redir_fd = -1 ;
  int pipe_count = 0 ;
  int current_num = 0 ;
  int nullfd ;
  int pipes[ pipenum * 2 ] ;
  int limit_num = 0 ;
  int userpipes[2] ; // for userpipe
  int numpipes[2] ; // for numberpipe
  int errornum = 0 ; // for error_handle()
  int temp_errornum ; // for userpipe_command 
  int sendnum = 0 ;
  signal( SIGCHLD, sigchld_handler) ; // its just like turn on interrupt

  while ( current_num < totalnum ) {
    errornum = 0 ;
    newpipe = tonewpipe(current_num) ;
    if ( pid != -1 && current_num != 0 && (command_list[current_num-1].type == 'n' 
                              || command_list[current_num-1].type == 'e') )
      adjust_fd_list() ;

    if ( pid != -1 && current_num != totalnum - 1 && newpipe == true ) { // to new a pipe for normal pipe
      if ( pipe( pipes + pipe_count*2 ) < 0 )
        fprintf( stderr, "pipe error! number:%d\n", pipe_count ) ; 
    } // if
    
    if ( pid != -1 && (command_list[current_num].type == 'n' || command_list[current_num].type == 'e')){ // to new a pipe for numberpipe 
      if ( tonew_numberpipe(command_list[current_num].num, numpipes[0], numpipes[1]) == true ) {
        if ( pipe(numpipes) < 0 ) 
          fprintf( stderr, "numpipe error! command_number:%d\n", current_num ) ;
        temp.readnum = numpipes[0] ;
        temp.writenum = numpipes[1] ;
        temp.count = command_list[current_num].num ;
        user_list[user_index].fd_list.push_back(temp) ;
      } // if
    } // if

    if ( pid != -1 && command_list[current_num].type == 'u' ) { // to new a pipe for user pipe  
      if ( check_userpipe( command_list[current_num].num, errornum ) == true ) {
        if ( pipe(userpipes) < 0 ) 
          fprintf( stderr, "userpipes error! command_number:%d\n", current_num ) ;
        userpipe_node temp2 ;
        temp2.readfd = userpipes[0] ;
        temp2.writefd = userpipes[1] ;  
        temp2.send_id = user_list[user_index].id ;
        temp2.receive_id = command_list[current_num].num ;
        userpipe_list.push_back(temp2) ;
      } // if
    } // if 
    
    pid = fork() ;
    if ( pid == -1 ) {
      wait(NULL) ;
    } // if 
    else if ( pid == 0 ) { // childprocess
      if ( command_list[current_num].type == 's' ) { // SINGLE WORK
        if ( get_userpipe( command_list[current_num].cstr, errornum, current_num, sendnum ) == true )
          ; //get_userpipe( command_list[current_num].cstr, errornum, current_num ) ;
        else if (current_num == 0) {
          checkfdlist() ;
        } // else if 
        else if (command_list[current_num-1].type == 'p' ) {
          close_numberpipe() ;
          dup2( pipes[(pipenum-1)*2], STDIN_FILENO ) ;
          limit_num = pipe_count * 2 ; 
          i = 0 ;
          while ( i < limit_num ) {
            close(pipes[i]) ; 
            i = i + 1 ;
          } // while 
        } // else if 
        else if (command_list[current_num-1].type == 'n'|| command_list[current_num-1].type == 'e' ) {
          checkfdlist() ;
        } // else if
        
        if ( errornum > 0 ) {
          error_handler( current_num, errornum, sendnum, false ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          simpletask(current_num) ;
        } // if
        else 
          simpletask(current_num) ; 
      } // if
      else if ( command_list[current_num].type == 'n' || command_list[current_num].type == 'e' ) { // NUMBER PIPE
        if ( get_userpipe( command_list[current_num].cstr, errornum, current_num, sendnum ) == true )
          ;
        else if ( current_num - 1 >= 0 && command_list[current_num-1].type == 'p' ) { // before command is normal pipe
          int np_pipein = ( pipe_count-1 ) * 2 ;
          dup2( pipes[np_pipein], STDIN_FILENO) ;
          close(pipes[np_pipein]) ;
          close(pipes[np_pipein+1]) ;
        } // else if
        else {// before command is numberpipe or its is first command
          checkfdlist_numberpipe() ;
        } // else
        
        if ( errornum > 0 ) {
          error_handler( current_num, errornum, sendnum, false ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          numberpipe(numpipes[0], numpipes[1], current_num) ;
        } // if
        else 
          numberpipe(numpipes[0], numpipes[1], current_num) ;
      } // else if 
      else if ( command_list[current_num].type == 'p') { // NORMAL PIPE
        get_userpipe( command_list[current_num].cstr, errornum, current_num, sendnum ) ;
        if ( pipe_count == 0 ) { // first time normal pipe
          checkfdlist() ;
          dup2( pipes[1], STDOUT_FILENO ) ;
        } // if    
        else if ( pipe_count != 0 && command_list[current_num-1].type == 'p') { // before command is normal pipe
          close_numberpipe() ;
          int pipein = ( pipe_count-1 ) * 2 ;
          int pipeout = pipein + 3 ;
          dup2( pipes[pipein], STDIN_FILENO ) ;
          dup2( pipes[pipeout], STDOUT_FILENO ) ;
        } // else if
        else if ( pipe_count != 0 && (command_list[current_num-1].type == 'n' || command_list[current_num-1].type == 'e')) { // before command is number pipe 
          checkfdlist() ;
          dup2( pipes[(pipe_count*2)+1], STDOUT_FILENO ) ;
        } // else if

        i = 0 ;
        limit_num = (pipe_count+1) * 2 ; // to close new create pipe  
        while ( i < limit_num ) {
          close(pipes[i]) ; 
          i = i + 1 ;
        } // while 

        if ( errornum > 0 ) {
          error_handler( current_num, errornum, sendnum, false ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          simpletask(current_num) ;
        } // if
        else 
          simpletask(current_num) ;  
      } // else if
      else if ( command_list[current_num].type == 'f' ) { // FILE REDIRATION
        j = 0 ;
        while ( command_list[current_num].cstr[j] != NULL )
          j = j + 1 ;
        // this line just for good looking
        redir_fd = open( command_list[current_num].cstr[j-1], O_TRUNC | O_CREAT | O_WRONLY, 0644 ) ;
        command_list[current_num].cstr[j-1] = NULL ;
        if ( get_userpipe( command_list[current_num].cstr, errornum, current_num, sendnum ) == true )
          ;
        else if ( current_num == 0 ) {
          checkfdlist() ;
          dup2( redir_fd, STDOUT_FILENO ) ;
        } // else if   
        else if (command_list[current_num-1].type == 'p') { // means it is followed pipe
          int redir_pipein = ( pipe_count-1 ) * 2 ;
          dup2( pipes[redir_pipein], STDIN_FILENO ) ;
          dup2( redir_fd, STDOUT_FILENO ) ;
          limit_num = pipe_count * 2 ; 
          i = 0 ;
          while ( i < limit_num ) {
            close(pipes[i]) ; 
            i = i + 1 ;
          } // while 
        } // else if 
        else if ( command_list[current_num-1].type == 'n'|| command_list[current_num-1].type == 'e') { // means it is followed number pipe
          checkfdlist() ;
          dup2( redir_fd, STDOUT_FILENO ) ;
        } // else if 

        close(redir_fd) ;
        if ( errornum > 0 ) {
          error_handler( current_num, errornum, sendnum, false ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          simpletask(current_num) ;
        } // if
        else 
          simpletask(current_num) ;
      } // else if
      else if ( command_list[current_num].type == 'u' ) { // USER PIPE
        temp_errornum = errornum ;
        errornum = 0 ;
        if ( get_userpipe( command_list[current_num].cstr, errornum, current_num, sendnum ) == true ) 
          ;
        else if ( current_num > 0 && command_list[current_num-1].type == 'p' ) { // before command is normal pipe
          int up_pipein = ( pipe_count-1 ) * 2 ;
          dup2( pipes[up_pipein], STDIN_FILENO) ;
          close(pipes[up_pipein]) ;
          close(pipes[up_pipein+1]) ;
        } // else if
        else // before is numberpipe or first command 
          checkfdlist_numberpipe() ; // it seems numberpipe is work somehow sme with user pipe 
        
        if ( errornum > 0 ) { // for input userpipe error(<)
          error_handler( current_num, errornum, sendnum, false ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
        } // if
        errornum = temp_errornum ;
        if ( errornum > 0 ) { // for output userpipe error(>)
          error_handler( current_num, errornum, sendnum, true ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDOUT_FILENO ) ;
          close(nullfd) ;
          simpletask(current_num) ;
        } // if
        else 
          userpipe(userpipes[0], userpipes[1], current_num) ;    
      } // else if 
    } // else if
    else { // parentprocess
      maintain_fd_list() ;
      parrent_adjust_userpipe( command_list[current_num].cstr, current_num ) ;
      if ( errornum == 0 && command_list[current_num].type == 'u' )
        broadcast_userpipe_success(command_list[current_num].num) ; 
      if ( current_num != 0 && command_list[current_num-1].type == 'p' ) {
        close(pipes[(pipe_count-1)*2]) ;
        close(pipes[(pipe_count-1)*2 + 1 ]) ;
      } // if
    } // else 
    
    if ( pid != -1 && newpipe == true )
      pipe_count = pipe_count + 1 ;
    if ( pid != -1 )
      current_num = current_num + 1 ;
  } // while

  if ( command_list[current_num-1].type == 'n' || command_list[current_num-1].type == 'e'  ) {
    percent = false ;
  } // if   
  else {
    waitpid(pid, NULL, 0) ;
    percent = true ;
  } // else   
} // execute()

void error_handler( int current, int error_num, int sendnum, bool is_send ) {
  char* info = (char*)malloc(sizeof(char)*4000) ;
  if ( error_num == 1 ) { // pipe already exist
    sprintf( info, "*** Error: the pipe #%d->#%d already exists. ***\n", user_list[user_index].id, command_list[current].num ) ;
    send( user_list[user_index].user_fd, info, strlen(info), 0 ) ;
  } // if 
  else if ( error_num == 2 ) { // the pipes does not exist
    sprintf( info, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", sendnum, user_list[user_index].id ) ;
    send( user_list[user_index].user_fd, info, strlen(info), 0 ) ;
  } // else if 
  else if ( error_num == 3 ) { // sender or receiver does not exist
    if ( is_send == false )
      sprintf( info, "*** Error: user #%d does not exist yet. ***\n", sendnum ) ;
    else 
      sprintf( info, "*** Error: user #%d does not exist yet. ***\n", command_list[current].num ) ;
    send( user_list[user_index].user_fd, info, strlen(info), 0 ) ; 
  } // else if 

  free(info) ;
} // error_handler()

void userpipe(int read_end, int write_end, int current) {
  dup2( write_end, STDOUT_FILENO ) ;
  close(read_end) ;
  close(write_end) ;
  close_numberpipe() ;
  simpletask(current) ;
} // userpipe()

bool get_userpipe( char** command, int &error_num, int current, int &sendnum ) {
  int i = 0 ;
  const char* del = "<" ;
  char temp[256] ;
  while ( command[i] != NULL ) 
    i = i + 1 ;
  strcpy( temp, command[i-1]) ;
  if ( temp[0] == '<' ) {
    char* token = strtok( command[i-1], del);
    if ( token != NULL ) {
      int send_id = atoi(token);
      sendnum = send_id ;
      command[i-1] = NULL ;
      if ( useid_find_user(send_id) == -1 ) 
        error_num = 3 ;
      else if ( find_userpipe(send_id) == true )
        return true ;
      else
        error_num = 2 ;
      return true ;   
    } // if 
  } // if 
  else 
    return false ;

  return false ;
} // get_userpipe

bool find_userpipe( int sendid ) {
  bool exist = false ;
  for( int i = 0 ; i < userpipe_list.size(); i++ ) {
    if ( sendid == userpipe_list[i].send_id && user_list[user_index].id == userpipe_list[i].receive_id ) {
      //broadcast_receive_success( sendid ) ;
      dup2( userpipe_list[i].readfd, STDIN_FILENO ) ;
      close( userpipe_list[i].readfd ) ;
      close( userpipe_list[i].writefd ) ;
      userpipe_list.erase(userpipe_list.begin()+i) ;
      exist = true ;
      return exist ;
    } // if 
  } // for

  return exist ;
} // find_userpipe()

void parrent_adjust_userpipe( char** command, int current ) {
  int i = 0 ;
  char temp[256] ;
  const char* del = "<" ;
  while ( command[i] != NULL )
    i = i + 1 ;
  if ( command_list[current].type == 'f' ) 
    strcpy(temp, command[i-2]) ;
  else 
    strcpy(temp, command[i-1]) ;
  if ( temp[0] == '<' ) {
    char* token = strtok( temp, del);
    if ( token != NULL ) {
      int id = atoi(token);
      parrent_check_userpipetable(id) ;
    } // if 
  } // if
} // parrent_adjust_userpipe()

void parrent_check_userpipetable( int send_id ) {
  for ( int i = 0 ; i < userpipe_list.size() ; i++ ) {
    if ( send_id == userpipe_list[i].send_id && user_list[user_index].id == userpipe_list[i].receive_id ) {
      broadcast_receive_success( send_id ) ;
      close(userpipe_list[i].readfd) ;
      close(userpipe_list[i].writefd) ;
      userpipe_list.erase(userpipe_list.begin()+i) ;
      break ; 
    } // if 
  } // for 
} // parrent_check_userpipetable()

void numberpipe(int read_end, int write_end, int current) {
  dup2( write_end, STDOUT_FILENO) ;
  if (command_list[current].type == 'e') {
    dup2( write_end, STDERR_FILENO) ;
  } // if
  close(read_end) ;
  close(write_end) ;
  close_numberpipe() ;
  simpletask(current) ;

} //numberpipe()

void checkfdlist() {
  int size = user_list[user_index].fd_list.size() ;
  int i = 0 ;
  while ( i < size ) {
    if ( user_list[user_index].fd_list[i].count == 0 ) {
      dup2(user_list[user_index].fd_list[i].readnum, STDIN_FILENO) ;
      close(user_list[user_index].fd_list[i].readnum) ;
      close(user_list[user_index].fd_list[i].writenum) ;
      user_list[user_index].fd_list.erase(user_list[user_index].fd_list.begin()+i) ;
      break ;
    } //if 
    i = i + 1 ;
  } // while 

  close_numberpipe() ;
} // checkfdlist()

void checkfdlist_numberpipe() {
  int size = user_list[user_index].fd_list.size() ;
  int i = 0 ;
  while ( i < size ) {
    if ( user_list[user_index].fd_list[i].count == 0 ) {
      dup2(user_list[user_index].fd_list[i].readnum, STDIN_FILENO) ;
      close(user_list[user_index].fd_list[i].readnum) ;
      close(user_list[user_index].fd_list[i].writenum) ;
      user_list[user_index].fd_list.erase(user_list[user_index].fd_list.begin()+i) ;
      break ;
    } //if 
    i = i + 1 ;
  } // while 
} //checkfdlist_numberpipe()

void close_numberpipe() {
  int i = 0 ;
  while( i < user_list[user_index].fd_list.size() ) {
    close(user_list[user_index].fd_list[i].readnum) ;
    close(user_list[user_index].fd_list[i].writenum) ; 
    i = i + 1;
  } // while

} // close_numberpipe()

void adjust_fd_list() {
  int i = 0 ;
  while ( i < user_list[user_index].fd_list.size() ) {
    user_list[user_index].fd_list[i].count = user_list[user_index].fd_list[i].count - 1 ;
    i = i + 1 ;
  } // while

} // adjust_fd_list()

void maintain_fd_list() {
  int i = 0 ;
  while ( i < user_list[user_index].fd_list.size() ) {
    if ( user_list[user_index].fd_list[i].count == 0 ) {
      close(user_list[user_index].fd_list[i].readnum) ;
      close(user_list[user_index].fd_list[i].writenum) ;
      user_list[user_index].fd_list.erase(user_list[user_index].fd_list.begin()+i) ;
      break ;
    } // if 
    i = i + 1 ;
  } // while 

} //maintain_fd_list()

bool is_pipe(char* str, int &pipenum, bool &is_mark) {
    char temp[256];
    const char* del = "!|";
    bool ans = false;
    is_mark = false ;
    pipenum = 0;
    strcpy(temp, str);
    if (temp[0] == '|' || temp[0] == '!') {
      if (temp[0] == '!')
        is_mark = true ;
      ans = true;
      char* token = strtok(str, del);
      if ( token != NULL )
        pipenum = atoi(token);
    } // if 

    return ans;
    
} // is_pipe()

bool tonew_numberpipe( int count, int &read, int &write ){
  bool ans = true ;
  int i = 0 ;
  read = -1 ;
  write = -1 ;
  while ( i < user_list[user_index].fd_list.size() ) {
    if (user_list[user_index].fd_list[i].count == count ) {
      ans = false ;
      read = user_list[user_index].fd_list[i].readnum ;
      write = user_list[user_index].fd_list[i].writenum ; 
      break ;
    } // if   
    i = i + 1 ;
  } // while 

  return ans ;
} // tonew_numberpipe()

bool is_rediration( char* str){
  if ( str != NULL ) {
    if ( strcmp( str, ">") == 0 )
      return true ;
  } // if 
  else 
    fprintf( stderr, "is_refiration error str is NULL!" ) ;

  return false ;
} // is_rediration()

bool is_userpipe( char* str, int &id ) {
  char temp[256] ;
  bool ans = false ;
  const char* del = ">";
  if ( str != NULL ) {
    strcpy(temp, str) ;
    if ( temp[0] == '>') {
      char* token = strtok(str, del);
      if ( token != NULL ) {
        id = atoi(token);
        ans = true ;
      } // if
      else 
        fprintf( stderr, "is_userpipe\n!" ) ;     
    } // if 
  } // if

  return ans ; 

} // is_userpipe()

bool tonewpipe( int current) {
  if ( command_list[current].type == 'f' || command_list[current].type == 'n' 
       || command_list[current].type == 's' || command_list[current].type == 'e' )
    return false ;

  return true ;
} // tonewpipe()

int is_build_in( char * str ) {
  if ( str ==  NULL ) {
    fprintf( stderr, "is_build_in()" ) ;  
    return -1 ;
  } // if   
  else if ( strcmp( str, "setenv") == 0 )
    return 1 ; // 1 means setenv
  else if ( strcmp( str, "printenv") == 0 )
    return 2 ; // 2 means printenv
  else if ( strcmp( str, "exit") == 0 )
    return 3 ; // 3 means exit
  else if ( strcmp( str, "who") == 0 )
    return 4 ; // 4 means who
  else if ( strcmp( str, "tell") == 0 )
    return 5 ; // 5 means tell
  else if ( strcmp( str, "yell") == 0 )
    return 6 ; // 6 means yell  
  else if ( strcmp( str, "name") == 0 )
    return 7 ; // 7 means name  
  else
    return 0 ; 

} // is_buid_in()

void simpletask(int current) {
  if ( execvp( command_list[current].cstr[0], command_list[current].cstr) == -1 ) {
    fprintf( stderr, "Unknown command: [%s].\n", command_list[current].cstr[0] ) ;
    exit(0) ;
  } // if 
} //simpletask()

void sigchld_handler( int sig ) { //for zombie process
  wait(NULL) ;
} //sigchld_handler()

void Setenv(char * var, char * value ) {
  setenv( var, value, 1 ) ;
  int index = user_list[user_index].var_index ;
  strcpy( user_list[user_index]._var[index], var ) ;
  strcpy( user_list[user_index]._value[index], value ) ;
  user_list[user_index].var_index = user_list[user_index].var_index  + 1 ;
} // setenv()

void printenv( char * var ) {
  char * str = getenv( var ) ;
  if ( str != NULL )
    printf( "%s\n", str ) ; 
} // printenv()

bool check_userpipe( int id, int &error_num ) {
  bool exist = false ;
  for ( int j = 0 ; j < user_list.size() ; j++ ) {
    if ( id == user_list[j].id ) {
      exist = true ;
      break ;
    } // if   
  } // for

  if ( exist == false ) {
    error_num = 3 ;
    return false ;
  } // if   
  int i = 0 ;
  while ( i < userpipe_list.size() ) {
    if ( user_list[user_index].id == userpipe_list[i].send_id && id == userpipe_list[i].receive_id ) {
      error_num = 1 ;
      return false ;
    } // if   
    i = i + 1 ;
  } // while 

  return true ;
} // check_userpipe()

void broadcast_userpipe_success( int id ) {
  char info[15085] ;
  int receive_index = useid_find_user(id) ;
  sprintf( info, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", user_list[user_index].name,
           user_list[user_index].id, line_input, user_list[receive_index].name, id ) ;
  int i = 0 ;
  while ( i < user_list.size() ) {
    send( user_list[i].user_fd, info, strlen(info), 0 ) ;
    i = i + 1 ;
  } // while 

} // broadcast_userpipe_succcess()

void broadcast_receive_success( int id ) {
  char info[15085] ;
  int send_index = useid_find_user(id) ;
  sprintf( info, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", user_list[user_index].name,
           user_list[user_index].id, user_list[send_index].name, id, line_input ) ;
  int i = 0 ;
  while ( i < user_list.size() ) {
    send( user_list[i].user_fd, info, strlen(info), 0 ) ;
    i = i + 1 ;
  } // while 
  
} // broadcast_userpipe_success()


