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
                char *args[16] = {}; //specifications said maximum of 16 arguments so we can limit size of args array to save memory
                char *exec_program;  //initialize empty char for program to be executed

                //beginning of string parsing
                char cmd_cpy[CMDLINE_MAX];

                strncpy(cmd_cpy, cmd, sizeof(cmd));

                char delimiter[] = " ";
                char *ptr = strtok(cmd_cpy, delimiter);
                int i = 0;

                while (ptr != NULL)
                {
                        args[i] = ptr;
                        ptr = strtok(NULL, delimiter);
                        i++;
                }
                exec_program = args[0];
                //end of string parsing

                pid = fork(); // fork, creating child process
                if (pid == 0) // Child
                {
                        execvp(exec_program, args); //execvp so shell automatically search programs in $Path
                        exit(0);
                }
                else if (pid != 0) // Parent
                {
                        int status;
                        waitpid(pid, &status, 0);     // wait for child to finishing executing, then continue
                        retval = WEXITSTATUS(status); // WEXITSTATUS gets exit status of child and puts into retval
                }

                fprintf(stderr, "Return status value for '%s': %d\n", // prints exit status of child
                        cmd, retval);
        }

        return EXIT_SUCCESS;
}

/*int cd(char *path)
{
        if sizeof(argv) != 2 {
                
        }
                printf()
        }
chdir(path);
}
*/

/*struct cmd{
char command[50];
cha
}
*/