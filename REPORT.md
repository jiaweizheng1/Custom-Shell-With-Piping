# ECS 150: Project #1 - Simple Shell

## Summary

The program, 'sshell' implements several features of a basic shell. The
program is able to execute various builtin commands in addition to several
commands implemented within the shell. The additional commands implemented
include output redirection from standard output and standard error to files
denoted by the '>' and '>&' meta characters, as well as the chaining of
processes or pipelines denoted by the '|' and '|&' meta characters.

## Parsing

Our code takes user input through several stages to recognize errors and
perform the correct commands. The first step is to break down user input into
what we call "contents" of processes and meta characters. Contents is a
pointer to an array of strings. The program achieves this through our own
my_strtok() function which we implemented using the string.h strpbrk()
function. The my_strtok() mimics the strtok() in that it searches through a
string to return pointers to words within the string separated by delimiters.
However, this implementation provides an advantage to the default strtok() in
that it is also able to recognize consecutive delimiters and return null
pointers or empty spaces for such cases. This addition is important because
user input like '||', two consecutive pipe signs, is allowed. Thus, for
parsing contents, we call my_strtok() with delimiters '>|' on the user input
several times until we have collected every process and meta character
necessary to check for errors or execute commands. We use smart indexing with
the returned pointer of my_strtok() to recognize which delimiter, '>' or '|',
was scanned in the user input to pass it into the contents array. Error
checking is then trivial because we can simply look for empty contents that
signify missing commands, empty contents after the '>' meta character which
signify a missing file name input, etc. Following contents parsing and error
checking, we have to break those processes within the contents array into
individual arguments so they can be fed into execvp for execution. Similarly
to how we used my_strtok(), we use strtok() with delimiter ' ', a space, to
extract each individual argument of those processes within the contents array.
The helper function argsfunction function uses strtok() to carry out this job.
The arguments are then fed into our own struct called args_execvp_struct for
storing all the arguments. The struct is a 2d array with each row containing
all the strings or arguments necessary for one command. Here, we also use a
counter for arguments to check if the user has bypassed the 16 arguments limit.

## Additional Parsing

More specifically, for cases such as 'echo hello > file world' -> 'echo hello
world > file' we use num_output_redirec_signs to detect if there is output
redirection in the user input. If there is, we copy all arguments of the
process after > except the first one argument, which is the file name, to the
process before > meta character. The args_execvp_struct has an integer called
my_num_items in order to make this part easier.

## Execution

Execution of commands is very simple due to args_execvp_struct. We pass in the
first or 0th index of each row and the corresponding row as arguments into
execvp() to execute the commands. Since execution with execvp() causes the
current process to terminate, we use syscall fork() before execvp() to create
another process to perform the command so the main parent process does not
terminate.

## Execution of Builtin Commands

Commands like sls and pwd are quite different as they are not executed through
the execvp() function. For pwd, we simply use syscall getcwd() to extract the
pathname of the current working directory. For sls, we first opened the
current directory using opendir(). Then we use readdir() to read files of the
current directory with stat() using file name as arguments to read the size of
each file.

## Output Redirection

In the event the meta characters '>' or '>&' are present, output redirection
occurs. Output redirection is performed using the open() and dup2() system
calls. We use the first index of the last process in args_execvp_struct array
for the file name argument of open(). Then, we use dup2() to redirect STDOUT
to the file descriptor returned by open(). If the meta character has '&' in
it, we also use dup2() to redirect STDERR to the file descriptor returned by
open(). Upon execution of the command, the output will then be redirected
into the file.

## Piping

In the event the characters '|' or '|&' are present, piping occurs. Similar to
output redirection, piping also uses dup2(). First, we call pipe() with an
integer array of size 2 as argument to create a pipe, which returns to us the
read and write descriptor of the pipe we have created. Then, we call fork()
to create two processes. Using dup2(), we can have one process redirect their
outputs, including STDERR if '|&', to the pipe after calling execvp() while
the other processes receive their inputs from the pipe. On the next iteration,
we would repeat, create another pipe, use fork again, and use dup2() again to
link outputs and inputs.

## Pipeline Processes Return Values

First, we collect all the pids of the forked children in the for
loop for pipelining. Then we make the parent process iterate through this
collection of pids using a for loop to wait with waitpid() until all the
children have terminated. At the same time, the parent would be copy all the return values to retval[] array. At the end, we print to STDERR all the return values.

## Limitations

The use of waitpid() for collecting return values of processes as of right now
in the program seems to cause hanging for specific pipelines of commands.
However, concurrent pipelining works 100% of the time when the waitpid()
function is removed from the implementation of the pipeline of commands.

# References

[Parsing - Trimming white space](https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/)

[Parsing - strtok](https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split)

[Parsing - strtok delimiter](https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok)

[Parsing - consecutive delimiters](https://stackoverflow.com/questions/26522583/c-strtok-skips-second-token-or-consecutive-delimiter)

[Piping](https://stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell)

[dup2](http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html)
