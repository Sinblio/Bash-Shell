/* Benjamin Schroeder
 *
 * Cmd.c
 *
 * The implementation of a structure designed to store linux commands. Adds
 * functions inorder to process and store a command in the form of its arguments.
 * Adds the ability to break down and execute a given command based on the broken
 * down arguments. Helper function are also included that allow for splitting
 * a command at an index providing a command containing that arguments left and
 * right of the indicated index, along with commands used to get the length of the
 * command and to search a command for certain specified symbols.
 */

#include "Cmd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <wait.h>

/* Sets the length property in Cmd. */
void setCmdLength(Cmd* cmd) {
    int length = 0;

    while (cmd->args[length] != NULL || cmd->symbols[length] != NULL)
    {
        length++;
    }

    cmd->length = length;
}

/* Parses the command string contained in cmd->line.
 * * Assumes all fields in cmd (except cmd->line) are initailized to zero.
 * * On return, all fields of cmd are appropriatly populated. */
void parseCmd(Cmd* cmd) {
    char* token;
    int i=0;
    strcpy(cmd->tokenLine, cmd->line);
    strtok(cmd->tokenLine, "\n");
    token = strtok(cmd->tokenLine, " ");
    while (token != NULL) {
        if (*token == '\n') {
            cmd->args[i] = NULL;
        } else if (*token == REDIRECT_OUT_OP || *token == REDIRECT_IN_OP
                   || *token == PIPE_OP || *token == BG_OP) {
            cmd->symbols[i] = token;
            cmd->args[i] = NULL;
        } else {
            cmd->args[i] = token;
        }
        token = strtok(NULL, " ");
        i++;
    }
    cmd->pid = -1;
    cmd->args[i] = NULL;
    cmd->left = NULL;
    cmd->right = NULL;
    setCmdLength(cmd);
}

/* Finds the index of the first occurance of symbol in cmd->symbols.
 * * Returns -1 if not found. */
int findSymbol(Cmd* cmd, char symbol) {
    for (int i = 0; i < MAX_ARGS; i++) {
        if (cmd->symbols[i] && *cmd->symbols[i] == symbol) {
            return i;
        }
    }
    return -1;
}

/* Opposite of findSymbol, finds the last occurance of a symbol in cmd->symbols.
 * Returns -1 if not found.
 * Needed inorder to have a command execute from cmd->left to cmd->right. */
int findSymbolReverse(Cmd* cmd, char symbol) {
    for (int i = cmd->length; i >= 0; i--) {
        if (cmd->symbols[i] && *cmd->symbols[i] == symbol) {
            return i;
        }
    }
    return -1;
}

/* Returns the index of the furthest cmd->right symbol in cmd->symbols
 * Returns -1 if there are no symbols that need to be dealt with. */
int splitPoint(Cmd* cmd) {
    int parse = -1;

    //Symbols that are checked for
    int insignal = findSymbolReverse(cmd, REDIRECT_IN_OP);
    int outsignal = findSymbolReverse(cmd, REDIRECT_OUT_OP);
    int pipesignal = findSymbolReverse(cmd, PIPE_OP);
    int bgopsignal = findSymbolReverse(cmd, BG_OP);

    //Checks to see which has the highest index
    if (parse < insignal)
        parse = insignal;

    if (parse < outsignal)
        parse = outsignal;

    if (parse < pipesignal)
        parse = pipesignal;

    if (parse < bgopsignal)
        parse = bgopsignal;

    return parse;
}

/* Fills cmd->left & cmd->right by splitting at a provided index.
 * Some code is unneeded in the function as cmd will only ever be split
 * at the last for current usages, but wanted to leave functionality in
 * case a use was found. */
void splitCMD(Cmd* cmd, int splitIndex) {
    int i;

    // if cmd already has a left and right declarations are frees
    if (cmd->left != NULL)
        free(cmd->left);
    if (cmd->right != NULL)
        free(cmd->right);

    // left and right are allocated memory
    cmd->left = (Cmd*) calloc(1, sizeof(Cmd));
    cmd->right = (Cmd*) calloc(1, sizeof(Cmd));

    // the args and symbols up to the split index are copied to left
    for (i = 0; i < splitIndex; i++) {
        cmd->left->args[i] = cmd->args[i];
        cmd->left->symbols[i] = cmd->symbols[i];
    }

    // makes sure there is a NULL value following the args and symbols
    cmd->left->args[i + 1] = NULL;
    cmd->left->symbols[i + 1] = NULL;

    // the args and symbols following the split index are copied to right
    for (i = splitIndex + 1; i < cmd->length; i++) {
        cmd->right->args[i - splitIndex - 1] = cmd->args[i];
        cmd->right->symbols[i - splitIndex - 1] = cmd->symbols[i];
    }

    // makes sure the args and symbols are followed by a NULL value
    cmd->right->args[i-splitIndex] = NULL;
    cmd->right->symbols[i-splitIndex] = NULL;;

    // runs setCmdLength for left and right
    setCmdLength(cmd->left);
    setCmdLength(cmd->right);
}

/* Executes a given Cmd a given input and output.
 * Returns exit code 2 if there is an execution error. */
void exec(Cmd* cmd, int input, int output) {
    // Set stdin if other input is desired
    if (input != STDIN_FILENO) {
        dup2(input, 0);
    }

    // Sets stdout if other output is desired
    if (output != STDOUT_FILENO) {
        dup2(output, 1);
    }

    // Executes cmd
    execvp(cmd->args[0], cmd->args);

    // Returns status 2 if there is an execution error
    exit(10);
}

/* Recursive breaks down a given Cmd
 * Returns an exit status of 2 if there is an execution error. */
void callCmd(Cmd* cmd, int input, int output) {
    // Checks to see if there is an index of a symbol that requires the command to be processed.
    int splitIndex = splitPoint(cmd);

    if (splitIndex != -1)
    {
        //If there is a symbol call splitCmd to split the command at the index found
        splitCMD(cmd, splitIndex);

        //Gets the symbol that was found by split point
        char symbol = cmd->symbols[splitIndex][0];

        /* If a & is found the pid of left is set to the pid of command
         * as the same thread will be dealing with the parsed code.
         * Anything to the right of the symbol is ignored. */
        if (symbol == BG_OP) {
            cmd->left->pid = cmd->pid;
            callCmd(cmd->left, input, output);
        }

        /* If a < is found the pid of left is set to the pid of command
         * as the same thread will be dealing with the parsed code.
         * The first argument to the right of the symbol is assumed to
         * be a file name and is opened, and passed through to the
         * recursive call. */
        if (symbol == REDIRECT_IN_OP) {
            cmd->left->pid = cmd->pid;
            input = open(cmd->right->args[0], O_RDONLY);
            callCmd(cmd->left, input, output);
            // closes the file after execution
            close(input);
        }

        /* If a > is found the pid of left is set to the pid of command
         * as the same thread will be dealing with the parsed code.
         * The first argument to the right of the symbol is assumed to
         * be a file name and is opened, and passed through to the
         * recursive call. */
        if (symbol == REDIRECT_OUT_OP) {
            cmd->left->pid = cmd->pid;
            output = open(cmd->right->args[0], O_WRONLY|O_TRUNC|O_CREAT);
            callCmd(cmd->left, input, output);
            // closes the file after execution
            close(output);
        }

        /* If the symbol is a |, a pipe is created. 2 new processes are then created
         * with the left cmd outputting to the pipe, and the right receiving. The main
         * process closes the pipe then waits for its children to be complete.
         * Recursive piping should be possible moving left to right although I haven't
         * tested it */
        if (symbol == PIPE_OP){
            // Creates and opens a pipe
            int cmdpipe[2];

            if (pipe(cmdpipe) == -1)
            {
                printf("Pipe Error\n");
            }

            // Forks for the left cmd
            cmd->left->pid = fork();

            if (cmd->left->pid == 0) {
                close(cmdpipe[0]);
                callCmd(cmd->left, input, cmdpipe[1]);
            }

            // Closes the input pipe
            close(cmdpipe[1]);

            // Forks for the right command
            cmd->right->pid = fork();

            if (cmd->right->pid == 0) {
                callCmd(cmd->right, cmdpipe[0], output);
            }

            // Closes the output command
            close(cmdpipe[0]);

            // Main process waits for children to complete
            int leftStatus = waitpid(cmd->left->pid, NULL, 0);
            int rightStatus = waitpid(cmd->right->pid, NULL, 0);

            // If either child exited due to an execution error, the parent also exits
            if (leftStatus == 10 || rightStatus == 10)
                exit(10);
        }
        // Frees memory reserved by left and right
        free(cmd->right);
        free(cmd->left);

        cmd->right = NULL;
        cmd->left = NULL;
    }
    else {
        // If command does no need to be processed then it is executed
        exec(cmd, input, output);
    }
}