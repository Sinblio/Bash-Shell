# 352Shell

Created by Benjamin Schroeder

## shell.c

A C-based shell created on Linux shell terminology and execution. Includes the functionality of understanding basic commands through usage of a struct defined in Cmd.c. Along with the ability to complete tasks, indicated by the '&' symbol, in the background using the functionality outlined in processList.c. Also allows for come more advanced functionality including setting inputs and output operators using '<' and '>' respectively. Also allows for the usage of pipes signaled by '|' and usage of 'ctrl + z' inorder to stop a foreground command.

## cmd.c & cmd.h

The implementation of a structure designed to store linux commands. Adds functions inorder to process and store a command in the form of its arguments. Adds the ability to break down and execute a given command based on the broken down arguments. Helper function are also included that allow for splitting a command at an index providing a command containing that arguments left and right of the indicated index, along with commands used to get the length of the command and to search a command for certain specified symbols.

## processList.c & processList.h

An implementation of a doubly linked list used to track commands and processes not running in the foreground of the shell. Provides functions inorder to create, add, and remove processes from the list created. Also implements tracking features inorder to close out of existing processes in an appropriate way, including stopped processes. Also provides the ability to print the status of all the commands on the list.

## shellVariables.h

A file created for the convenience of having shell variables stored in an importable class, allowing for their global usage while only needing to modify one file inorder to modify shell parameters.