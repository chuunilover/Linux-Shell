#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <mcheck.h>

#include "parser.h"
#include "shell.h"

/**
 * Program that simulates a simple shell.
 * The shell covers basic commands, including builtin commands 
 * (cd and exit only), standard I/O redirection and piping (|). 
 
 */

#define MAX_DIRNAME 100
#define MAX_COMMAND 1024
#define MAX_TOKEN 128

/* Functions to implement, see below after main */
int execute_cd(char** words);
int execute_nonbuiltin(simple_command *s);
int execute_simple_command(simple_command *cmd);
int execute_complex_command(command *cmd);


int main(int argc, char** argv) {
	
	char cwd[MAX_DIRNAME];           /* Current working directory */
	char command_line[MAX_COMMAND];  /* The command */
	char *tokens[MAX_TOKEN];         /* Command tokens (program name, 
					  * parameters, pipe, etc.) */

	while (1) {

		/* Display prompt */		
		getcwd(cwd, MAX_DIRNAME-1);
		printf("%s> ", cwd);
		
		/* Read the command line */
		fgets(command_line, MAX_COMMAND, stdin);
		/* Strip the new line character */
		if (command_line[strlen(command_line) - 1] == '\n') {
			command_line[strlen(command_line) - 1] = '\0';
		}
		
		/* Parse the command into tokens */
		parse_line(command_line, tokens);

		/* Check for empty command */
		if (!(*tokens)) {
			continue;
		}
		
		/* Construct chain of commands, if multiple commands */
		command *cmd = construct_command(tokens);
		//print_command(cmd, 0);
    
		int exitcode = 0;
		if (cmd->scmd) {
			exitcode = execute_simple_command(cmd->scmd);
			if (exitcode == -1) {
				break;
			}
		}
		else {
			exitcode = execute_complex_command(cmd);
			if (exitcode == -1) {
				break;
			}
		}
		release_command(cmd);
	}
    
	return 0;
}


/**
 * Changes directory to a path specified in the words argument;
 * For example: words[0] = "cd"
 *              words[1] = "csc209/assignment3/"
 * Your command should handle both relative paths to the current 
 * working directory, and absolute paths relative to root,
 * e.g., relative path:  cd csc209/assignment3/
 *       absolute path:  cd /u/bogdan/csc209/assignment3/
 */
int execute_cd(char** words) {
	
	char cwd[MAX_DIRNAME];
	getcwd(cwd, MAX_DIRNAME-1);
	/** 
	 * TODO: 
	 * The first word contains the "cd" string, the second one contains 
	 * the path.
	 * Check possible errors:
	 * - The words pointer could be NULL, the first string or the second 
	 *   string could be NULL, or the first string is not a cd command
	 * - If so, return an EXIT_FAILURE status to indicate something is 
	 *   wrong.
	 */
	if (!words || !words[0] || !words[1] || strcmp("cd", words[0]) != 0) {
		return EXIT_FAILURE;
	}


	/**
	 * TODO: 
	 * The safest way would be to first determine if the path is relative 
	 * or absolute (see is_relative function provided).
	 * - If it's not relative, then simply change the directory to the path 
	 * specified in the second word in the array.
	 * - If it's relative, then make sure to get the current working 
	 * directory, append the path in the second word to the current working
	 * directory and change the directory to this path.
	 * Hints: see chdir and getcwd man pages.
	 * Return the success/error code obtained when changing the directory.
	 */
	 if (!is_relative(words[1])){
		if(strlen(words[1]) >= MAX_DIRNAME){
			return EXIT_FAILURE;
		}
		strcpy(cwd, words[1]);
	 }
	 else{
		if(strlen(words[1]) + strlen(cwd) >= MAX_DIRNAME){
			return EXIT_FAILURE;
		}
		strcpy(cwd+strlen(cwd), "/\0");
		strcpy(cwd+strlen(cwd), words[1]);
	 }
	 if(chdir(cwd)){
	   perror("changing directory");

	 } 
	 return 0;
	 
}


/**
 * Executes a program, based on the tokens provided as 
 * an argument.
 * For example, "ls -l" is represented in the tokens array by 
 * 2 strings "ls" and "-l", followed by a NULL token.
 * The command "ls -l | wc -l" will contain 5 tokens, 
 * followed by a NULL token. 
 */
int execute_command(char **tokens) {
	
	/**
	 * TODO: execute a program, based on the tokens provided.
	 * The first token is the command name, the rest are the arguments 
	 * for the command. 
	 * Hint: see execlp/execvp man pages.
	 * 
	 * - In case of error, make sure to use "perror" to indicate the name
	 *   of the command that failed.
	 *   You do NOT have to print an identical error message to what would 
	 *   happen in bash.
	 *   If you use perror, an output like: 
	 *      my_silly_command: No such file of directory 
	 *   would suffice.
	 * Function returns only in case of a failure (EXIT_FAILURE).
	 */
	 execvp(tokens[0], tokens);
	 perror(tokens[0]);
	 return -1;
}


/**
 * Executes a non-builtin command.
 */
int execute_nonbuiltin(simple_command *s) {
	/**
	 * TODO: Check if the in, out, and err fields are set (not NULL),
	 * and, IN EACH CASE:
	 * - Open a new file descriptor (make sure you have the correct flags,
	 *   and permissions);
	 * - redirect stdin/stdout/stderr to the corresponding file.
	 *   (hint: see dup2 man pages).
	 * - close the newly opened file descriptor in the parent as well. 
	 *   (Avoid leaving the file descriptor open across an exec!) 
	 * - finally, execute the command using the tokens (see execute_command
	 *   function above).
	 * This function returns only if the execution of the program fails.
	 */
	//in file descriptor, out file descriptor, err file descriptor
	int ifiledesc, ofiledesc, efiledesc;
	//redirect stdin, stdour, stderr if specified
	if(s->in){
		ifiledesc = open(s->in, O_RDONLY);
		if (ifiledesc < 0){
		  perror("open");
		  exit(1);
		}
		if(dup2(ifiledesc, fileno(stdin)) < 0){
		  perror("dup2");
		  exit(1);
		}
		if (close(ifiledesc)){
		  perror("close");
		  exit(1);
		}
	}
	if(s->out){
		ofiledesc = open(s->out, O_WRONLY|O_CREAT|O_APPEND, S_IRWXU);
		if (ofiledesc < 0){
		  perror("open");
		  exit(1);
		}
		if(dup2(ofiledesc, fileno(stdout)) < 0){
		  perror("dup2");
		  exit(1);
		}
		if (close(ofiledesc)){
		  perror("close");
		  exit(1);
		}
	  
		
		close(ofiledesc);
	}
	if(s->err){
	  efiledesc = open(s->err, O_WRONLY|O_CREAT|O_APPEND, S_IRWXU);
	  if (efiledesc < 0){
	    perror("open");
	    exit(1);
	  }
	  if (dup2(efiledesc, fileno(stderr)) < 0){
	    perror("dup2");
	  }
	  if (close(efiledesc)){
	    perror("close");
	  }
	}
	//this should not return since the running process should exec
	return execute_command(s -> tokens);
}


/**
 * Executes a simple command (no pipes).
 */
int execute_simple_command(simple_command *cmd) {
	if (cmd -> builtin == 1){ //for cd
		execute_cd(cmd -> tokens);
	}
	else if (cmd -> builtin == 2){ //for exit
		exit(0);
	}
	else{ //non-builtin execution
		if (fork() == 0){
			execute_nonbuiltin(cmd);
			exit(1);
		}
		else{
			int status = 0;
			if(wait(&status) < 1){
			  perror("wait");
			  exit(1);
			}
		}
		
	}
	/**
	 * TODO: 
	 * Check if the command is builtin.
	 * 1. If it is, then handle BUILTIN_CD (see execute_cd function provided) 
	 *    and BUILTIN_EXIT (simply exit with an appropriate exit status).
	 * 2. If it isn't, then you must execute the non-builtin command. 
	 * - Fork a process to execute the nonbuiltin command 
	 *   (see execute_nonbuiltin function above).
	 * - The parent should wait for the child.
	 *   (see wait man pages).
	 */
	return 0;
}


/**
 * Executes a complex command.  A complex command is two commands chained 
 * together with a pipe operator.
 */
int execute_complex_command(command *c) {
	
	/**
	 * TODO:
	 * Check if this is a simple command, using the scmd field.
	 * Remember that this will be called recursively, so when you encounter
	 * a simple command you should act accordingly.
	 * Execute nonbuiltin commands only. If it's exit or cd, you should not 
	 * execute these in a piped context, so simply ignore builtin commands. 
	 */
	 
         //execute simple command if c is simple command
	 if (c -> scmd && !c->scmd->builtin){
		 execute_nonbuiltin(c -> scmd);
		 exit(1);//should only reach here if nonbuiltin fails to execute.
		 return 0;
	 }
	


	/** 
	 * Optional: if you wish to handle more than just the 
	 * pipe operator '|' (the '&&', ';' etc. operators), then 
	 * you can add more options here. 
	 */

	else if (!strcmp(c->oper, "|")) {

		/**
		 * TODO: Create a pipe "pfd" that generates a pair of file 
		 * descriptors, to be used for communication between the 
		 * parent and the child. Make sure to check any errors in 
		 * creating the pipe.
		 */
		
                //pipe file descriptors
		int pfd[2];
		
		if (pipe(pfd)){
			perror("Pipe");
			return -1;
		}
		
                //store both child pids in parent variables
		int c1pid = 0, c2pid = 0;
	
		// Child 1, forked and recursively executing cmd1 with pipe set up
		if((c1pid = fork()) == 0){
		  if(dup2(pfd[1], fileno(stdout)) == -1){
		    perror("dup2");
		    exit(1);
		  }
		  if (close(pfd[0])){
		    perror("close");
		    exit(1);
		  }
			execute_complex_command(c -> cmd1);
			// If cmd1 is also a complex command, this child process will fork 2 processes for each command and exit them
                        // This process will return to shell loop if not exited; prevents duplicate shell processes running as a result of this fork call
			exit(1);
			//first child should not get here; it should have been exec'd in a later execute_complex command call
			//If this does not happen, kill the process so it doesn't reenter the shell input loop
		}
		else if (c1pid == -1){
		  perror("fork");
		  exit(1);
		}
			
		/**
		 * TODO: Fork a new process.
		 * In the child:
		 *  - close one end of the pipe pfd and close the stdout 
		 * file descriptor.
		 *  - connect the stdout to the other end of the pipe (the 
		 * one you didn't close).
		 *  - execute complex command cmd1 recursively. 
		 * In the parent: 
		 *  - fork a new process to execute cmd2 recursively.
		 *  - In child 2:
		 *     - close one end of the pipe pfd (the other one than 
		 *       the first child), and close the standard input file 
		 *       descriptor.
		 *     - connect the stdin to the other end of the pipe (the 
		 *       one you didn't close).
		 *     - execute complex command cmd2 recursively. 
		 *  - In the parent:
		 *     - close both ends of the pipe. 
		 *     - wait for both children to finish.
		 */
		 // Child 2, forked and recursively executing cmd1 with pipe set up
		else if((c2pid = fork()) == 0){
		  if(dup2(pfd[0], fileno(stdin)) == -1){
		    perror("dup2");
		    exit(1);
		  }
		  if(close(pfd[1])){
		    perror("close");
		    exit(1);
		  }
			execute_complex_command(c -> cmd2);
			// As with the first child, this prevents a duplicate shell from running if cmd2 is a complex command
			exit(1);
		}
		else if (c2pid == -1){
		  perror("fork");
		  exit(1);
		}
		else{
		  if(close(pfd[0]) || close(pfd[1])){
		    perror("close");
		    exit(1);
		  }
			int status1 = 0, status2 = 0;
			//Wait for both children; store their status
			if(wait(&status1) < 1 || wait(&status2) < 1){
			  perror("wait");
			  exit(1);
			}
			if (WIFEXITED(status1) < 0){
			  printf("Forked process exited with status %d\n", WEXITSTATUS(status1));
			}
			if(WIFEXITED(status2) < 0){
			  printf("Forked process exited with status %d\n", WEXITSTATUS(status2));
			}
		}
		
	}
	else{ // c is a simple builtin command; ignore and exit
		exit(1);
	}
	return 0;
}
