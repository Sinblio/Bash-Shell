all: shell352

shell352: shell.o processList.o Cmd.o
	gcc -o shell352 shell.o processList.o Cmd.o -Wall -lm

shell.o: shell.c
	gcc -c shell.c

processList.o: processList.c processList.h
	gcc -c processList.c

Cmd.o: Cmd.c Cmd.h
	gcc -c Cmd.c

clean:
	rm shell352 shell.o processList.o Cmd.o
