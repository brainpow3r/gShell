#include "stdio.h"
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h> 

// Clearing the shell using escape sequences 
#define MAX_COMMANDS 100		// max supported commands
#define clear() printf("\033[H\033[J")

void init_shell() {
    clear();  
    printf("\n=============gShell v.0.6=============");  
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

// getting user input
int get_input(char *str) {
	char *buffer;

	buffer = readline(" > ");
	if (strlen(buffer) != 0) {
		add_history(buffer);
		strcpy(str, buffer);
		return 0;
	} else {
		return 1;
	}
}

// find commands in user input
void parse_commands(char *str, char **parsed) {
	
	//printf("\nstr in parse_commands: %s", str);

	int i;
	for (i = 0; i < MAX_COMMANDS; i++) {
		parsed[i] = strsep(&str, " ");

		if (parsed[i] == NULL)
			break;
		if (strlen(parsed[i]) == 0)
			i--;			
	}

	for (int j = 0; j < (sizeof(parsed)/sizeof(parsed[0])); j++) {
		//printf("\nparsed: %s", parsed[j]);
	}
}

// used to find pipes in user input
int parse_pipes(char *str, char **str_piped) {
	
	//printf("\nstr in parse_pipes: %s", str);

	int i;
	for (i = 0; i < 2; i++) {
		str_piped[i] = strsep(&str, "|");
		if (str_piped[i] == NULL)
			break;
	}

	for (int j = 0; j < (sizeof(str_piped)/sizeof(str_piped[0])); j++) {
		//printf("\n parsed in parse_pipes: %s", str_piped[j]);
	}

	if (str_piped[1] == NULL) {
		return 0;		// no pipes were found
	} else {
		return 1;		// found some pipes
	}
	
}

int execute_builtins(char **parsed) {
		
	int no_of_builtins = 2, switch_args = 0;
	int i;

	char *listOfBuiltins[no_of_builtins];

	listOfBuiltins[0] = "exit";
	listOfBuiltins[1] = "cd";

	for (i = 0; i < no_of_builtins; i++) {
		if (strcmp(parsed[0], listOfBuiltins[i]) == 0) {
			switch_args = i + 1;
		}
	}

	switch(switch_args){
		case 1:
			exit(0);
		case 2:
			chdir(parsed[1]);
			return 1;
		default:
			break;
	}

	return 0;
}

int process_user_input(char *str, char **parsed, char **parsed_piped) {
	
	char *strpiped[2];
	int piped = 0;

	piped = parse_pipes(str, strpiped);
	//printf("\npiped: %d", piped);

	if (piped) {
		//printf("\npiped input");
		parse_commands(strpiped[0], parsed);
		parse_commands(strpiped[1], parsed_piped);
	} else {
		//printf("\nnon-piped input");
		parse_commands(str, parsed);
	}

	if (execute_builtins(parsed)) {
		return 0;
	} else {
		return 1 + piped;
	}
}

void execute_commands(char **parsed) {
	
	// fork a child
	pid_t pid = fork();

	if (pid == -1) {
		//printf("\n Failed to fork a child.");
		return;
	} else if (pid == 0) {
		if (execvp(parsed[0], parsed) < 0) {
			//printf("\nCould not execute command.");
		}
		exit(0);
	} else {
		// wait for child to terminate
		wait(NULL);
		return;
	}

}

// executing piped system commands
void execute_piped_commands(char **parsed, char **parsed_pipes) {
	
	// 0 is read end, 1 is write end
	int pipefd[2];
	pid_t p1, p2; 		// parent process id's

	if (pipe(pipefd) < 0) {
		//printf("\n Can't initialize pipe.");
		return;
	}

	p1 = fork();		//creating child process
	if (p1 < 0) {
		//printf("\n fork() call failed while trying to execute pipe operators.");
		return;
	}

	// execute first child
	// only needs to write at the write end 	
	if (p1 == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		//printf("\nfirst child process");
		if (execvp(parsed[0], parsed) < 0) {
			//printf("\n Failed to execute command 1..");
			exit(0);
		}
	} else {
		// parent executing
		p2 = fork();

		if (p2 < 0) {
			//printf("\n fork() call failed while trying to execute pipe operators.");
			return;
		}

		// child2 executing
		// only needs to read at the read end
		if (p2 == 0) {

			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]);
			//printf("\nsecond child process");

			if (execvp(parsed_pipes[0], parsed_pipes) < 0) {
				//printf("\n Failed to execute command 2..");
				exit(0);
			}
		} else {
			//parent executing, waiting for both children
			close(pipefd[0]);
			close(pipefd[1]);
			wait(NULL);
			wait(NULL);
		}

	}

}

int main() 
{ 
    char user_input[1000];
	char *parsed_arguments[MAX_COMMANDS], *parsed_arguments_piped[MAX_COMMANDS];
 	int execution_flag = 0;			// 0 -  no command or builtin; 1 - simple command; 2 - includes pipe operator

	// start shell 
    init_shell(); 
  
    while (1) { 
        // print shell line 
        print_directory(); 
        
		// take input
        if (get_input(user_input)) {
            continue;		
		}

		execution_flag = process_user_input(user_input, parsed_arguments, parsed_arguments_piped);
		//printf("\nexecution flag: %d", execution_flag);
		// returns zero if there's no command or it's a builtin
		// 1 if it is a simple command
		// 2 if it includes a pipe

		// execute
		if (execution_flag == 1) {
			execute_commands(parsed_arguments);
		}
		
		if (execution_flag == 2) {
			execute_piped_commands(parsed_arguments, parsed_arguments_piped);
		}
         
    } 
   
    return 0; 
} 
