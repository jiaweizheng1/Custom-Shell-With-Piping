/* TO DO
-report
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h> // to access waitpid function
#include <unistd.h>

#define CMDLINE_MAX 512
#define PROCESS_MAX 5  // up to 3 pipe signs or 4 processes + 1 output redirection
#define METACHAR_MAX 4 // up to 3 pipe signs + 1 output redirection sign
#define ARGS_MAX 17    // base on specifications + 1 extra for NULL
#define CONTENTS_MAX 9
#define TOKEN_MAX 32

struct mystruct
{
        char *my_process_args[ARGS_MAX];
        int my_num_items;
};

int argsfunction(char *process, char *arg_array[ARGS_MAX])
{
        int index_args = 0;
        char delimiter[] = " ";
        char *ptr_args;

        ptr_args = strtok(process, delimiter);
        while (ptr_args != NULL)
        {
                arg_array[index_args] = ptr_args;
                index_args++;
                ptr_args = strtok(NULL, delimiter);
        }

        return index_args;
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
                int ERROR_THROWN = 0;
                int retval;
                //// int *retval[PROCESS_MAX - 1]; // output redirect doesnt fork()

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
                if (!strcmp(cmd_cpy, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed 'exit' [0]\n");
                        exit(0);
                }
                // built-in command pwd
                // with help from https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
                if (!strcmp(cmd_cpy, "pwd"))
                {
                        char file_name_buff[_PC_PATH_MAX];
                        getcwd(file_name_buff, sizeof(file_name_buff)); // Prints out filename representing current directory.
                }
                // built-in command pwd
                // HAVENT DONE THIS YET
                if (!strcmp(cmd_cpy, "sls"))
                {
                        return 0;
                }
                if (strcmp(cmd, "")) // none of the previous built-ins and input is not empty
                {
                        // start of process + meta character parsing
                        // with help from https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                        // with help from https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok
                        char *processes[PROCESS_MAX]; // array of processes + output redirection or many output redirections
                        char *metachar[METACHAR_MAX]; // array of strings for holding meta characters
                        char delimiter[] = "|>";      // We want to parse the string, ignoring all spaces, >, and |
                        int num_processes = 0;        // integer for selecting indexes in processes array
                        int num_metachar = 0;         // integer for selecting indexes in process separators array
                        int num_output_redirec_signs = 0;
                        int num_pipesigns = 0;
                        int index_output_redirect = -1; // initially, not set; we dont know yet if instruction includes output redirection
                        int expect_file_arg = 0;        // boolean for checking file name after usage of > or >&; 0 false and 1 true
                        char *ptr_process;
                        char *ptr_metachar;
                        char *contents[METACHAR_MAX];
                        int num_contents = 0;

                        // initial error checking
                        if (NULL != strstr(cmd, "|>") || NULL != strstr(cmd, "|&>") || cmd_cpy[0] == '|' || cmd_cpy[0] == '>')
                        {
                                error_message(ERR_MISSING_CMD);
                                ERROR_THROWN = 1;
                        }

                        if (0 && !(ERROR_THROWN) && NULL == strchr(cmd, '>') && NULL == strchr(cmd, '|')) // no further parsing needed because only one process
                        {
                                processes[num_processes] = cmd_cpy;
                                num_processes++;
                        }
                        else if (1)
                        {
                                char *token;
                                int index_token = 0;
                                for (int index = 0; index < (int)strlen(cmd_cpy); index++)
                                {
                                        strcat(token, &cmd_cpy[index]);
                                        printf("%s", token);
                                }
                                for (int i = 0; i < num_contents; i++)
                                {
                                        printf("%s\n", contents[num_contents]);
                                }
                        }
                        else if (!(ERROR_THROWN)) // more than one process; need further parsing
                        {
                                ptr_process = strtok(cmd_cpy, delimiter); // ptr_process points to each process in the string
                                while (ptr_process != NULL)               // keep parsing until end of user input
                                {
                                        processes[num_processes] = ptr_process; // copy each process of cmd_copy to args array

                                        if (processes[num_processes][0] == '&' && strlen(processes[num_processes]) != 1) // replace & with space if index 0 of process is &;because our delimiter is only > and |
                                        {
                                                processes[num_processes][0] = ' ';
                                        }

                                        if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '>') // <------ I dont understand this part at all
                                        {
                                                if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '&')
                                                {
                                                        ptr_metachar = ">&";
                                                        metachar[num_metachar] = ptr_metachar;
                                                        num_output_redirec_signs++;
                                                }
                                                else
                                                {
                                                        ptr_metachar = ">";
                                                        metachar[num_metachar] = ptr_metachar;
                                                        num_output_redirec_signs++;
                                                }
                                                num_metachar++;
                                                index_output_redirect = num_metachar;
                                                expect_file_arg = 1;
                                        }
                                        else if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '|')
                                        {
                                                if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '&')
                                                {
                                                        ptr_metachar = "|&";
                                                        metachar[num_metachar] = ptr_metachar;
                                                        num_pipesigns++;
                                                }
                                                else
                                                {
                                                        ptr_metachar = "|";
                                                        metachar[num_metachar] = ptr_metachar;
                                                        num_pipesigns++;
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
                        printf("%d\n", num_processes);
                        printf("%d\n", num_metachar);
                        for (int x = 0; x < num_processes; x++)
                        {
                                printf("%s\n", processes[x]);
                        }
                        for (int x = 0; x < num_metachar; x++)
                        {
                                printf("%s\n", metachar[x]);
                        }

                        // more error checking, then parse processes into individual arguments if no errors
                        struct mystruct process_args_and_items_count[PROCESS_MAX];                     // array of array of args(broken down from process)
                        memset(process_args_and_items_count, 0, sizeof(process_args_and_items_count)); // reset mystruct

                        if ((!ERROR_THROWN) && (NULL != strstr(cmd, "||") || NULL != strstr(cmd, "|&|")))
                        {
                                error_message(ERR_MISSING_CMD);
                                ERROR_THROWN = 1;
                        }
                        else if (!(ERROR_THROWN) && expect_file_arg)
                        {
                                error_message(ERR_MISSING_FILE);
                                ERROR_THROWN = 1;
                        }
                        else if (!(ERROR_THROWN) && num_metachar >= num_processes)
                        {
                                error_message(ERR_MISSING_CMD);
                                ERROR_THROWN = 1;
                        }
                        else if (!(ERROR_THROWN) && index_output_redirect != -1 && index_output_redirect < num_metachar)
                        {
                                error_message(ERR_MISLOCATE_OUT_REDIRECTION);
                                ERROR_THROWN = 1;
                        }
                        else if (!ERROR_THROWN)
                        {
                                // args parsing
                                for (int i = 0; i < num_processes; i++)
                                {
                                        process_args_and_items_count[i].my_num_items = argsfunction(processes[i], process_args_and_items_count[i].my_process_args);
                                }

                                // argument checking; NOTE: file names are not arguments
                                int num_args = 0;

                                if (num_metachar == 0) // one process and no file names
                                {
                                        num_args += process_args_and_items_count[0].my_num_items;
                                }
                                else if (num_pipesigns > num_output_redirec_signs) // 3 pipe signs + 1 output redirect; only 1 file name
                                {
                                        for (int i = 0; i < num_processes; i++)
                                        {
                                                num_args += process_args_and_items_count[i].my_num_items; // file name is not considered a argument
                                        }
                                        num_args--; // subtract 1 file name from number of arguments
                                }
                                else // only output redirects or one redirect
                                {
                                        for (int i = 0; i < num_processes; i++)
                                        {
                                                num_args += process_args_and_items_count[i].my_num_items; // file name is not considered a argument
                                        }
                                        num_args -= num_output_redirec_signs;
                                }

                                // error if arguments > 16
                                if (num_args > 16)
                                {
                                        error_message(ERR_TOO_MANY_ARGS);
                                        ERROR_THROWN = 1;
                                }

                                // if output redirect, need additional parsing: "echo hello > file world" -> "echo hello world > file"
                                if (num_output_redirec_signs > 0)
                                {
                                        int new_items_count = process_args_and_items_count[num_processes - 2].my_num_items;

                                        for (int prev_index = process_args_and_items_count[num_processes - 2].my_num_items, index = 1; index < process_args_and_items_count[num_processes - 1].my_num_items; prev_index++, index++)
                                        {
                                                process_args_and_items_count[num_processes - 2].my_process_args[prev_index] = process_args_and_items_count[num_processes - 1].my_process_args[index];
                                                new_items_count++;
                                        }

                                        process_args_and_items_count[num_processes - 2].my_num_items = new_items_count;
                                }
                        }

                        if (!ERROR_THROWN) // input has no errors; we execute the commands
                        {
                                if (num_metachar == 0)
                                {
                                        if (!strcmp(process_args_and_items_count[0].my_process_args[0], "cd"))
                                        {
                                                if (chdir(process_args_and_items_count[0].my_process_args[1]) < 0) // set process's working directory to file name specified by 2nd argument
                                                {
                                                        retval = 1;
                                                        error_message(ERR_CANT_CD_DIR); // error if not successful
                                                }
                                        }

                                        else
                                        {
                                                pid_t pid = fork(); // fork, creating child process
                                                if (pid == 0)       // child
                                                {
                                                        if (execvp(process_args_and_items_count[0].my_process_args[0], process_args_and_items_count[0].my_process_args) < 0)
                                                        {
                                                                error_message(ERR_CMD_NOTFOUND); // error if not successful
                                                        }
                                                        exit(0); // try to exit with exit successful flag
                                                }
                                                else if (pid != 0) // parent
                                                {
                                                        int child_status;
                                                        waitpid(pid, &child_status, 0);     // wait for child to finishing executing, then parent continue operation
                                                        retval = WEXITSTATUS(child_status); // WEXITSTATUS gets exit status of child and puts it into retval
                                                }
                                        }
                                }
                        }
                }

                if (!(ERROR_THROWN) && strcmp(cmd, ""))
                {
                        fprintf(stderr, "+ completed '%s': %d\n", cmd, retval); // prints exit status of child
                }
        }

        return EXIT_SUCCESS;
}