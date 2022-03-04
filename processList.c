/* Benjamin Schroeder
 *
 * processList.c
 *
 * An implementation of a doubly linked list used to track commands and processes not running
 * in the foreground of the shell. Provides functions inorder to create, add, and remove processes
 * from the list created. Also implements tracking features inorder to close out of existing
 * processes in an appropriate way, including stopped processes. Also provides the ability to
 * print the status of all the commands on the list.
 */

#include "processList.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

// Internally used to track assign process numbers to each processList node.
int processNumber = 1;

// The used to store the head (the furthest left node)
processList* head = NULL;

/* Creates a newProcess allocating memory and setting the default fields
 * then returns a reference to the created node. */
processList* newProcess(Cmd* cmd, int output, int status)
{
    processList* list = (processList*) calloc(1, sizeof(processList));

    list->next = NULL;
    list->last = NULL;
    list->cmd = cmd;
    list->file = output;
    list->status = status;
    list->pid = processNumber;
    processNumber++;

    return list;
}

/* When given a process it is added to the double linked list at the
 * furthest right position. If the list is empty then it is set as the head
 * of the list. */
void addProcess(processList* toAdd) {
    if (head == NULL) {
        // Sets it to the head if there is none
        head = toAdd;
    }
    else {
        // Creates a node to search
        processList* node = head;

        // Increments node to the last filled position
        while (node->next != NULL)
        {
            node = node->next;
        }

        // Insets node into the list
        node->next = toAdd;
        toAdd->last = node;
    }
}

/* Removes a given node from the list. When the head is removed
 * the next node takes its place. */
void removeProcess(processList* toRemove) {
    // Checks to see if there is even a list to remove from
    if (head != NULL) {
        if (head == toRemove) {
            // If the head is the node that is to be removed
            // Disconnects the head
            processList *newlist = head->next;
            head->next = NULL;

            // Releases memory of the command and the node
            free(toRemove->cmd);
            free(toRemove);

            if (newlist != NULL)
                // If there is a next node then reference to head is removed
                newlist->last = NULL;

            // the next node is set to the head
            head = newlist;
        } else {
            // Creates a node to search
            processList* node = head;

            // Node iterates through the list until it reaches the end or it finds
            // the node to remove
            while (node != NULL && node != toRemove) {
                node = node->next;
            }

            // Checks if the node was found otherwise does nothing
            if (node == toRemove) {
                // Disconnects reference in the next node if there is one
                if (node->next != NULL)
                    node->next->last = node->last;

                // Disconnects the rest of the node
                // A last node can be assumed to exist since it was not the head
                node->last->next = node->next;
                node->last = NULL;
                node->next = NULL;

                // Frees the memory of the node
                free(toRemove->cmd);
                free(toRemove);
            }
        }
    }
}

/* Prints the output of the command inside the node stored in  the
 * temporary file. Will not do this in the case that a foreground
 * node is stopped and restored as it was never assigned a tmpfile. */
void printOutput(processList* node) {
    // checks if there is an outfile
    if (node->file != STDOUT_FILENO) {
        // Creates a buffer
        char *buff = malloc(sizeof(char) * 10);

        // Goes to the start of the file
        // May be unessicary as im not sure how file pointers act in children
        lseek(node->file, SEEK_SET, 0);

        // reads amount into buffer
        size_t buffLen = read(node->file, buff, sizeof(buff));

        // while the buffer isn't empty
        while (buffLen > 0) {
            // write the buffer to std out
            write(STDOUT_FILENO, buff, buffLen);
            // fill the buffer with the next section of the file
            buffLen = read(node->file, buff, sizeof(buff));
        }

        // frees the memory of the buffer
        free(buff);
        // closes the tmpfile deleting it form memory
        close(node->file);
    }
}

/* Checks the status of all the processes in the list.
 * Is called at the end of each cycle through the main loop. */
void checkProcessStatus() {
    // Checks to see if there are any processes
    if (head != NULL) {
        //Creates a search node
        processList* node = head;

        //Iterates through every node
        while (node != NULL) {
            /* Only Running nodes that are activly running (those with
             * a status of 0) have the possibility of needing to be updated
             * so the rest can be ignored. */
            if (node->status == 0) {
                // creates a status variable to store status indicators from wait pid
                int status;

                // uses wait pid in conjunction with no WNOHANG to "check on the status"
                node->status = waitpid(node->cmd->pid, &status, WNOHANG);

                // If the node is no longer running processes need to be updates
                if (node->status > 0) {
                    /* If the node was signaled to stop then it prints the node was terminated
                     * followed by identifying information. */
                    if (WIFSIGNALED(status)) {
                        /* Note: I am not specifically printing args as with the recursive program
                         * it is possible for there to be multiple symbols in a line. The code below
                         * acts functionally the same but also can display a more complex command if
                         * symbols are in use in conjunction with a background operator. */
                        printf("[%d] Terminated %s\n", node->pid, strtok(node->cmd->line, "&\n"));
                        // Sets the node status to 2 that being the one used by terminated nodes
                        node->status=-2;
                    }

                    /* If the node exited and the node status does not equal the pid then the node
                     * exited with an error. In this case an error message is displayed with the status
                     * code. */
                    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                        /* Note: I am not specifically printing args as with the recursive program
                         * it is possible for there to be multiple symbols in a line. The code below
                         * acts functionally the same but also can display a more complex command if
                         * symbols are in use in conjunction with a background operator. Note in the
                         * case of an error the message is dumped to console not quite sure on how to
                         * delay the output as to make it less messy. */
                        printf("[%d] Exit %d %s\n", node->pid, WEXITSTATUS(status), strtok(node->cmd->line, "&\n"));
                        // Sets the status of the node to 1 so its marked for deletion
                        node->status = 1;
                    }

                    /* If the node status equals the pid then the node exited successfully. A completion
                     * message is printed along with the output of the node. */
                    if (node->status == node->cmd->pid) {
                        /* Note: I am not specifically printing args as with the recursive program
                         * it is possible for there to be multiple symbols in a line. The code below
                         * acts functionally the same but also can display a more complex command if
                         * symbols are in use in conjunction with a background operator. */
                        printf("[%d] Done %s: \n", node->pid, strtok(node->cmd->line, "&\n"));
                        // Prints the output of the command
                        printOutput(node);
                    }
                }
            }
            //Iterates node
            node = node->next;
        }

        // Sets the iterator back to the beginning
        node = head;

        // Checks each node to see if they need to be removed
        while (node != NULL) {
            // Used inorder to not skip a node in code of the head being removed
            int skip = 0;

            // If a nodes status is greater than 0, then it is removed from the list
            if (node->status > 0) {
                // tmp holds the address of the head to check if it was removed
                processList* tmp = head;

                removeProcess(node);

                // If the head address no longer matches tmp
                if (head != tmp) {
                    // Node is set to the new head
                    node = head;
                    // Skip is set to 1
                    skip = 1;
                }
            }
            // If skip was not changed
            if (skip == 0)
                // Iterate node
                node = node->next;
        }
    }
}

/* Prints the status of every node.
 * Used in the implementation of jobs. */
void printProcess() {
    if (head == NULL)
        // If there are no processes print an empty message.
        printf("No processes to list.\n");
    else {
        // Otherwise create an iterator
        processList* node = head;

        // Go through every node
        while (node != NULL) {
            if (node->status == -2) {
                // If the status is -2 then the node  is stopped
                printf("[%d] Stopped\t%s", node->pid, node->cmd->line);
            } else {
                // Otherwise it is running
                printf("[%d] Running\t%s", node->pid, node->cmd->line);
            }
            node = node->next;
        }
    }
}

/* Resumed a stopped process given a process id. */
int resumeProcess(int processID) {
    // checks if list is empty
    if (head != NULL) {
        processList* node = head;

        // Checks every node if the pid matched if it does then the process is sent SIGCONT
        while (node != NULL) {
            if (node->pid == processID) {
                kill(node->cmd->pid, SIGCONT);
                node->status = 0;
                return 0;
            }
            node = node->next;
        }
    }
    return 1;
}

/* Deletes the entire list freeing any reserved memory. */
void removeAllProcesses() {
    while (head != NULL)
        removeProcess(head);
}