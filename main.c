#include "stdio.h"
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h> 

// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J")

void init_shell() {
    clear();  
    printf("\n=============gShell v.0.1=============");  
    char* username = getenv("USER"); 
    printf("\n\nLogged in as: @%s", username); 
    printf("\n"); 
    sleep(3); 
    clear();
}

void print_directory(){
    char cwd[1024];

    getcwd(cwd, sizeof(cwd));
    printf("\n%s> ", cwd);
}

char* get_user_input() {
    char *line;
    size_t buffer_size = 0;

    getline(&line, &buffer_size, stdin);
    printf("\nInput: %s", line);
    add_history(line);
    return line;
}

int main() 
{ 
    char *user_input;  
    init_shell(); 
  
    while (1) { 
        // print shell line 
        print_directory(); 
        // take input
        user_input = get_user_input();
        if (user_input != NULL && (*user_input) != '\0') 
            continue;

        // TODO:
        // Input string processing
        // Command execution
        // Handling built-in commands and pipes 
         
    } 
   
    return 0; 
} 