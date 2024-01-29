# Problem 2

## Description

### How to Use the Makefile
1. Compile the executables
   * `make all`
2. Running server & client
   * `./remotecmds`
   * `./remotecmdc`
3. Cleaning unuseful files in the end
   * `make clean`

### Files

The folder contains several files serving different purposes:

- `Makefile`:  
  This file simplifies the compilation process.

- `utils.o`, `utils.h`, and `constants.h`:  
  These files enhance modularity by separating functions and constants required for the program's functionality.

- `remotecmds` and `remotecmdc` source files:  
  These files contain the main code for the server and client components respectively.

### Functions and Libraries

Several crucial functions and libraries play essential roles in the program:

- `open()`:  
  This Linux system call opens or creates a file or device, providing a file descriptor as an integer to represent the opened resource.

- `read()` & `write()`:  
  These system calls facilitate reading from and writing to file descriptors, enabling communication between processes and I/O operations.

- `mkfifo()`:  
  This system call creates a named pipe (FIFO) in Linux, allowing inter-process communication through a special file system interface.

- `unlink()`:  
  This Linux system call removes a file or directory entry from the file system. In this program, it's used to delete the FIFO, enabling the creation of a new one when the program is executed again.

- `execvp()`:  
  This function in Linux replaces the current process with a new process specified by the provided command and arguments, facilitating command execution in the child process.

- `waitpid()`:  
  This system call suspends the execution of the calling process until a child process specified by the process ID (PID) argument terminates, enabling proper synchronization in the parent process.

- `strtok()`:  
  This function in the C standard library tokenizes a string into smaller strings (tokens) based on specified delimiters, facilitating string parsing and command-line argument extraction.
