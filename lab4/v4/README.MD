# Problem 2

## Description

### Important Functions
#### Source Address Filtering

The `inet_ntop` function is used to convert the binary address of the client to a human-readable IPv4 address. This is then compared with a predefined source IP prefix (`SOURCE_IP_PREFIX`) to ensure that the connection is authorized. If the client's IP address does not match the expected prefix, the connection is considered unauthorized and closed.

```c
// Source Address Filtering
inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) &client_addr)->sin6_addr), client_ip, sizeof client_ip);
if (strncmp(client_ip, SOURCE_IP_PREFIX, strlen(SOURCE_IP_PREFIX)) != 0) {
    printf("Unauthorized access from %s\n", client_ip);
    close(new_fd);
    continue;
}
```

#### Command Authorization
The is_command_allowed function checks whether a specific command is allowed to be executed based on predefined constraints.
```c
int is_command_allowed(char *command, char **args) {
    if (strcmp(command, "ls") == 0) {
        // Check argument constraints for "ls"
        // Check if the number of arguments exceeds the limit
        // ...
    } else if (strcmp(command, "date") == 0) {
        return ALLOW;
    }
    return DISALLOW;
}
```

#### Forking and Executing Commands
After receiving a command from the client, the server forks a new process to execute the command.
```c
k = fork();
if (k == 0) {
    // child code
    if (execvp(args[0], args) == ERROR) {    // if execution failed, terminate child
        // Send response to client
        // ...
    }
} else {
    waitpid(k, &status, 0);        // block until child process terminates
}
```

#### TCP Connection Usage
The server and client communicate over a TCP (Transmission Control Protocol) connection. In this implementation, the server binds to a specified IP address and port, listens for incoming connections, and accepts incoming connections.

- connect: The connect function is used by the client to establish a connection with the server.
- accept: The accept function is used by the server to accept incoming client connections.
- Maximum Clients: The server is configured to handle a maximum number of clients, defined by MAX_CLIENTS in the code.
```c
// Listen for incoming connections
if (listen(sockfd, MAX_CLIENTS) == ERROR) {
perror("listen");
return EXIT_FAILURE;
}
```

### How to Use the Makefile
1. Compile the executables
   * `make all`
2. Running server & client
   * `./remotecmds.bin <server_ip> <server_port>`
   * `./remotecmdc.bin <client_ip> <server_port>`
3. Cleaning unuseful files in the end
   * `make clean`


