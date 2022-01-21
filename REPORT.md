# ECS 150: Project #1 - Simple Shell

## Summary

The program, 'sshell' implements several features of a basic shell. The
program is able to execute various builtin commands in addition to several
other commands implemented within the shell. The additional commands include
output redirection from standard output and standard error to files via the >
meta character, as well as the chaining of processes or pipelines through the
| meta character.

## Parsing

Our code takes user input through several stages to recognize errors and
perform the correct commands. The first step is to break down user input into
what we call "contents" of processes and meta characters. Contents is a
pointer to an array of strings. The program achieves this through our own
my_strtok() function which we implemented using the string.h strpbrk()
function. my_strtok() mimics the strtok() in that it searches through a string
to return pointers to words within the string separated by delimiters.
However, it is superior to strtok() in that it is also able to recognize
consecutive delimiters and return null pointers or empty spaces for such
cases. This superiority is especially important because user input like "||",
two consecutive pipe signs, are allowed. Thus, for parsing contents, we call
my_strtok() with delimiters ">|" on the user input several times until we have
collected every process and meta character we need to check for errors or
execute commands. We use smart indexing with the returned pointer of my_strtok
() to recognize which delimiter, > or |, was scanned over in the user input to
pass it into the contents array. Then, error checking is trivial because we
can simply look for empty contents which signify missing commands, empty
contents after ">" meta character which signify missing file name input, and
etc. After contents parsing and error checking, we have to break those
processes within the contents array into individual arguments so they can be
fed into execvp for execution. Similarly to how we used my_strtok(), we use
strtok() with delimiter " ", space, to extract each individual argument of
those processes within the contents array. The helper function argsfunction
function uses strtok() to carry out this job. The arguments are then fed into
our own struct called mystruct for storing all the arguments. mystruct is a 2d
array with each row containing all the strings or arguments necessary for one
command. Here, we also use a counter for arguments to check if the user has
bypassed the 16 arguments limit.

## Additional Parsing

More specifically, this is cases like "echo hello > file world" -> "echo hello
world > file". For this part, we use num_output_redirec_signs to detect if
there is output redirection in user input. If so, we copy all arguments of the
process after > except the first one argument, which is the file name, to the
process before > meta character. mystruct has integer my_num_items
specifically to make this part easier.

## Execution

Execution of commands is very simple due to mystruct. We pass in every first
or 0th index of each row and the corresponding row as arguments into execvp()
to execute the commands. Since execution with execvp() causes the current
process to terminate, we use syscall fork() before execvp() to create another
process to perform the command so the main parent process does not terminate.

## Execution of Builtin Commands

Commands like sls and pwd are quite different as they are not executed through
execvp() function. For pwd, we simply use syscall getcwd() to extract the
pathname of the current working directory. For sls, we first opened the
current directory using opendir(). Then we use readdir() to read files of the
current directory with stat() with file name as arguments to read the bytes
size of each file.

## Output Redirection

In the event the meta characters ">" or ">&" are present, output redirection
occurs. Output redirection is performed using the open() and dup2() system
call. We use the first index of the last process in mystruct array for the
file name argument of open(). Then, we use dup2() to redirect STDOUT to the
file descriptor returned by open(). If the meta character has "&" in it, we
also use dup2() to redirect STDERR to the file descriptor returned by open().
Upon execution of the command, the output will then be redirected
into the file.

## Piping

In the event the characters '|' or '|&' are present, piping occurs. Similar to
output redirection, piping also uses dup2(). First, we use pipe() with an
integer array of size 2 as argument to create a pipe, which returns to us a
read and write descriptor to that pipe we have created. Then, we call fork()
to create two processes. Using dup2(), we can have one process redirect their
outputs, including STDERR if "|&", to the pipe after call of execvp() while
the other processes receive their inputs from the pipe. On the next iteration,
we would repeat, create another pipe, use fork again, and use dup2() again to
link outputs and inputs.

## Pipeline Processes Return Values

We choose to use a pipe for communicating the return values of child processes
to the parent process. retfd[] is the array of read and write descriptors to
this pipe. First, we collect all the pids of forked children in the for loop
for pipelining. Then we make the parent of these children iterate through this
collection of pids using a for loop to wait with waitpid() until all the
children have terminated. At the same time, the parent would be sending the
WEXITSTATUSes through the pipe using the write() function. Further, the main
parent who never terminates until "exit" is called will use read() function to
collect the WEXITSTATUSes and print them out to STDERR.

## Limitations

The use of waitpid() for collecting return values of processes as of right now
in the program seems to cause hanging for specific pipeline of commands.
However, concurrent pipelining works 100% of the time when the waitpid()
function is removed from the implementation of the pipeline of commands.

# References

[Parsing - Trimming white space](https://www.geeksforgeeks.org/
c-program-to-trim-leading-white-spaces-from-string/)

[Parsing - strtok](https://www.codingame.com/playgrounds/14213/
how-to-play-with-strings-in-c/string-split)

[Parsing - strtok delimiter](https://stackoverflow.com/questions/12460264/
c-determining-which-delimiter-used-strtok)

[Parsing - consecutive delimiters](https://stackoverflow.com/questions/
26522583/c-strtok-skips-second-token-or-consecutive-delimiter)

[Piping](https://stackoverflow.com/questions/8082932/
connecting-n-commands-with-pipes-in-a-shell)

[dup2](http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html)
