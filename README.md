# Simplified_Interactive_Linux_Shell

## Project Overview

The **Simplified Interactive Linux Shell** project is a custom Linux shell that has the functionalities of process management, input/output redirection, and inter-process communication. The goal was to create a simplified shell that mimics the functionality of a standard Linux shell while implementing essential system calls directly, without relying on higher-level functions like `system` or `popen`.

## Implementation Details

### Key Features

- **Shell Initialization**: Upon launching `myshell`, it displays its process ID and enters a command loop, waiting for user input.
  
- **Exit Command**: The shell gracefully handles the `exit` command, displaying its process ID before termination.

- **Signal Handling**: Customized handling for the Ctrl-C (SIGINT) signal to ensure that the shell can terminate gracefully while providing feedback to the user.

- **Input/Output Redirection**: Implemented using `dup` and `dup2` system calls to redirect standard input and output to/from files, enabling commands to read from or write to files seamlessly.

- **Piping Support**: Utilized pipes to connect multiple command segments, allowing for complex command execution patterns like `ls | sort | uniq`.

