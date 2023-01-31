#pragma once
# include <iostream> // cout, endl
# include <string> // string, find_last_of, substr
# include <semaphore.h>
struct command_node{
    char ** cstr ;
    char type ; 
    int num ;
};

struct fd_node{
  int readnum ;
  int writenum ;
  int count ;
};

struct fifo_node{
  int fd ;
  int sender ;
  char file_name[20] ;
};

typedef struct {
  int id ;
  int fd ;
  char name[20] ;
  char ip[40] ;
  int port ;
  bool valid ;
  pid_t pid ;
} user_info;

typedef struct {
  int sendid ;
  int receiveid ;
  char file_name[20] ; 
  bool valid ;
  bool _open ;
} userpipe_info;


void npshell();
void Setenv(char * var, char * value) ;
void printenv( char * var) ;
void init_env() ;
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
void checkfdlist() ;
void checkfdlist_numberpipe() ;
bool check_userpipe( int id, int &error ) ;
bool tonew_numberpipe( int count, int &read, int &write ) ;
void close_numberpipe() ;
void adjust_fd_list() ;
void maintain_fd_list() ; // for parent
void userpipe( int current, int fd ) ;
bool receive_userpipe( char** command, int &error_num, int &sendnum ) ;
void receive_userpipe_parent( char** command ) ;
bool check_user_exist( int id ) ;
bool find_receive_list( int id, int &index ) ;
bool find_receive_list_parent( int id, int &index ) ;
int get_userpipe_index() ;
void clean_userpipe_ptr( int sendid, int receiveid ) ;
pid_t get_pid( int id ) ;
void broadcast_pipe_out( int send, int receive ) ;
void broadcast_pipe_in( int send, int receive ) ;
void error_handler( int error_num, int current, bool _send, int sendnum ) ;
void clean_up() ;

extern std::vector<command_node>command_list;
extern std::vector<fd_node>fd_list ;
extern std::vector<fifo_node>receive_list ;
extern char* message ;
extern userpipe_info* userpipe_ptr ;
extern user_info* user ;
extern int userlist_index ;
extern std::string c_line ;
extern sem_t *message_sem ; // semaphore_for_message

