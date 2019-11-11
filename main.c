#include<stdio.h>
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

struct command {
	char *argv[10];
	int num_of_tokens;
};

void init_shell() {
    clear();  
    printf("\n=============gShell v.0.6=============");  
    char* username = getenv("USER"); 
    printf("\n\nLogged in as: @%s", username); 
    printf("\n"); 
    sleep(0.5); 
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

void parse_commands_for_pipes(struct command *cmd, int command_number, char *str) {
		int j = 0;
		for (j = 0; j < MAX_COMMANDS; j++) {
			cmd[command_number].argv[j] = strsep(&(str), " ");
			if (cmd[command_number].argv[j] == NULL)
				break;
			if (strlen(cmd[command_number].argv[j]) == 0)
				j--;			
		}
		cmd[command_number].num_of_tokens = j;

}

// find commands in user input
void parse_commands(char *str, char **parsed) {

	int i;
	for (i = 0; i < MAX_COMMANDS; i++) {
		parsed[i] = strsep(&str, " ");

		if (parsed[i] == NULL)
			break;
		if (strlen(parsed[i]) == 0)
			i--;			
	}

}

// used to find pipes in user input
int parse_pipes(char *str, char **str_piped, int *no_piped_cmd) {

	int i;
	for (i = 0; i < 100; i++) {
		str_piped[i] = strsep(&str, "|");
		if (str_piped[i] == NULL)
			break;
	}

	*no_piped_cmd = i;

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

int process_user_input(char *str, char **parsed, char **parsed_piped, int *no_piped_cmd, struct command *cmd) {
	
	char *strpiped[100];
	int piped = 0;
	piped = parse_pipes(str, strpiped, no_piped_cmd);

	for (int i = 0; i < *no_piped_cmd; i++) {
		parse_commands_for_pipes(cmd, i, strpiped[i]);
	}


	parse_commands(str, parsed);

	if (execute_builtins(cmd[0].argv)) {
		return 0;
	} else {
		return 1 + piped;
	}
}

void execute_commands(char **parsed) {
	
	// fork a child
	pid_t pid = fork();

	if (pid == -1) {
		printf("\n Failed to fork a child.");
		return;
	} else if (pid == 0) {
		if (execvp(parsed[0], parsed) < 0) {
			printf("\nCould not execute command.");
		}
		exit(0);
	} else {
		// wait for child to terminate
		wait(NULL);
		return;
	}

}

int spawn_process(int in, int out, struct command *cmd) {
	pid_t pid;			// spawned process id

	if ( (pid = fork()) == 0 ) {
		if (in != 0) {
			dup2(in, 0);
			close(in);
		}

		if (out != 1) {
			dup2(out, 1);
			close(out);
		}
		printf("\nin, out: %i, %i", in, out);
		if (execvp(cmd->argv[0], (char * const *)cmd->argv) < 0) {
			printf("\nCould not execute command.");
		}
		exit(0);
	} else if (pid > 0) {
		wait(NULL);
	}

	return pid;
}

void fork_pipes(int n, struct command *cmd) {
	int i;
	pid_t pid;
	int in, fd[2];

	// first process gets input from original file descriptor 0
	in = 0;
	// spawn all but last stage of pipe
	for (i = 0; i < n - 1; ++i) {
		pipe(fd);

		// printf("\nfd[0], fd[1]: %i, %i", fd[0], fd[1]);
		// printf("\nin ->  i: %i, %i", in, i);
		// fd[1] is the write end of the pipe we carry 'in' from prev iteration
		pid = spawn_process(in, fd[1], cmd + i);
		if (pid == 0) {
			wait(NULL);
		}
		// child will write here
		close(fd[1]);

		// keep read end of pipe, next child will read from there
		in = fd[0];
		
	}
	// last piping stage - set stdin to be read end of previous pipe and output to original file descr 1
	if (in != 0) {
		printf("\nIN: %i", in);
		dup2(in, 0);

		close(fd[1]);
		close(in);
		in = fd[0];
	}
	if (execvp(cmd[i].argv[0], (char * const *)cmd[i].argv) < 0) {
		printf("\nCould not execute command.");
		exit(0);
	}
	wait(NULL);
} 

void signal_handler(int signum)
{
	printf("Received signal %d\n", signum);
}

int main() 
{ 
    char user_input[1000];
	char *parsed_arguments[MAX_COMMANDS], *parsed_arguments_piped[MAX_COMMANDS];
 	int execution_flag = 0;			// 0 -  no command or builtin; 1 - simple command; 2 - includes pipe operator
	int no_parsed_cmd = 0;
	struct command cmd[100] = {};
	int *ptr_no_p_cmd = &no_parsed_cmd;

	// start shell 
    init_shell(); 
  
	for(int i=1;i<=64;i++) {
		if (i != 17)
  			signal(i, signal_handler);
	}

    while (1) { 
        // print shell line 
        print_directory(); 
        
		// take input
        if (get_input(user_input)) {
            continue;		
		}

		execution_flag = process_user_input(user_input, parsed_arguments, parsed_arguments_piped, ptr_no_p_cmd, cmd);
		printf("\nno of piped commands: %d", no_parsed_cmd);
		// returns zero if there's no command or it's a builtin
		// 1 if it is a simple command
		// 2 if it includes a pipe

		// execute
		if (execution_flag == 1) {
			execute_commands(cmd[0].argv);
		}
		
		if (execution_flag == 2) {
			fork_pipes(*ptr_no_p_cmd, cmd);
			// for (int i = 0; i < *ptr_no_p_cmd; i++) {
			// 	wait(NULL);
			// }
		}
         
    } 
   
    return 0; 
} 
