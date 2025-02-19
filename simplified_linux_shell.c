#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls
#include <signal.h>   // For signal handling

#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_TERMINATE "Myshell (pid=%d) terminates by Ctrl-C\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// The input character
#define INPUT_CHAR "<"

// The output character
#define OUTPUT_CHAR ">"

// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// The shell command
#define SHELL_COMMAND "./myshell"

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *command_line)
{
    int i, n;
    if (!fgets(command_line, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(command_line);
    command_line[--n] = '\0';
    i = 0;
    while (i < n && command_line[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char command_line[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  read_tokens(pipe_segments, command_line, &num_pipe_segments, "|");
//
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void process_cmd(char *command_line)
{
    // Uncomment this line to check the cmdline content
    // printf("Debug: The command line is [%s]\n", command_line);
}

// To handle Ctrl + C
void sigint_handler(int signum) {
    printf(TEMPLATE_MYSHELL_TERMINATE, getpid());
    exit(0);
}

// Helper function: To reset arguments array to NULL
void set_all_null(char *arguments[], int n) {
    for (int i = 0; i < n; ++i) 
    {
        arguments[i] = NULL;
    }
}

// Helper function: To handle redirection of input and output
void handle_redirection(char *arguments[], int num_arguments) {
    for (int i = 0; i < num_arguments; ++i) 
    {
        if (strcmp(arguments[i], INPUT_CHAR) == 0) 
        {
            char *input_file = arguments[i + 1];
            int fd = open(input_file,
                        O_RDONLY,           // Read only
                        S_IRUSR | S_IWUSR); // Read permission and Write permission for owner
            if (fd < 0) {
                perror("open input file");
                exit(1);
            }

            dup2(fd, STDIN_FILENO);
            close(fd);

            // Nullify the redirection operator
            arguments[i] = NULL;    // End the command arguments
        } 
        else if (strcmp(arguments[i], OUTPUT_CHAR) == 0) 
        {
            char *output_file = arguments[i + 1];
            int fd = open(output_file,
                        O_CREAT | O_WRONLY,  // Create file if it doesn't exist, Write only
                        S_IRUSR | S_IWUSR);  // Read permission and Write permission for owner
            if (fd < 0) 
            {
                perror("open output file");
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);

            // Nullify the redirection operator
            arguments[i] = NULL;    // End the command arguments
        }
    }
}

void execute_segment(char *segment, int current_segment_index, int num_segments) {
    char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
    set_all_null(arguments, MAX_ARGUMENTS_PER_SEGMENT);
    int num_arguments = 0;

    read_tokens(arguments, segment, &num_arguments, SPACE_CHARS);
    arguments[num_arguments] = NULL;

    int pfds[2];

    // Handle input/output redirection
    handle_redirection(arguments, num_arguments);

    // If it is the last segment, no need to fork()
    if (current_segment_index == (num_segments - 1)) 
    {
        execvp(arguments[0], arguments);

        perror("execvp failed");
        exit(1);
    } 
    else 
    {
        pipe(pfds);
        pid_t pid = fork();

        if (pid == 0) 
        {
            // the child process handles the command
            dup2(pfds[1], STDOUT_FILENO);
            close(pfds[0]);
            execvp(arguments[0], arguments);

            perror("execvp failed");
            exit(1);
        } 
        else if (pid > 0) 
        {
            // the parent process simply wait for the child and do nothing
            dup2(pfds[0], STDIN_FILENO);
            close(pfds[1]);
            wait(0);
        } 
        else 
        {
            perror("fork failed");
            exit(1);
        }
    }
}

// Function to handle command execution with redirection and pipes
void execute_command(char *command_line) {
    char *segments[MAX_PIPE_SEGMENTS];
    int num_segments = 0;

    read_tokens(segments, command_line, &num_segments, PIPE_CHAR);

    for (int i = 0; i < num_segments; ++i) 
    {
        execute_segment(segments[i], i, num_segments);
    }
}

// Assumptions: Input format is valid
int main()
{
    char *prompt = "jtanat";
    char *exit_prompt = "exit";
    char command_line[MAX_CMDLINE_LENGTH];

    signal(SIGINT, sigint_handler);

    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {
        printf("%s> ", prompt);
        if (get_cmd_line(command_line) == -1)
            continue; /* empty line handling */
        
        if (strcmp(command_line, exit_prompt) == 0) 
        {
            break; // Terminate the shell
        }   

        pid_t pid = fork();
        if (pid == 0)
        {
            // the child process handles the command
            execute_command(command_line);
        }
        else
        {
            // the parent process simply wait for the child and do nothing
            wait(0);
        }
        // execute_command(command_line);
    }

    printf(TEMPLATE_MYSHELL_END, getpid());

    return 0;
}
