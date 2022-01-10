#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> // To access waitpid function

#define CMDLINE_MAX 512

struct outputred
{
        char metacharacter;
        char file[];
};

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

                // Builtin command exit
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                // Built-in command pwd
                if (!strcmp(cmd, "pwd")) //https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
                {
                        getcwd(NULL, 0); // Prints out filename representing current directory. With arguments NULL and 0, getcwd automatically allocate a buffer larger enough to contain the filename.
                }

                // Other commands
                // Beginning of string parsing
                //based on https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                char *args[17] = {};       // Specifications said maximum of 16 arguments so we can limit size of args array to save memory; 1 extra argument for NULL so execvp works properly for 16 arguments instructions
                char cmd_cpy[CMDLINE_MAX]; //need a copy of cmd because strtok modifies the string in the first argument

                strncpy(cmd_cpy, cmd, sizeof(cmd));

                char delimiter[] = {" "};                  // We want to parse the string, ignoring all spaces
                char *wrdptr = strtok(cmd_cpy, delimiter); // ptr points to each word in the string

                int i = 0; // Integer for selecting indexes in array args
                while (wrdptr != NULL)
                {
                        args[i] = wrdptr;                 // Copy each word of cmd to args array
                        wrdptr = strtok(NULL, delimiter); // First parameter is NULL so that strtok split the string from the next token's starting position.
                        i++;
                }
                // End of string parsing

                // cd implementation
                if (!strcmp(args[0], "cd"))
                {
                        chdir(args[1]); // Set process's working directory to file name
                        continue;
                }

                // Regular command
                pid_t pid = fork(); // Fork, creating child process
                if (pid == 0)       // Child
                {
                        execvp(args[0], args); // Execvp so shell automatically search programs in $Path
                        exit(0);
                }
                else if (pid != 0) // Parent
                {
                        int child_status;
                        waitpid(pid, &child_status, 0);     // Wait for child to finishing executing, then parent continue operation
                        retval = WEXITSTATUS(child_status); // WEXITSTATUS gets exit status of child and puts it into retval
                }

                fprintf(stderr, "Return status value for '%s': %d\n", // Prints exit status of child
                        cmd, retval);
        }

        return EXIT_SUCCESS;
}