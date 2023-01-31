#pragma once

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

void npshell();
void Setenv(char * var, char * value) ;
void printenv( char * var) ;
void init_env() ;
void parser(char* linestr);
bool is_pipe(char* str, int &pipenum, bool &is_mark) ;
bool is_rediration( char* str) ;
bool tonewpipe( int current) ;
int is_build_in( char * str ) ;
void depatcher(char** input);
void simpletask(int current);
void sigchld_handler( int sig ) ;
void execute() ;
void numberpipe(int read_end, int write_end, int current) ;
void checkfdlist() ;
void checkfdlist_numberpipe() ;
bool tonew_numberpipe( int count, int &read, int &write ) ;
void close_numberpipe() ;
void adjust_fd_list() ;
void maintain_fd_list() ; // for parent

extern std::vector<command_node>command_list;
extern std::vector<fd_node>fd_list ;

