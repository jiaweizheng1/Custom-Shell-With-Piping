#include <ctype.h>  // isspace function
#include <dirent.h> // directory functions
#include <fcntl.h>  // open() function for files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // file stats function for file size in bytes
#include <sys/wait.h> // to access waitpid function
#include <unistd.h>   // piping

#define ARGS_MAX 17 // base on specifications + 1 extra for NULL
#define CMDLINE_MAX 512
#define CONTENTS_MAX PROCESS_MAX + METACHAR_MAX // 5 process + 4 meta char
#define METACHAR_MAX 4                          // up to 3 pipe signs + 1 output redirection sign
#define PROCESS_MAX 5                           // up to 3 pipe signs or 4 processes + 1 output redirection
#define RETVAL_MAX 4                            // 3 pipe signs so max of 4 return values

struct stat stat_buff; // sls stat() wont work if not global

struct mystruct // my struct for passing args into execvp
{
        char *my_process_args[ARGS_MAX];
        int my_num_items;
};

int is_spaces(char const *chr) // check if string is all spaces
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
        if (ptr_source == NULL) // done... no more parsing left to do
                return NULL;

        if ((ptr_delimiter = strpbrk(ptr_source, delimiter)) != NULL) // locate delimiters
        {
                *ptr_delimiter = 0; // set delimiter to null so on next iteration we look for next delimiter and not the same one
                ptr_ret = ptr_source;
                ptr_delimiter++;
                ptr_source = ptr_delimiter; // set new starting point for next parse call
        }

        return ptr_ret;
}

int argsfunction(char *process, char *arg_array[ARGS_MAX]) // helper function to break processes into smaller args for mystruct and passing into execvp
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
char *removeleadspaces(char *str, int *spacesremoved) // remove leading spaces from a string
{
        static char ret_str[CMDLINE_MAX];
        int num_spaces = 0;
        int str_index;
        int ret_str_index;

        while (str[num_spaces] == ' ') // iterate through string until last leading space character
        {
                num_spaces++;
        }

        for (str_index = num_spaces, ret_str_index = 0; str[str_index] != '\0'; str_index++, ret_str_index++) // copy input string(without leading spaces) to return string
        {
                ret_str[ret_str_index] = str[str_index];
        }
        ret_str[ret_str_index] = '>';      // add extra > for easier parsing for my_strtok
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
                struct mystruct arr_args_and_count[PROCESS_MAX];           // my struct of array of array of args(broken down from process)
                memset(arr_args_and_count, 0, sizeof(arr_args_and_count)); // set to 0's or reset for reuse
                int retfd[2];
                pipe(retfd); // shared pipe for return values of processes in a pipeline
                int num_pipe_signs = 0;
                char *nl;
                int ERROR_THROWN = 0;
                int retval[RETVAL_MAX]; // return value array
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

                if (!is_spaces(cmd)) // if input is not empty, we continue
                {
                        // start of contents parsing(for error checking) and meta char parsing
                        // with help from https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
                        // with help from https://stackoverflow.com/questions/12460264/c-determining-which-delimiter-used-strtok
                        char *contents[CONTENTS_MAX]; // array of strings; hold strings of processes + meta characters
                        memset(contents, 0, sizeof(contents));
                        char *metachar[METACHAR_MAX]; // array of strings; hold only the meta characters
                        memset(metachar, 0, sizeof(metachar));
                        int num_contents = 0;
                        int num_metachar = 0;
                        int num_output_redirec_signs = 0;
                        int index_output_redirect = -1; // initially, not set
                        char delimiter[] = "|>";        // we want to parse the string, ignoring all spaces, >, and |
                        int skip_and_sign = 0;          // bool for skipping & sign because our delimiter is only | and >(doesnt include &)
                        char *ptr_process;
                        char *ptr_metachar;

                        ptr_process = my_strtok(cmd_cpy, delimiter); // ptr_process points to each process in the string
                        while (ptr_process)                          // keep parsing until end of string
                        {
                                if (skip_and_sign) // copy each process of cmd_copy to args array
                                {
                                        contents[num_contents] = ++ptr_process; // skip & sign if last delimiter detected was > of >& or | of |&
                                        skip_and_sign = 0;
                                }
                                else // copy each process of cmd_copy to args array
                                {
                                        contents[num_contents] = ptr_process;
                                }

                                if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '>') // append meta chars to contents
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
                                        contents[num_contents] = ptr_metachar; // append to contents and metachar array at same time so we dont have to loop again
                                        metachar[num_metachar] = ptr_metachar;
                                        num_metachar++;
                                        num_output_redirec_signs++;
                                }
                                else if (cmd[num_spaces + ptr_process - cmd_cpy + strlen(ptr_process)] == '|') // append meta chars to contents
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
                                        contents[num_contents] = ptr_metachar; // append to contents and metachar array at same time so we dont have to loop again
                                        metachar[num_metachar] = ptr_metachar;
                                        num_metachar++;
                                        num_pipe_signs++;
                                }

                                ptr_process = my_strtok(NULL, delimiter);
                                num_contents++;
                        }
                        // end of contents parsing(for error checking) and meta char parsing

                        // error checking
                        for (int i = 0; i < num_contents; i++) // check for blank processes(||, |>, etc)
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
                        if (!(ERROR_THROWN) && index_output_redirect > 0) // check for mislocated output redirection sign
                        {
                                for (int i = 0; i < num_contents; i++)
                                {
                                        if (contents[i][0] == '|' && i > index_output_redirect)
                                        {
                                                if (((open(contents[i - 1], O_WRONLY | O_TRUNC | O_CREAT, 0644))) < 0) // aparently prof's shell creates the file while also throwing error
                                                {
                                                        error_message(ERR_CANT_OPEN_FILE);
                                                        ERROR_THROWN = 1;
                                                        break;
                                                }
                                                error_message(ERR_MISLOCATE_OUT_REDIRECTION);
                                                ERROR_THROWN = 1;
                                                break;
                                        }
                                }
                        }

                        // start of process to args parsing
                        int num_processes = 0;

                        if (!ERROR_THROWN)
                        {
                                // process -> args parsing
                                for (int i = 0; i < num_contents; i++)
                                {
                                        if (contents[i][0] != '>' && contents[i][0] != '|')
                                        {
                                                arr_args_and_count[num_processes].my_num_items = argsfunction(contents[i], arr_args_and_count[num_processes].my_process_args);
                                                num_processes++;
                                        }
                                }

                                // argument checking; NOTE: file names are not arguments
                                int num_args = 0;

                                if (num_metachar == 0) // one process and no file names
                                {
                                        num_args += arr_args_and_count[0].my_num_items;
                                }
                                else if (num_pipe_signs > num_output_redirec_signs) // 3 pipe signs + 1 output redirect; only 1 file name
                                {
                                        for (int i = 0; i < num_processes; i++)
                                        {
                                                num_args += arr_args_and_count[i].my_num_items; // file name is not considered a argument
                                        }
                                        num_args--; // subtract 1 file name from number of arguments
                                }
                                else // only output redirects or one redirect
                                {
                                        for (int i = 0; i < num_processes; i++)
                                        {
                                                num_args += arr_args_and_count[i].my_num_items; // file name is not considered a argument
                                        }
                                        num_args -= num_output_redirec_signs;
                                }

                                // error if arguments > 16
                                if (num_args > 16)
                                {
                                        error_message(ERR_TOO_MANY_ARGS);
                                        ERROR_THROWN = 1;
                                }

                                if (num_output_redirec_signs > 0) // if need to open file, try to open it first to check for errors; we will reopen later if its fine
                                {
                                        int filename;
                                        if ((filename = open(arr_args_and_count[num_processes - 1].my_process_args[0], O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
                                        {
                                                error_message(ERR_CANT_OPEN_FILE);
                                                ERROR_THROWN = 1;
                                        }
                                        close(filename);
                                }

                                // if output redirect, need additional parsing: "echo hello > file world" -> "echo hello world > file"
                                if (num_output_redirec_signs > 0)
                                {
                                        int new_items_count = arr_args_and_count[num_processes - 2].my_num_items;

                                        for (int prev_index = arr_args_and_count[num_processes - 2].my_num_items, index = 1; index < arr_args_and_count[num_processes - 1].my_num_items; prev_index++, index++)
                                        {
                                                arr_args_and_count[num_processes - 2].my_process_args[prev_index] = arr_args_and_count[num_processes - 1].my_process_args[index];
                                                new_items_count++;
                                        }

                                        arr_args_and_count[num_processes - 2].my_num_items = new_items_count;
                                }
                        }

                        if (!ERROR_THROWN) // input has no errors; we execute the commands
                        {
                                // built-in command pwd
                                // with help from https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
                                if (!strcmp(arr_args_and_count[0].my_process_args[0], "pwd"))
                                {
                                        char dire_name[_PC_PATH_MAX];
                                        if (getcwd(dire_name, sizeof(dire_name)) != NULL) // Prints out filename representing current directory.
                                        {
                                                retval[num_retval] = 0;
                                                num_retval++;
                                        }
                                }
                                // builtin command exit
                                if (!strcmp(arr_args_and_count[0].my_process_args[0], "exit"))
                                {
                                        fprintf(stderr, "Bye...\n");
                                        fprintf(stderr, "+ completed 'exit' [0]\n");
                                        exit(0);
                                }
                                // built-in command sls
                                if (!strcmp(arr_args_and_count[0].my_process_args[0], "sls"))
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
                                        else // unsucessful on opening up directory
                                        {
                                                error_message(ERR_CANT_SLS_DIR);
                                                retval[num_retval] = 1;
                                                num_retval++;
                                        }
                                }
                                else if (num_metachar == 0) // no > or & in command; normal commands including cd
                                {
                                        // built-in command cd
                                        if (!strcmp(arr_args_and_count[0].my_process_args[0], "cd"))
                                        {
                                                int i;
                                                if ((i = chdir(arr_args_and_count[0].my_process_args[1])) < 0) // set process's working directory to file name specified by 2nd argument
                                                {
                                                        error_message(ERR_CANT_CD_DIR); // error if not successful
                                                        retval[num_retval] = 1;
                                                        num_retval++;
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
                                                        if (execvp(arr_args_and_count[0].my_process_args[0], arr_args_and_count[0].my_process_args) < 0)
                                                        {
                                                                error_message(ERR_CMD_NOTFOUND); // error if not successful
                                                                exit(1);
                                                        }
                                                        exit(0); // try to exit with exit successful flag
                                                }
                                                else if (pid < 0) // if Fork failure
                                                {
                                                        fprintf(stderr, "Fork failure.\n");
                                                        return EXIT_FAILURE;
                                                }
                                                else // parent
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
                                                int filename;
                                                if (num_output_redirec_signs > 0) // if need to open file, try to open to see if there are errors
                                                {
                                                        filename = open(arr_args_and_count[num_processes - 1].my_process_args[0], O_WRONLY | O_TRUNC | O_CREAT, 0644);
                                                        dup2(filename, STDOUT_FILENO);
                                                        if (!strcmp(metachar[num_metachar - 1], ">&")) // >
                                                        {
                                                                dup2(filename, STDERR_FILENO);
                                                        }
                                                }
                                                if (execvp(arr_args_and_count[0].my_process_args[0], arr_args_and_count[0].my_process_args) < 0)
                                                {
                                                        error_message(ERR_CMD_NOTFOUND); // error if not successful
                                                        close(filename);
                                                        exit(1);
                                                }
                                                close(filename);
                                                exit(0); // try to exit with exit successful flag
                                        }
                                        else if (pid < 0) // if Fork failure
                                        {
                                                fprintf(stderr, "Fork failure.\n");
                                                return EXIT_FAILURE;
                                        }
                                        else // parent
                                        {
                                                int child_status;
                                                waitpid(pid, &child_status, 0);           // wait for child to finishing executing, then parent continue operation
                                                child_status = WEXITSTATUS(child_status); // WEXITSTATUS gets exit status of child and puts it into retval
                                                retval[num_retval] = child_status;
                                                num_retval++;
                                        }
                                }
                                // with help from https://stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell
                                else // either commands with | signs or commands with | signs and > signs
                                {
                                        pid_t pid = fork();
                                        if (pid == 0)
                                        {
                                                int i;
                                                int in;
                                                pid_t pid[PROCESS_MAX];
                                                int num_pid = 0;

                                                int fd[2];

                                                in = STDIN_FILENO;

                                                for (i = 0; i < num_pipe_signs; i++)
                                                {
                                                        pipe(fd);

                                                        // f[1] is the write end of the pipe, we carry `in` from the prev iteration.
                                                        pid[num_pid] = fork();
                                                        if (pid[num_pid] == 0) // child
                                                        {
                                                                if (in != STDIN_FILENO)
                                                                {
                                                                        dup2(in, STDIN_FILENO); // set input to receive from pipe
                                                                        close(in);
                                                                }

                                                                if (fd[1] != STDOUT_FILENO)
                                                                {
                                                                        if (!strcmp(metachar[i], "|&"))
                                                                        {
                                                                                dup2(fd[1], STDERR_FILENO); // also set stderr to write to pipe
                                                                        }
                                                                        dup2(fd[1], STDOUT_FILENO); // set output to write to pipe
                                                                        close(fd[1]);
                                                                }

                                                                if (execvp(arr_args_and_count[i].my_process_args[0], arr_args_and_count[i].my_process_args) < 0)
                                                                {
                                                                        error_message(ERR_CMD_NOTFOUND); // error if not successful
                                                                        exit(1);
                                                                }
                                                                exit(0);
                                                        }

                                                        close(fd[1]);

                                                        // Keep the read end of the pipe, the next child will read from here
                                                        in = fd[0];
                                                        num_pid++;
                                                }
                                                if (!strcmp(arr_args_and_count[0].my_process_args[0], "cat"))
                                                {
                                                        for (int i = 0; i < num_pid; i++) // wait for children
                                                        {
                                                                int status;
                                                                waitpid(pid[num_pid], &status, 0);
                                                                status = WEXITSTATUS(status);
                                                                write(retfd[1], &status, sizeof(status));
                                                        }
                                                }
                                                else
                                                {
                                                        for (int i = 0; i < num_pid; i++) // wait for children
                                                        {
                                                                int status;
                                                                waitpid(pid[i], &status, 0);
                                                                status = WEXITSTATUS(status);
                                                                write(retfd[1], &status, sizeof(status));
                                                        }
                                                }

                                                if (in != STDIN_FILENO) // last process receives input from pipe
                                                        dup2(in, STDIN_FILENO);

                                                int filename;
                                                if (num_output_redirec_signs > 0) // if need to open file, try to open to see if there are errors
                                                {
                                                        filename = open(arr_args_and_count[num_processes - 1].my_process_args[0], O_WRONLY | O_TRUNC | O_CREAT, 0644);
                                                        dup2(filename, STDOUT_FILENO);
                                                        if (!strcmp(metachar[num_metachar - 1], ">&")) // >
                                                        {
                                                                dup2(filename, STDERR_FILENO);
                                                        }
                                                }
                                                // Execute the last command
                                                if (execvp(arr_args_and_count[i].my_process_args[0], arr_args_and_count[i].my_process_args) < 0)
                                                {
                                                        error_message(ERR_CMD_NOTFOUND); // error if not successful
                                                        close(filename);
                                                        exit(1);
                                                }
                                                close(filename);
                                                exit(0);
                                        }
                                        else if (pid < 0) // if Fork failure
                                        {
                                                fprintf(stderr, "Fork failure.\n");
                                                return EXIT_FAILURE;
                                        }
                                        else // parent
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
                        fprintf(stderr, "+ completed '%s' ", cmd); // prints exit status of child
                        for (int i = 0; i < num_pipe_signs; i++)   // prints ret values of pipelines
                        {
                                int status;
                                read(retfd[0], &status, sizeof(status));
                                fprintf(stderr, "[");
                                fprintf(stderr, "%d", status);
                                fprintf(stderr, "]");
                        }

                        for (int i = 0; i < num_retval; i++) // print other ret values
                        {
                                fprintf(stderr, "[%d]", retval[i]);
                        }
                        fprintf(stderr, "\n");
                }
                close(retfd[0]); // close return value pipe for pipeline
                close(retfd[1]);
        }

        return EXIT_SUCCESS;
}