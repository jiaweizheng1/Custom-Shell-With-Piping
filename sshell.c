#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1)
        {
                char *nl;
                int retval;

                // Print prompt
                printf("sshell$ ");
                fflush(stdout);

                // Get command line
                fgets(cmd, CMDLINE_MAX, stdin);

                // Print command line if stdin is not provided by terminal
                if (!isatty(STDIN_FILENO))
                {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                // Remove trailing newline from command line
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                // Builtin command
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                // Regular command
                pid_t pid;
                char *args[16] = {}; // Specifications said maximum of 16 arguments so we can limit size of args array to save memory

                // Beginning of string parsing
                //based on https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                char cmd_cpy[CMDLINE_MAX]; //need a copy of cmd because strtok modifies the string in the first argument

                strncpy(cmd_cpy, cmd, sizeof(cmd));

                char delimiter[] = " ";                 // We want to parse the string, ignoring all spaces
                char *ptr = strtok(cmd_cpy, delimiter); // ptr points to each word in the string

                int i = 0; // Interger for selecting indexes in array args
                while (ptr != NULL)
                {
                        args[i] = ptr;                 // Copy each word of cmd to args array
                        ptr = strtok(NULL, delimiter); // First parameter is NULL so that strtok split the string from the next token's starting position.
                        i++;
                }
                // End of string parsing

                pid = fork(); // Fork, creating child process
                if (pid == 0) // Child
                {
                        execvp(args[0], args); // Execvp so shell automatically search programs in $Path
                        exit(0);
                }
                else if (pid != 0) // Parent
                {
                        int status;
                        waitpid(pid, &status, 0);     // Wait for child to finishing executing, then parent continue operation
                        retval = WEXITSTATUS(status); // WEXITSTATUS gets exit status of child and puts it into retval
                }

                fprintf(stderr, "Return status value for '%s': %d\n", // Prints exit status of child
                        cmd, retval);
        }

        return EXIT_SUCCESS;
}
/*
int cd(*path)
{
       printf("%s\n", getcwd(path, 100));
}
chdir("..");
printf("New directory is '%c'\n", path);
}*/
