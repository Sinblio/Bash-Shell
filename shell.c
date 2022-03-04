/* Benjamin Schroeder
 *
 * shell.c
 *
 * A C-based shell created on Linux shell terminology and execution. Includes
 * the functionality of understanding basic commands through usage of a
 * struct defined in Cmd.c. Along with the ability to complete tasks, indicated
 * by the '&' symbol, in the background using the functionality outlined in
 * processList.c. Also allows for come more advanced functionality including
 * setting inputs and output operators using '<' and '>' respectively. Also
 * allows for the usage of pipes signaled by '|' and usage of 'ctrl + z'
 * inorder to stop a foreground command.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <wait.h>

#include "processList.h"

/* The process of the currently executing foreground command, or 0
 * if none exists. */
pid_t foregroundPid = 0;
/* Stores a reference to the Cmd that is currently being executed in
 * the foreground. */
Cmd* foregroundCmd = NULL;

/* Signal handler for SIGTSTP (SIGnal - Terminal SToP),
 * which is caused by the user pressing control+z. */
void sigtstpHandler(int sig_num) {
	/* Reset handler to catch next SIGTSTP. */
	signal(SIGTSTP, sigtstpHandler);
	if (foregroundPid > 0) {
		/* Forward SIGTSTP to the currently running foreground process. */
		kill(foregroundPid, SIGTSTP);

        if (foregroundCmd != NULL) {
            // Adds the command to the process list
            processList* new = newProcess(foregroundCmd, STDOUT_FILENO, -2);
            addProcess(new);
        }
	}
}

/* The main process loop of the shell. Repeatedly prompts users
 * for an input and processes it. also listens for a 'ctrl + z'
 * input. */
int main(void) {
	/* Listen for control+z (suspend process). */
	signal(SIGTSTP, sigtstpHandler);

	while (1) {
		// Prompts the user for input
	    printf("\n352> ");
		fflush(stdout);

		// Allocates space for the incoming command
		Cmd *cmd = (Cmd*) calloc(1, sizeof(Cmd));

		// Grabs the command in the form of a string
		fgets(cmd->line, MAX_LINE, stdin);

		// Parses the command from a string into arguments
		parseCmd(cmd);

		// Uses if statements to begin seeing how to deal with the command

		/* if the command is empty free allocated space */
		if (!cmd->args[0]) {
			free(cmd);

        /* when exit is entered free allocated space then exit the command */
		} else if (strcmp(cmd->args[0], "exit") == 0) {
            free(cmd);
            removeAllProcesses();
            exit(0);

        /* if jobs is entered prints the status of all background commands */
        } else if (strcmp(cmd->args[0], "jobs") == 0) {
		    printProcess();
            free(cmd);

        /* Resumes a stopped process with a corresponding process id */
        } else if (strcmp(cmd->args[0], "bg") == 0) {
            if (cmd->args[1] != NULL) {
                int status = resumeProcess(atoi(cmd->args[1]));
                if (status == 1) {
                    printf("Could Not Resume Command\n");
                }
            }
            free(cmd);

        /* Otherwise begins to execute the command as a linux command */
		} else {
		    // Creates variables inorder to determine how to execute
		    int input = STDIN_FILENO;
		    int output = STDOUT_FILENO;
		    int background = findSymbol(cmd, BG_OP);

		    // Sets the output to a tmpfile if its a background command
            if (background != -1) {
                output = memfd_create("tmp", O_RDWR);
            }

            //Forks the process and begins execution of the command
            cmd->pid = fork();

            if (cmd->pid == 0) {
                dup2(output, 1);
                callCmd(cmd, input, output);
            }

            // If the process is to run in the foreground the parent waits
			if (background == -1) {
			    // Foreground variables are set
			    foregroundPid = cmd->pid;
			    foregroundCmd = cmd;

			    // Waits for child to finish or be stopped
			    int status = waitpid(cmd->pid, NULL, WUNTRACED);

                // Frees child command if exited without interrupt
                if (status == foregroundCmd->pid)
                    free(foregroundCmd);

                // Resets foreground variables
                foregroundCmd = NULL;
                foregroundCmd = 0;
			} else {
			    // Adds command to the background list
			    processList* new = newProcess(cmd, output, 0);
			    printf("[%d] %d\n", new->pid, cmd->pid);
                addProcess(new);
			}
		}
		// Runs a check on background processes
		checkProcessStatus();

	}
	return 0;
}
