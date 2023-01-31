#pragma once

struct command_node{
    char ** cstr ;
    char type ; 
    int num ;
};

// s for simple task 
// p for simple pipe 
// n for number pipe
// f for file_rediration
// e for (!)number pipe
// u for user pipe

struct fd_node{
  int readnum ;
  int writenum ;
  int count ;
};

struct userpipe_node{
  int readfd ;
  int writefd ;
  int send_id ;
  int receive_id ;
};

struct user_info{
  int id ;
  int user_fd ;
  char * name ;
  char * ip ;
  int port ;
  std::vector<fd_node>fd_list ;
  char** _var ;
  char** _value ;
  int var_index ;
} ;

void Setenv(char * var, char * value) ;
void printenv( char * var) ;
void init_env() ;
void who() ;
void name( char* name_str ) ;
bool checkname( char* str ) ;
void tell( char ** message ) ;
int useid_find_user( int id ) ;
bool checktell( int id ) ;
void yell( char ** message ) ;
int parser(char* linestr);
bool is_pipe(char* str, int &pipenum, bool &is_mark) ;
bool is_rediration( char* str) ;
bool is_userpipe( char* str, int &id ) ;
bool tonewpipe( int current) ;
int is_build_in( char * str ) ;
void depatcher(char** input);
void simpletask(int current);
void sigchld_handler( int sig ) ;
void execute() ;
void numberpipe(int read_end, int write_end, int current) ;
void userpipe(int read_end, int write_end, int current) ;
void checkfdlist() ;
void checkfdlist_numberpipe() ;
bool tonew_numberpipe( int count, int &read, int &write ) ;
void close_numberpipe() ;
void adjust_fd_list() ;
void maintain_fd_list() ; // for parent
bool check_userpipe( int id, int &error_num ) ;
void broadcast_userpipe_success( int id ) ;
void broadcast_receive_success( int id ) ;
bool get_userpipe( char** command, int &error_num, int current, int &sendnum ) ;
bool find_userpipe( int sendid ) ;
void parrent_adjust_userpipe( char** command, int current ) ;
void parrent_check_userpipetable( int send_id ) ;
void error_handler( int current, int error_num, int sendnum, bool is_send ) ;

extern std::vector<command_node>command_list;
extern std::vector<user_info>user_list ;
extern std::vector<userpipe_node>userpipe_list ;
extern char* line_input ;
extern int user_index ;
extern bool percent ;
