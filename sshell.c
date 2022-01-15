#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h> // to access waitpid function
#include <unistd.h>

/* TO DO
-rename dumbass names like ptr
-implement: echo hello > file world
-fix makefile
-report
*/

#define CMDLINE_MAX 512
#define PROCESS_MAX 5  // up to 3 pipe signs or 4 processes + 1 output redirection
#define METACHAR_MAX 4 // up to 3 pipe signs + 1 output redirection sign;
#define ARGS_MAX 16    // base on specifications

void *parseargsfunction(char *process, char *arg_array, int *args_counter)
{
        int index_args = 0;
        char delimiter[] = " ";
        char *ptr_args;
        ptr_args = strtok(process, delimiter);
        while (ptr_args != NULL)
        {
                arg_array[index_args] = *ptr_args;
                index_args++;
                ptr_args = strtok(NULL, delimiter);
        }
        for (int x = 0; x < index_args; x++)
        {
                printf("%s\n", &arg_array[x]);
        }
        *args_counter += index_args;

        return 0;
}

// with help from https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/
char *removeleadspaces(char *str, int *spacesremoved)
{
        static char ret_str[CMDLINE_MAX];
        int num_spaces = 0;
        int str_index;
        int ret_str_index;
        while (str[num_spaces] == ' ') // iterate string until last leading space char
        {
                num_spaces++;
        }

        for (str_index = num_spaces, ret_str_index = 0; str[str_index] != '\0'; str_index++, ret_str_index++) // copy input string(without leading spaces) to return string
        {
                ret_str[ret_str_index] = str[str_index];
        }
        ret_str[ret_str_index] = '\0'; // add newline on last index to signify end of string
        *spacesremoved += num_spaces;
        return ret_str;
}

enum
{
        ERR_TOO_MANY_ARGS,
        ERR_MISSING_CMD,
        ERR_MISSING_FILE,
        ERR_CANT_OPEN_FILE,
        ERR_MISLOCATE_OUT_REDIRECTION,
        ERR_CANT_CD_DIR,
        ERR_CMD_NOTFOUND,
        ERR_CANT_SLS_DIR,
};

void error_message(int error_code)
{
        switch (error_code)
        {
        case ERR_TOO_MANY_ARGS:
                fprintf(stderr, "Error: too many process arguments\n");
                break;
        case ERR_MISSING_CMD:
                fprintf(stderr, "Error: missing command\n");
                break;
        case ERR_MISSING_FILE:
                fprintf(stderr, "Error: no output file\n");
                break;
        case ERR_CANT_OPEN_FILE:
                fprintf(stderr, "Error: cannot open output file\n");
                break;
        case ERR_MISLOCATE_OUT_REDIRECTION:
                fprintf(stderr, "Error: mislocated output redirection\n");
                break;
        case ERR_CANT_CD_DIR:
                fprintf(stderr, "Error: cannot cd into directory\n");
                break;
        case ERR_CMD_NOTFOUND:
                fprintf(stderr, "Error: command not found\n");
                break;
        case ERR_CANT_SLS_DIR:
                fprintf(stderr, "Error: cannot open directory\n");
                break;
        }
}

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1)
        {
                char *nl;
                // int retval;

                // print prompt
                printf("sshell$ ");
                fflush(stdout);

                // get command line
                fgets(cmd, CMDLINE_MAX, stdin);

                // print command line if stdin is not provided by terminal
                if (!isatty(STDIN_FILENO))
                {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                // remove trailing newline from command line
                nl = strchr(cmd, '\n');
                if (nl)
                {
                        *nl = '\0';
                }

                // get a copy of cmd for parsing
                char *cmd_cpy;
                int num_spaces = 0;
                cmd_cpy = removeleadspaces(cmd, &num_spaces);

                // builtin command exit
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                // built-in command pwd
                // with help from https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
                if (!strcmp(cmd, "pwd"))
                {
                        char file_name_buf[_PC_PATH_MAX];
                        getcwd(file_name_buf, sizeof(file_name_buf)); // Prints out filename representing current directory.
                }

                // start of process + meta character parsing
                // with help from https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                // with help from https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok
                char *processes[PROCESS_MAX];  // 2d array for holding processes
                char *metachar[METACHAR_MAX];  // 2d array for holding meta characters
                char delimiter[] = ">|";       // We want to parse the string, ignoring all spaces, >, and |
                int num_processes = 0;         // integer for selecting indexes in processes array
                int num_metachar = 0;          // integer for selecting indexes in process separators array
                int index_output_redirec = -1; // initially, not set; we dont know yet if instruction includes output redirection
                int expect_file_arg = 0;       // boolean for checking file name after usage of > or >&; 0 false and 1 true
                int ERROR_THROWN = 0;
                char *ptr_process;
                char *ptr_metachar;

                // first error checking
                if (cmd_cpy[0] == '>' || cmd_cpy[0] == '|')
                {
                        error_message(ERR_MISSING_CMD);
                        ERROR_THROWN = 1;
                }

                if (NULL == strchr(cmd, '>') && NULL == strchr(cmd, '|')) // no further parsing needed because only one process
                {
                        processes[num_processes] = cmd_cpy;
                        num_processes++;
                }
                else // more than one process; need further parsing
                {
                        ptr_process = strtok(cmd_cpy, delimiter); // ptr_process points to each process in the string
                        while (ptr_process != NULL)               // keep parsing until end of user input
                        {
                                processes[num_processes] = ptr_process; // copy each process of cmd to args array

                                if (processes[num_processes][0] == '&') // replace & with space if index 0 of process is &;because our delimiter is only > and |
                                {
                                        processes[num_processes][0] = ' ';
                                }

                                if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '>') // <------ I dont understand this part at all
                                {
                                        if (ERROR_THROWN == 0 && (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '>' || cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '|' || cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 2] == '>' || cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 2] == '|'))
                                        {
                                                error_message(ERR_MISSING_FILE);
                                                ERROR_THROWN = 1;
                                        }
                                        else if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '&')
                                        {
                                                ptr_metachar = ">&";
                                                metachar[num_metachar] = ptr_metachar;
                                        }
                                        else
                                        {
                                                ptr_metachar = ">";
                                                metachar[num_metachar] = ptr_metachar;
                                        }
                                        num_metachar++;
                                        index_output_redirec = num_metachar;
                                        expect_file_arg = 1;
                                }
                                else if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '|')
                                {
                                        if (ERROR_THROWN == 0 && (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '>' || cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '|' || cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 2] == '>' || cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 2] == '|'))
                                        {
                                                error_message(ERR_MISSING_CMD);
                                                ERROR_THROWN = 1;
                                        }
                                        else if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '&')
                                        {
                                                ptr_metachar = "|&";
                                                metachar[num_metachar] = ptr_metachar;
                                        }
                                        else
                                        {
                                                ptr_metachar = "|";
                                                metachar[num_metachar] = ptr_metachar;
                                        }
                                        num_metachar++;
                                        expect_file_arg = 0;
                                }
                                else
                                {
                                        expect_file_arg = 0;
                                }
                                ptr_process = strtok(NULL, delimiter);
                                num_processes++;
                        }
                }
                // end of process + meta character parsing

                // DEBUG: verify correct parsing
                for (int x = 0; x < num_processes; x++)
                {
                        printf("%s\n", processes[x]);
                }
                for (int x = 0; x < num_metachar; x++)
                {
                        printf("%s\n", metachar[x]);
                }
                printf("%d\n", num_processes);
                printf("%d\n", num_metachar);

                // more error checking
                if (ERROR_THROWN == 0 && expect_file_arg)
                {
                        error_message(ERR_MISSING_FILE);
                        ERROR_THROWN = 1;
                }
                else if (ERROR_THROWN == 0 && index_output_redirec != -1 && index_output_redirec < num_metachar)
                {
                        error_message(ERR_MISLOCATE_OUT_REDIRECTION);
                        ERROR_THROWN = 1;
                }
                else if (ERROR_THROWN == 0 && num_metachar >= num_processes)
                {
                        error_message(ERR_MISSING_CMD);
                        ERROR_THROWN = 1;
                }

                /*
                int num_args = 0;
                char *arg = (parseargsfunction(processes[0], &num_args));
                printf("%s\n", arg);
                */
                /*
                printf("%d\n", num_args);
                for (int x = 0; x < num_args; x++)
                {
                        printf("%s\n", arg[x]);
                }
                */

                /*
                char *args[16] = {"cd", "lol"};
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
                */

                /*
                // regular command
                pid_t pid = fork(); // fork, creating child process
                if (pid == 0)       // child
                {
                        if (execvp(args[0], args) < 0) // set process's working directory to file name specified by 2nd argument
                        {
                                fprintf(stderr, "Error: command not found\n"); // error if not successful
                                continue;
                        }
                        exit(0); // try to exit with exit successful flag
                }
                else if (pid != 0) // parent
                {
                        int child_status;
                        waitpid(pid, &child_status, 0);     // wait for child to finishing executing, then parent continue operation
                        retval = WEXITSTATUS(child_status); // wEXITSTATUS gets exit status of child and puts it into retval
                }
                fprintf(stderr, "Return status value for '%s': %d\n", cmd, retval); // prints exit status of child
                */
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