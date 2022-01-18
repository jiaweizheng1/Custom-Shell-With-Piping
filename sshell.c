/* TO DO
-recomment due to change of parsing
-report
*/

#include <ctype.h> // isspace function
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h> // to access waitpid function
#include <unistd.h>   // open file function

#define CMDLINE_MAX 512
#define PROCESS_MAX 5                           // up to 3 pipe signs or 4 processes + 1 output redirection
#define METACHAR_MAX 4                          // up to 3 pipe signs + 1 output redirection sign
#define CONTENTS_MAX PROCESS_MAX + METACHAR_MAX // 5 process + 4 meta char
#define ARGS_MAX 17                             // base on specifications + 1 extra for NULL
#define RETVAL_MAX 4                            // 3 pipe signs so max of 4 return values

struct mystruct
{
        char *my_process_args[ARGS_MAX];
        int my_num_items;
};

struct stat stat_buff;

int is_spaces(char const *chr)
{
        while (*chr != '\0')
        {
                if (!isspace((unsigned char)*chr))
                {
                        return 0;
                }
                chr++;
        }
        return 1;
}

// with help from https://stackoverflow.com/questions/26522583/c-strtok-skips-second-token-or-consecutive-delimiter
// what strtok should have been; able to detect consecutive delimiters
char *my_strtok(char *string, char const *delimiter)
{
        static char *ptr_source = NULL;
        char *ptr_delimiter;
        char *ptr_ret = 0;

        if (string != NULL) // if not done, continue finding delimiters
                ptr_source = string;
        if (ptr_source == NULL)
                return NULL; // done... found all delimiters

        if ((ptr_delimiter = strpbrk(ptr_source, delimiter)) != NULL) // locate delimiters
        {
                *ptr_delimiter = 0;
                ptr_ret = ptr_source;
                ptr_delimiter++;
                ptr_source = ptr_delimiter;
        }

        return ptr_ret;
}

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
        ret_str[ret_str_index] = '>';      // add extra > for easier parsing
        ret_str[ret_str_index + 1] = '\0'; // add newline on last index to signify end of string
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
                int retval[RETVAL_MAX];
                int num_retval = 0;

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

                if (!is_spaces(cmd)) // none of the previous built-ins and input is not empty
                {
                        // start of contents parsing(for error checking) and meta char parsing
                        // with help from https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                        // with help from https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok
                        char *contents[CONTENTS_MAX]; // array of strings; hold strings of processes + meta characters
                        char *metachar[METACHAR_MAX]; // array of strings; hold only the meta characters
                        int num_contents = 0;
                        int num_metachar = 0;
                        int num_pipe_signs = 0;
                        int num_output_redirec_signs = 0;
                        int index_output_redirect = -1; // initially, not set
                        char delimiter[] = "|>";        // we want to parse the string, ignoring all spaces, >, and |
                        int skip_and_sign = 0;
                        char *ptr_process;
                        char *ptr_metachar;

                        ptr_process = my_strtok(cmd_cpy, delimiter); // ptr_process points to each process in the string
                        while (ptr_process)                          // keep parsing until end of string
                        {
                                if (skip_and_sign) // copy each process of cmd_copy to args array
                                {
                                        contents[num_contents] = ++ptr_process;
                                        skip_and_sign = 0;
                                }
                                else
                                {
                                        contents[num_contents] = ptr_process;
                                }

                                if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '>') // append meta chars to cotents
                                {
                                        if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '&')
                                        {
                                                ptr_metachar = ">&";
                                                skip_and_sign = 1;
                                        }
                                        else
                                        {
                                                ptr_metachar = ">";
                                        }
                                        num_contents++;
                                        index_output_redirect = num_contents;
                                        contents[num_contents] = ptr_metachar;
                                        metachar[num_metachar] = ptr_metachar;
                                        num_metachar++;
                                        num_output_redirec_signs++;
                                }
                                else if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '|') // append meta chars to cotents
                                {
                                        if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process) + 1] == '&')
                                        {
                                                ptr_metachar = "|&";
                                                skip_and_sign = 1;
                                        }
                                        else
                                        {
                                                ptr_metachar = "|";
                                        }
                                        num_contents++;
                                        contents[num_contents] = ptr_metachar;
                                        metachar[num_metachar] = ptr_metachar;
                                        num_metachar++;
                                        num_pipe_signs++;
                                }

                                ptr_process = my_strtok(NULL, delimiter);
                                num_contents++;
                        }
                        // end of contents parsing(for error checking) and meta char parsing

                        /* debug
                        for (int i = 0; i < num_contents; i++)
                        {
                                printf("contents: %s\n", contents[i]);
                        }
                        */

                        // error checking
                        for (int i = 0; i < num_contents; i++) // check for blank processes
                        {
                                if (is_spaces(contents[i]))
                                {
                                        error_message(ERR_MISSING_CMD);
                                        ERROR_THROWN = 1;
                                        break;
                                }
                                else if (contents[i][0] == '>')
                                {
                                        break;
                                }
                        }
                        if (!(ERROR_THROWN) && index_output_redirect > 0) // check for missing file names
                        {
                                if (!is_spaces(contents[index_output_redirect - 1]) && is_spaces(contents[index_output_redirect + 1]))
                                {
                                        error_message(ERR_MISSING_FILE);
                                        ERROR_THROWN = 1;
                                }
                        }
                        if (!(ERROR_THROWN)) // check for mislocated output redirection sign
                        {
                                for (int i = 0; i < num_contents; i++)
                                {
                                        if (contents[i][0] == '|' && i > index_output_redirect)
                                        {
                                                error_message(ERR_MISLOCATE_OUT_REDIRECTION);
                                                ERROR_THROWN = 1;
                                                break;
                                        }
                                }
                        }

                        // start of process to args parsing
                        struct mystruct process_args_and_items_count[PROCESS_MAX];                     // array of array of args(broken down from process)
                        memset(process_args_and_items_count, 0, sizeof(process_args_and_items_count)); // reset mystruct
                        int num_processes = 0;

                        if (!ERROR_THROWN)
                        {
                                // process -> args parsing
                                for (int i = 0; i < num_contents; i++)
                                {
                                        if (contents[i][0] != '>' && contents[num_contents][0] != '|')
                                        {
                                                process_args_and_items_count[num_processes].my_num_items = argsfunction(contents[i], process_args_and_items_count[num_processes].my_process_args);
                                                num_processes++;
                                        }
                                }

                                // argument checking; NOTE: file names are not arguments
                                int num_args = 0;

                                if (num_metachar == 0) // one process and no file names
                                {
                                        num_args += process_args_and_items_count[0].my_num_items;
                                }
                                else if (num_pipe_signs > num_output_redirec_signs) // 3 pipe signs + 1 output redirect; only 1 file name
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

                        /*
                        for (int i = 0; i < num_processes; i++)
                        {
                                int k = process_args_and_items_count[i].my_num_items;
                                for (int j = 0; j < k; j++)
                                {
                                        printf("args: %s\n", process_args_and_items_count[i].my_process_args[j]);
                                }
                        }
                        for (int i = 0; i < num_metachar; i++)
                        {
                                printf("meta: %s\n", metachar[i]);
                        }
                        */

                        if (!ERROR_THROWN) // input has no errors; we execute the commands
                        {
                                // built-in command pwd
                                // with help from https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
                                if (!strcmp(process_args_and_items_count[0].my_process_args[0], "pwd"))
                                {
                                        char file_name[_PC_PATH_MAX];
                                        if (NULL != getcwd(file_name, sizeof(file_name))) // Prints out filename representing current directory.
                                        {
                                                retval[num_retval] = 0;
                                                num_retval++;
                                        }
                                        else // unsuccessful
                                        {
                                                retval[num_retval] = 1;
                                                num_retval++;
                                        }
                                }
                                // builtin command exit
                                if (!strcmp(process_args_and_items_count[0].my_process_args[0], "exit"))
                                {
                                        fprintf(stderr, "Bye...\n");
                                        fprintf(stderr, "+ completed 'exit' [0]\n");
                                        exit(0);
                                }
                                // built-in command sls
                                if (!strcmp(process_args_and_items_count[0].my_process_args[0], "sls"))
                                {
                                        DIR *ptr_dir;
                                        struct dirent *ptr_dir_entry;

                                        ptr_dir = opendir("."); // "." refer to current working directory
                                        if (ptr_dir != NULL)
                                        {
                                                while ((ptr_dir_entry = readdir(ptr_dir)))
                                                {
                                                        if (ptr_dir_entry->d_name[0] != '.')
                                                        {
                                                                stat(ptr_dir_entry->d_name, &stat_buff);
                                                                fprintf(stdout, "%s (%ld bytes)\n", ptr_dir_entry->d_name, stat_buff.st_size);
                                                        }
                                                }
                                                (void)closedir(ptr_dir); // done with directory

                                                retval[num_retval] = 0;
                                                num_retval++;
                                        }
                                        else
                                        {
                                                error_message(ERR_CANT_SLS_DIR);
                                                retval[num_retval] = 1;
                                                num_retval++;
                                        }
                                }
                                else if (num_metachar == 0) // no > or & in command
                                {
                                        // built-in command cd
                                        if (!strcmp(process_args_and_items_count[0].my_process_args[0], "cd"))
                                        {
                                                if (chdir(process_args_and_items_count[0].my_process_args[1]) < 0) // set process's working directory to file name specified by 2nd argument
                                                {
                                                        retval[num_retval] = 1;
                                                        num_retval++;
                                                        error_message(ERR_CANT_CD_DIR); // error if not successful
                                                }
                                                else
                                                {
                                                        retval[num_retval] = 0;
                                                        num_retval++;
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
                                                        waitpid(pid, &child_status, 0);                 // wait for child to finishing executing, then parent continue operation
                                                        retval[num_retval] = WEXITSTATUS(child_status); // WEXITSTATUS gets exit status of child and puts it into retval
                                                        num_retval++;
                                                }
                                        }
                                }
                                else if (num_metachar == num_output_redirec_signs) // command with >*
                                {
                                        pid_t pid = fork(); // fork, creating child process
                                        if (pid == 0)       // child
                                        {
                                                int fd;

                                                if ((fd = open(process_args_and_items_count[1].my_process_args[0], O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
                                                {
                                                        error_message(ERR_CANT_OPEN_FILE);
                                                }
                                                dup2(fd, STDOUT_FILENO);
                                                if (!strcmp(metachar[0], ">&")) // >
                                                {
                                                        dup2(fd, STDERR_FILENO);
                                                }

                                                if (execvp(process_args_and_items_count[0].my_process_args[0], process_args_and_items_count[0].my_process_args) < 0)
                                                {
                                                        error_message(ERR_CMD_NOTFOUND); // error if not successful
                                                }
                                                close(fd);
                                                exit(0); // try to exit with exit successful flag
                                        }
                                        else if (pid != 0) // parent
                                        {
                                                int child_status;
                                                waitpid(pid, &child_status, 0);                 // wait for child to finishing executing, then parent continue operation
                                                retval[num_retval] = WEXITSTATUS(child_status); // WEXITSTATUS gets exit status of child and puts it into retval
                                                num_retval++;
                                        }
                                }
                        }
                }

                if (!(ERROR_THROWN) && !is_spaces(cmd))
                {
                        fprintf(stderr, "+ completed '%s'", cmd); // prints exit status of child

                        if (num_retval == 1)
                        {
                                fprintf(stderr, " ");
                        }
                        else
                        {
                                fprintf(stderr, ": ");
                        }

                        for (int i = 0; i < num_retval; i++)
                        {
                                fprintf(stderr, "[%d]", retval[i]);
                        }
                        fprintf(stderr, "\n");
                }
        }

        return EXIT_SUCCESS;
}