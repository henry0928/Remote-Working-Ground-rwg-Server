//#define _CRT_SECURE_NO_WARNINGS
# include <stdio.h>
# include <string.h> // string, find_last_of, substr
# include <vector> // vector, push_back
# include <stdlib.h> // setenv() getenv()
# include <cstdlib> // system, atoi
# include <ctype.h> // isdigit() 
# include <cstdio> // system, atoi
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "npshellforserver3.h"

using namespace std;

void init_env() {
  setenv( "PATH", "bin:.", 1 ) ;
} // init_env()

void npshell() {
    init_env() ;
    int status = 0;
    int i = 0 ;
    int size = 0 ;
    char* line = (char*)malloc(sizeof(char) * 15001);
    command_list.clear() ;
    fd_list.clear() ;
    while (1) {
        cout << "% ";
        if ( fgets( line, 15001, stdin ) != NULL ) {
          if ( strcmp( line, "\n") != 0 ){
            adjust_fd_list() ;
            parser(line);
          } // if   
        } // if   
        else 
          break;
        free(line) ;
        line = (char*)malloc(sizeof(char) * 15001);
    } // while
    
    free(line) ;
} //npshell()

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
    if ( is_build_in(argv[0]) == 1 )
      Setenv( argv[1], argv[2] ) ;
    else if ( is_build_in(argv[0]) == 2 )
      printenv( argv[1] ) ;
    else if ( is_build_in(argv[0]) == 3 ) {
      free(argv) ;
      return 0 ;
    } // else if   
    else if ( is_build_in(argv[0]) == 0 ) 
      depatcher(argv);   
    else
      return 0 ;
    free(argv) ;

    return 1 ;

} // parser

void depatcher(char** input) {
    int i = 0 ;
    int j = 0;
    int k = 0 ;
    int pipe_num = 0 ;
    int command_num = 0;
    int user_id = 0 ;
    int singlepipe_num = 0;
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
  char* file = (char*)malloc(sizeof(char)*20) ;
  int pipenum = 0 ;
  int i = 0 ;
  int j = 0 ;
  pid_t pid  ;
  bool newpipe = true ;
  bool has_open_userpipe = false ;
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
  int pipes[ pipenum * 2 ] ;
  int limit_num = 0 ;
  int numpipes[2] ; // for numberpipe
  int userpipe_fd ; // for userpipe
  int close_userpipe_fd = -1 ;
  int up_index ;
  int errno_num_pipeout = 0 ;
  int errno_num_pipein = 0 ;
  int sendnum ;
  int nullfd ;

  signal( SIGCHLD, sigchld_handler) ; // its just like turn on interrupt

  while ( current_num < totalnum ) {
    errno_num_pipeout = 0 ;
    errno_num_pipein = 0 ;
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
        fd_list.push_back(temp) ;
      } // if
    } // if

    if ( pid != -1 && command_list[current_num].type == 'u' ) {
      if ( check_userpipe( command_list[current_num].num, errno_num_pipeout ) == true ) {
        memset( file, 0, 20 ) ;
        sprintf( file, "user_pipe/%d_%d", user[userlist_index].id, command_list[current_num].num) ;
        mkfifo( file, S_IFIFO | 0666 ) ;
        userpipe_fd = open( file, O_RDWR ) ;
        up_index = get_userpipe_index() ;
        if ( up_index >= 0 ) {
          userpipe_ptr[up_index].sendid = user[userlist_index].id ;
          userpipe_ptr[up_index].receiveid = command_list[current_num].num ;
          memset( userpipe_ptr[up_index].file_name, 0, 20 ) ;
          strcpy( userpipe_ptr[up_index].file_name, file ) ;
          userpipe_ptr[up_index].valid = true ;
          userpipe_ptr[up_index]._open = true ;     
          kill( get_pid( command_list[current_num].num ), SIGUSR2 ) ; // let receive know to open fifo
        } // if
        else 
          cerr << "fuckyou there is fucking error!\n" ;
      } // if
    } // if

    pid = fork() ;
    if ( pid == -1 ) {
      wait(NULL) ;
    } // if 
    else if ( pid == 0 ) { // childprocess
      if ( command_list[current_num].type == 's' ) { // SINGLE WORK
        receive_userpipe( command_list[current_num].cstr, errno_num_pipein, sendnum ) ;
        if (current_num == 0) {
          checkfdlist() ;
        } // if 
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
        
        if ( errno_num_pipein > 0 ) {
          error_handler( errno_num_pipein, current_num, false, sendnum ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          clean_up() ;
          simpletask(current_num) ;
        } // if
        else {
          clean_up() ;
          simpletask(current_num) ;
        } // else    
      } // if
      else if ( command_list[current_num].type == 'n' || command_list[current_num].type == 'e' ) { // NUMBER PIPE
        receive_userpipe( command_list[current_num].cstr, errno_num_pipein, sendnum ) ;
        if ( current_num - 1 >= 0 && command_list[current_num-1].type == 'p' ) { // before command is normal pipe
          int np_pipein = ( pipe_count-1 ) * 2 ;
          dup2( pipes[np_pipein], STDIN_FILENO) ;
          close(pipes[np_pipein]) ;
          close(pipes[np_pipein+1]) ;
        } // if
        else {// before command is numberpipe or its is first command
          checkfdlist_numberpipe() ;
        } // else
        
        if ( errno_num_pipein > 0 ) {
          error_handler( errno_num_pipein, current_num, false, sendnum ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          clean_up() ;
          numberpipe(numpipes[0], numpipes[1], current_num) ;
        } // if
        else {
          clean_up() ;
          numberpipe(numpipes[0], numpipes[1], current_num) ;
        } // else   
      } // else if 
      else if ( command_list[current_num].type == 'p') { // NORMAL PIPE
        receive_userpipe( command_list[current_num].cstr, errno_num_pipein, sendnum ) ;
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

        if ( errno_num_pipein > 0 ) {
          error_handler( errno_num_pipein, current_num, false, sendnum ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          clean_up() ;
          simpletask(current_num) ;
        } // if
        else {
          clean_up() ;
          simpletask(current_num) ;
        } // else      
      } // else if
      else if ( command_list[current_num].type == 'f' ) { // FILE REDIRATION
        j = 0 ;
        while ( command_list[current_num].cstr[j] != NULL )
          j = j + 1 ;
        // this line just for good looking
        redir_fd = open( command_list[current_num].cstr[j-1], O_TRUNC | O_CREAT | O_WRONLY, 0644 ) ;
        command_list[current_num].cstr[j-1] = NULL ;
        receive_userpipe( command_list[current_num].cstr, errno_num_pipein, sendnum ) ;
        if ( current_num == 0 ) {
          checkfdlist() ;
          dup2( redir_fd, STDOUT_FILENO ) ;
        } // if   
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
        if ( errno_num_pipein > 0 ) {
          error_handler( errno_num_pipein, current_num, false, sendnum ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
          clean_up() ;
          simpletask(current_num) ;
        } // if
        else {
          clean_up() ;
          simpletask(current_num) ;
        } // else      
      } // else if
      else if ( command_list[current_num].type == 'u' ) { // USER PIPE
        receive_userpipe( command_list[current_num].cstr, errno_num_pipein, sendnum ) ;
        if ( current_num > 0 && command_list[current_num-1].type == 'p' ) { // before command is normal pipe
          int up_pipein = ( pipe_count-1 ) * 2 ;
          dup2( pipes[up_pipein], STDIN_FILENO) ;
          close(pipes[up_pipein]) ;
          close(pipes[up_pipein+1]) ;
        } // if
        else // before is numberpipe or first command 
          checkfdlist_numberpipe() ; // it seems numberpipe is work somehow sme with user pipe 
        
        if ( errno_num_pipein > 0 ) {
          error_handler( errno_num_pipein, current_num, false, sendnum ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDIN_FILENO ) ;
          close(nullfd) ;
        } // if

        if ( errno_num_pipeout > 0 ) {
          error_handler( errno_num_pipeout, current_num, true, sendnum ) ;
          nullfd = open( "/dev/null", O_RDWR ) ;
          dup2( nullfd, STDOUT_FILENO ) ;
          close(nullfd) ;
          clean_up() ;
          simpletask(current_num) ;
        } // if 
        else {
          clean_up() ;
          userpipe(current_num, userpipe_fd) ;
        } // else       
      } // else if
    } // else if
    else { // parentprocess
      maintain_fd_list() ;
      receive_userpipe_parent( command_list[current_num].cstr ) ;
      if ( command_list[current_num].type == 'u' && errno_num_pipeout == 0 ) {
        close(userpipe_fd) ;
      } // if  
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

  if ( command_list[current_num-1].type == 'n' || command_list[current_num-1].type == 'e'  )
    ;
  else 
    waitpid(pid, NULL, 0) ;

  free(file) ;
} // execute()

void clean_up() {
  int i = 0 ;
  while ( i < receive_list.size() ) {
    close( receive_list[i].fd ) ;
    i = i + 1 ;
  } // while 
} //clean_up 

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

void userpipe( int current, int fd ) {
  dup2( fd, STDOUT_FILENO ) ;
  close(fd) ;
  close_numberpipe() ;
  broadcast_pipe_out( user[userlist_index].id, command_list[current].num ) ;
  simpletask(current) ;
} // userpipe()

void broadcast_pipe_out( int send, int receive ) {
  char info[15085] ;
  sprintf( info, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", user[userlist_index].name, send,
           c_line.c_str(), user[receive-1].name, receive ) ;
  memset( message, 0, 15085 ) ;
  strcpy( message, info ) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( user[i].valid == true )
      kill( user[i].pid, SIGUSR1 ) ;
  } // for
} // broadcast_pipe_out()

void broadcast_pipe_in( int send, int receive ) {
  char info[15085] ;
  sprintf( info, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", user[userlist_index].name, receive,
           user[send-1].name, send, c_line.c_str() ) ;
  memset( message, 0, 15085 ) ;
  strcpy( message, info ) ;
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( user[i].valid == true ) 
      kill( user[i].pid, SIGUSR1 ) ;        
  } // for
} // broadcast_pipe_in()

bool receive_userpipe( char** command, int &error_num, int &sendnum ) {
  int i = 0 ;
  const char* del = "<" ;
  char temp[256] ;
  int remove_index ; 
  while ( command[i] != NULL ) 
    i = i + 1 ;
  strcpy( temp, command[i-1]) ;
  if ( temp[0] == '<' ) {
    char* token = strtok( command[i-1], del);
    if ( token != NULL ) {
      int send_id = atoi(token);
      sendnum = send_id ;
      command[i-1] = NULL ;
      if ( check_user_exist(send_id) == false ) 
        error_num = 3 ;
      else if ( find_receive_list(send_id, remove_index) == true ) {
        receive_list.erase(receive_list.begin()+remove_index) ;
        return true ;
      } // else if   
      else
        error_num = 2 ;
      return true ;   
    } // if 
  } // if 
  else 
    return false ;

  return false ;
} // receive_userpipe

void receive_userpipe_parent( char** command ) {
  int i = 0 ;
  const char* del = "<" ;
  char temp[256] ;
  int remove_index ; 
  while ( command[i] != NULL ) 
    i = i + 1 ;
  strcpy( temp, command[i-1]) ;
  if ( temp[0] == '<' ) {
    char* token = strtok( command[i-1], del);
    if ( token != NULL ) {
      int send_id = atoi(token);
      command[i-1] = NULL ;
      if ( check_user_exist(send_id) == false ) 
        ;
      else if ( find_receive_list_parent(send_id, remove_index) == true ) {
        broadcast_pipe_in( send_id, userlist_index+1 ) ;
        receive_list.erase(receive_list.begin()+remove_index) ;
        clean_userpipe_ptr( send_id, userlist_index+1 ) ;
      } // else if   
      else
        ;   
    } // if 
  } // if 

} // receive_userpipe_parent()

bool check_user_exist( int id ) {
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( user[i].valid == true ) {
      if ( user[i].id == id )
        return true ;
    } // if 
  } // for

  return false ; 
} // check_user_exist()

bool find_receive_list_parent( int id, int &index ) {
  int i = 0 ;
  while ( i < receive_list.size() ) {
    if ( receive_list[i].sender == id ) {
      index = i ;
      close( receive_list[i].fd ) ;
      return true ;
    } // if 

    i = i + 1 ;
  } // while 

  return false ;
} // find_receive_list_parent()

bool find_receive_list( int id, int &index ) {
  int i = 0 ;
  while ( i < receive_list.size() ) {
    if ( receive_list[i].sender == id ) {
      index = i ;
      dup2( receive_list[i].fd, STDIN_FILENO ) ;
      close( receive_list[i].fd ) ;
      unlink( receive_list[i].file_name ) ;
      return true ;
    } // if 

    i = i + 1 ;
  } // while 

  return false ;
} // find_receive_list()

void clean_userpipe_ptr( int sendid, int receiveid ) {
  for( int i = 0 ; i < 900 ; i++ ) {
    if ( userpipe_ptr[i].valid == true ) {
      if ( userpipe_ptr[i].sendid == sendid && userpipe_ptr[i].receiveid == receiveid ) {
        userpipe_ptr[i]._open = false ;
        userpipe_ptr[i].valid = false ;
        break ;
      } // if 
    } // if 
  } // for 

} // clean_userpipe_ptr()

void error_handler( int error_num, int current, bool _send, int sendnum ) {
  char* info = (char*)malloc(sizeof(char)*500) ;
  if ( error_num == 0 )
    ;
  else if ( error_num == 1 ) { // pipe already exist
    sprintf( info, "*** Error: the pipe #%d->#%d already exists. ***\n", user[userlist_index].id, command_list[current].num ) ;
    send( user[userlist_index].fd, info, strlen(info), 0 ) ;
  } // else if 
  else if ( error_num == 2 ) { // the pipes does not exist
    sprintf( info, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", sendnum, user[userlist_index].id ) ;
    send( user[userlist_index].fd, info, strlen(info), 0 ) ;
  } // else if 
  else if ( error_num == 3 ) { // sender or receiver does not exist
    if ( _send == true )
      sprintf( info, "*** Error: user #%d does not exist yet. ***\n", command_list[current].num ) ;
    else 
      sprintf( info, "*** Error: user #%d does not exist yet. ***\n", sendnum ) ;
    send( user[userlist_index].fd, info, strlen(info), 0 ) ; 
  } // else if 

  free(info) ;
} // error_handler 

int get_userpipe_index() {
  for( int i = 0 ; i < 900 ; i++ ) {
    if ( userpipe_ptr[i].valid == false )
      return i ;
  } // for 

  return -1 ;
} // get_userpipe_index() 

bool check_userpipe( int id, int &error ) {
  int j ; 
  for ( j = 0 ; j < 30 ; j++ ) {
    if ( user[j].valid == true ) {
      if ( user[j].id == id )
        break ;
    } // if 
  } // for 
 
  if ( j == 30 ) {
    error = 3 ;
    return false ;
  } // if 

  for ( int i = 0 ; i < 900 ; i++ ) {
    if ( userpipe_ptr[i].valid == true ) {
      if ( user[userlist_index].id == userpipe_ptr[i].sendid && id == userpipe_ptr[i].receiveid ) {
        error = 1 ;
        return false ; 
      } // if   
    } // if 
  } // for 

  return true ;
} //check_userpipe

void checkfdlist() {
  int size = fd_list.size() ;
  int i = 0 ;
  while ( i < size ) {
    if ( fd_list[i].count == 0 ) {
      dup2(fd_list[i].readnum, STDIN_FILENO) ;
      close(fd_list[i].readnum) ;
      close(fd_list[i].writenum) ;
      fd_list.erase(fd_list.begin()+i) ;
      break ;
    } //if 
    i = i + 1 ;
  } // while 

  close_numberpipe() ;
} // checkfdlist()

void checkfdlist_numberpipe() {
  int size = fd_list.size() ;
  int i = 0 ;
  while ( i < size ) {
    if ( fd_list[i].count == 0 ) {
      dup2(fd_list[i].readnum, STDIN_FILENO) ;
      close(fd_list[i].readnum) ;
      close(fd_list[i].writenum) ;
      fd_list.erase(fd_list.begin()+i) ;
      break ;
    } //if 
    i = i + 1 ;
  } // while 
} //checkfdlist_numberpipe()

void close_numberpipe() {
  int i = 0 ;
  while( i < fd_list.size() ) {
    close(fd_list[i].readnum) ;
    close(fd_list[i].writenum) ; 
    i = i + 1;
  } // while

} // close_numberpipe()

void adjust_fd_list() {
  int i = 0 ;
  while ( i < fd_list.size() ) {
    fd_list[i].count = fd_list[i].count - 1 ;
    i = i + 1 ;
  } // while

} // adjust_fd_list()

void maintain_fd_list() {
  int i = 0 ;
  while ( i < fd_list.size() ) {
    if ( fd_list[i].count == 0 ) {
      close(fd_list[i].readnum) ;
      close(fd_list[i].writenum) ;
      fd_list.erase(fd_list.begin()+i) ;
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
  while ( i < fd_list.size() ) {
    if (fd_list[i].count == count ) {
      ans = false ;
      read = fd_list[i].readnum ;
      write = fd_list[i].writenum ; 
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

pid_t get_pid( int id ) {
  for ( int i = 0 ; i < 30 ; i++ ) {
    if ( user[i].valid == true ) {
      if ( user[i].id == id )
        return user[i].pid ;
    } // if
  } // for 

  return -1 ;
} //get_pid()

int is_build_in( char * str ) {
  if ( str ==  NULL ) {
    fprintf( stderr, "is_build_in error str is NULL!" ) ;  
    return -1 ;
  } // if   
  else if ( strcmp( str, "setenv") == 0 )
    return 1 ; // 1 means setenv
  else if ( strcmp( str, "printenv") == 0 )
    return 2 ; // 2 means printenv
  else if ( strcmp( str, "exit") == 0 )
    return 3 ; // 3 means exit
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
} // setenv()

void printenv( char * var ) {
  char * str = getenv( var ) ;
  if ( str != NULL )
    printf( "%s\n", str ) ; 
} // printenv()


