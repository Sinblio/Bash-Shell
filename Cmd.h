/* Benjamin Schroeder
 *
 * Cmd.c
 *
 * The header file for a structure designed to store linux commands. Adds
 * functions inorder to process and store a command in the form of its arguments.
 * Adds the ability to break down and execute a given command based on the broken
 * down arguments. Helper function are also included that allow for splitting
 * a command at an index providing a command containing that arguments left and
 * right of the indicated index, along with commands used to get the length of the
 * command and to search a command for certain specified symbols.
 */

#ifndef CS352P1_CMD_H
#define CS352P1_CMD_H

#include "shellVariables.h"
#include <signal.h>

/* Holds a single command. */
typedef struct Cmd {
    /* The command as input by the user. */
    char line[MAX_LINE + 1];
    /* The command as null terminated tokens. */
    char tokenLine[MAX_LINE + 1];
    /* Pointers to each argument in tokenLine, non-arguments are NULL. */
    char *args[MAX_ARGS];
    /* Pointers to each symbol in tokenLine, non-symbols are NULL. */
    char *symbols[MAX_ARGS];
    /* The process id of the executing command. */
    pid_t pid;
    /* How many arguments a command has including ending NULL value. */
    int length;
    /* Used to create a tree of arguments in callCmd */
    struct Cmd *left;
    struct Cmd *right;
} Cmd;

/* Sets the length property in Cmd. */
void setCmdLength(Cmd* cmd);

/* Parses the command string contained in cmd->line.
 * * Assumes all fields in cmd (except cmd->line) are initailized to zero.
 * * On return, all fields of cmd are appropriatly populated. */
void parseCmd(Cmd* cmd);

/* Finds the index of the first occurance of symbol in cmd->symbols.
 * * Returns -1 if not found. */
int findSymbol(Cmd* cmd, char symbol);

/* Opposite of findSymbol, finds the last occurance of a symbol in cmd->symbols.
 * Returns -1 if not found.
 * Needed inorder to have a command execute from cmd->left to cmd->right. */
int findSymbolReverse(Cmd* cmd, char symbol);

/* Returns the index of the furthest cmd->right symbol in cmd->symbols
 * Returns -1 if there are no symbols that need to be dealt with. */
int parsePoint(Cmd* cmd);

/* Fills cmd->left & cmd->right by splitting at a provided index.
 * Some code is unneeded in the function as cmd will only ever be split
 * at the last for current usages, but wanted to leave functionality in
 * case a use was found. */
void parseCMD(Cmd* cmd, Cmd* left, Cmd* right, int parsePoint);

/* Executes a given Cmd a given input and output.
 * Returns exit code 2 if there is an execution error. */
void exec(Cmd* cmd, int input, int output);

/* Recursive breaks down a given Cmd
 * Returns an exit status of 2 if there is an execution error. */
void callCmd(Cmd* cmd, int input, int output);


#endif //CS352P1_CMD_H