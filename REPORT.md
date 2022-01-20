# ECS 150: Project #1 - Simple Shell

## Summary

The program, 'sshell' implements several features of a basic shell. The program is able to execute various builtin commands in addition to several implemented within the shell. The additional commands include output redirection from standard output and standard error to files, as well as the chaining of commands through pipping.

## Execution

This program follows a few steps in order to achieve basic shell functionality.

1. The program will parse the input into commands and arguments
2. Several system calls are used in order to perform the proper command
  * If the command is a built in command such as cd, the program will execute the command
  * If the command is followed by the '>' or '>&' meta-charaters, the output (and error in the second case) of the command will be redirected into a specified file
  * If the command is followed by the '|' or '|&' meta-characters, piping of the commands will occur, where the output of the command before the pipe symbol will be the input of the following command. This may occur up to three times.

### Parsing

### Output Redirection

In the event the characters '>' or '>&' are present, output redirection occurs. Output redirection is performed with the dup2() and fork() system calls.

The fork() system call creates a new process, known as the child process, that will execute the next command. The program will check what the current process ID is, in order to determine the proper process (parent or child).

The dup2() system call duplicates a file descriptor. It receives two file descriptors as arguments. The second argument is the file descriptor that the first argument will be duplicated into.

In the event of output redirection, the program will call fork(), which will create a child process. The child process will open or create a file for output to be redirected into. open() will return a file descriptor. The dup2() call will then replace the file descriptor with the standard output. If the metacharacter '>&' was present, the standard error will be similarly duplicated. Upon execution of the command, the output will then be redirected into the file.

### Piping

In the event the characters '|' or '|&' are present, piping occurs. Piping is achieved through many of the same system calls as output redirection, such as the dup2() and fork() system calls.

In piping, both the fork() and dup2() system calls function in similar ways to their usage in output redirection. The dup2() system calls does however vary slightly in piping. Rather than duplicating the file descriptor of a file to the standard output (and standard error), the standard output file descriptor of the first (or previous) command is duplicated as the standard input descriptor of the second (or next) command.

## References
[Parsing - Trimming white space](https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/)
[Parsing - strtok](https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split)
[Parsing - strtok delimiter](https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok)
[Parsing - consecutive delimiters](https://stackoverflow.com/questions/26522583/c-strtok-skips-second-token-or-consecutive-delimiter)
[Piping](https://stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell)
[dup2](http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html)

