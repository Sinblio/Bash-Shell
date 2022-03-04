/* Benjamin Schroeder
 *
 * processList.h
 *
 * The header file for a doubly linked list used to track commands and processes not running
 * in the foreground of the shell. Provides functions inorder to create, add, and remove processes
 * from the list created. Also implements tracking features inorder to close out of existing
 * processes in an appropriate way, including stopped processes. Also provides the ability to
 * print the status of all the commands on the list.
 */

#ifndef CS352P1_PROCESSLIST_H
#define CS352P1_PROCESSLIST_H

#include "Cmd.h"

/* Implements a doubly linked list used to store and manage background processes. */
typedef struct processList
{
    // Stores the associated command
    Cmd* cmd;
    // Stores the tmp file index for the output
    int file;
    // Stores the Process Id
    int pid;
    // Stores the status of a node
    int status;
    // Holds the next and previous nodes in a list
    struct processList* next;
    struct processList* last;
} processList;

/* Creates a newProcess allocating memory and setting the default fields
 * then returns a reference to the created node. */
processList* newProcess(Cmd* cmd, int output, int status);

/* When given a process it is added to the double linked list at the
 * furthest right position. If the list is empty then it is set as the head
 * of the list. */
void addProcess(processList* toAdd);

/* Removes a given node from the list. When the head is removed
 * the next node takes its place. */
void removeProcess(processList* toRemove);

/* Prints the output of the command inside the node stored in  the
 * temporary file. Will not do this in the case that a foreground
 * node is stopped and restored as it was never assigned a tmpfile. */
void printOutput(processList* node);

/* Checks the status of all the processes in the list.
 * Is called at the end of each cycle through the main loop. */
void checkProcessStatus();

/* Prints the status of every node.
 * Used in the implementation of jobs. */
void printProcess();

/* Resumed a stopped process given a process id. */
int resumeProcess(int processID);

/* Deletes the entire list freeing any reserved memory. */
void removeAllProcesses();

#endif //CS352P1_PROCESSLIST_H