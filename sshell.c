#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> // To access waitpid function

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

                // Builtin command exit
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                // Built-in command pwd
                // with help from https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
                if (!strcmp(cmd, "pwd"))
                {
                        char file_name_buf[_PC_PATH_MAX];
                        getcwd(file_name_buf, sizeof(file_name_buf)); // Prints out filename representing current directory.
                }

                // Start of process parsing
                // with help from https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                // with help from https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok
                char cmd_cpy[CMDLINE_MAX]; //need a copy of cmd with newline for parsing
                strcpy(cmd_cpy, cmd);
                char *processes[512];                   // up to 3 pipe signs or 4 processes + 1 output redirection; 1 process only can be 512 characters
                char process_seps[4][2];                // up to 3 pipe signs + 1 output redirection sign
                char delimiter[] = ">|";                // We want to parse the string, ignoring all spaces, >, and |
                char *ptr = strtok(cmd_cpy, delimiter); // ptr points to each process in the string
                int num_processes = 0;                  // Integer for selecting indexes in processes array
                int num_process_seps = 0;               // Integer for selecting indexes in process separators array
                while (1)                               // keep parsing until end of user input
                {
                        if (ptr != NULL)
                        {
                                processes[num_processes] = ptr;              // Copy each process of cmd to args array
                                if (cmd[ptr - cmd_cpy + strlen(ptr)] == '>') // <------ I dont understand this part at all
                                {
                                        strcpy(process_seps[num_process_seps], ">");
                                        num_process_seps++;
                                }
                                else
                                {
                                        strcpy(process_seps[num_process_seps], "|");
                                        num_process_seps++;
                                }
                                ptr = strtok(NULL, delimiter);
                                num_processes++;
                                continue;
                        }
                        else
                        {
                                break;
                        }
                }
                num_processes -= 1;
                num_process_seps -= 1;
                for (int x = 0; x <= num_processes; x++)
                {
                        printf("%s\n", processes[x]);
                }
                for (int x = 0; x <= num_process_seps; x++)
                {
                        printf("%s\n", process_seps[x]);
                }
                // End of process parsing

                // Start of argument parsing <-------------- should be made into a function later
                int i = 0;
                char *temp_process = processes[i];
                char *args[16] = {"cd", "lol"};
                char arg_delimiter[] = " ";
                char *argptr = strtok(temp_process, arg_delimiter);
                i = 0;
                while (argptr != NULL)
                {
                        args[i] = argptr;
                        argptr = strtok(NULL, arg_delimiter);
                        i++;
                }
                // End of argument parsing

                // cd implementation
                if (!strcmp(args[0], "cd"))
                {
                        if (chdir(args[1]) < 0) // Set process's working directory to file name specified by 2nd argument
                        {
                                fprintf(stderr, "Error: cannot cd into directory\n"); // error if not successful
                                continue;
                        }
                        continue;
                }

                // Regular command
                pid_t pid = fork(); // Fork, creating child process
                if (pid == 0)       // Child
                {
                        if (execvp(args[0], args) < 0) // Set process's working directory to file name specified by 2nd argument
                        {
                                fprintf(stderr, "Error: command not found\n"); // error if not successful
                                continue;
                        }
                        exit(0); // try to exit with exit successful flag
                }
                else if (pid != 0) // Parent
                {
                        int child_status;
                        waitpid(pid, &child_status, 0);     // Wait for child to finishing executing, then parent continue operation
                        retval = WEXITSTATUS(child_status); // WEXITSTATUS gets exit status of child and puts it into retval
                }
                fprintf(stderr, "Return status value for '%s': %d\n", cmd, retval); // Prints exit status of child
        }

        return EXIT_SUCCESS;
}

/*
if (i > 15)
{
        fprintf(stderr, "Error: too many process arguments\n"); // error if too many arguments passed in through terminal
        break;
}
*/