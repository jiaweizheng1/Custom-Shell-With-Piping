.PHONY: all
all: sshell

sshell: sshell.c
	gcc -Wall -Wextra -Werror sshell.c -o sshell

.PHONY: clean
clean:
	rm sshell
	rm *.txt

.PHONY: update
update:
	make clean
	make all