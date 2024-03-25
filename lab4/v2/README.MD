# Tunneling Application

This application comprises two components: a tunneling client (`tunnelc.c`) and a tunneling server (`tunnels.c`). These components facilitate single-hop tunneling of network traffic between a client and a final destination using a server as an intermediary. Below are the instructions and details for each component:

## Tunneling Server (tunnels.c)
### Overview
The tunneling server listens for incoming connections from tunneling clients. It handles client requests, establishes tunneling sessions, and forwards data between clients and final destinations.

### How it Works
- Listens for incoming connections from tunneling clients.
- Accepts client connections and verifies authentication using the secret key.
- Establishes tunneling sessions with clients and forwards data between clients and final destinations.
- Monitors activity on control and data sockets using select() for efficient handling.
- Terminates sessions upon client request.


## Tunneling Client (`tunnelc.c`)

### Overview

The tunneling client establishes a tunneling session with the tunneling server by sending necessary parameters and information. It then communicates with the tunneling server to facilitate data forwarding to the final destination.

### How it Works
- Establishes connection with the tunneling server.
- Sends necessary parameters and information to initiate a tunneling session.
- Receives a data port number from the tunneling server.
- Prints the data port number.
- Closes the TCP socket.

---

## Data Forwarding Using Multiple Sockets

To efficiently handle data forwarding between the client and the destination, the tunneling server employs three sockets and utilizes the `select()` function to monitor activity on these sockets. Here's how data forwarding is achieved using these sockets:

### Utilizing `select()`

```c
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(control_conn, &readfds);
FD_SET(data_sock, &readfds);
FD_SET(response_sock, &readfds);

// Timeout is NULL to block indefinitely until activity is detected on one of the sockets
int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
if (activity < 0) {
    perror("Select error");
    exit(EXIT_FAILURE);
}
```

- The select() function is used to monitor activity on three sockets: 
  - control_conn
  - data_sock
  - response_sock.
- This setup allows the server to handle multiple simultaneous connections efficiently.

### Handling Activity on Control Socket (TCP)

```c
if (FD_ISSET(control_conn, &readfds)) {
// Read termination message from the client
// Check if termination message matches the secret key
// If termination message matches, close connection and free resources
}
```
- The control socket (control_conn) is used for communication related to session termination and control messages.
- Upon receiving a termination message, the server closes the connection and frees resources associated with the session.

### Handling Activity on Data Socket (Client <-> Tunnel)
```c
if (FD_ISSET(data_sock, &readfds)) {
// Receive data from the client
// Forward the received data to the destination
}
```
- The data socket (data_sock) is used for communication between the client and the tunneling server.
- The server receives data from the client and forwards it to the destination.


### Handling Activity on Forwarding Socket (Tunnel <-> Destination)
```c
if (FD_ISSET(response_sock, &readfds)) {
// Receive response from the destination
// Forward the received response to the client
}
```
- The forwarding socket (response_sock) is used for communication between the tunneling server and the destination.
- The server receives responses from the destination and forwards them to the client.

---

## Managing Tunneling Sessions

To manage tunneling sessions effectively, the tunneling server includes functions for finding available session entries and freeing session resources when needed.

### Finding Available Entry

```c
#include "utils.h"
#include "constants.h"

int find_available_entry(struct forwardtab tabentry[]) {
    int tunnel_index;
    for (tunnel_index = 0; tunnel_index < MAX_CLIENTS; tunnel_index++) {
        if (tabentry[tunnel_index].srcaddress == 0) {
            break;
        }
    }
    return (tunnel_index < MAX_CLIENTS) ? tunnel_index : -1;
}
```
- The find_available_entry() function searches for an available entry in the tabentry[] array to store session information.
- It iterates through the array and returns the index of the first available entry where srcaddress is set to 0.
- If no available entry is found, it returns -1.

### Freeing Session Entry
```c
void free_session_entry(struct forwardtab tabentry[], int index) {
    tabentry[index].srcaddress = 0;
    tabentry[index].srcport = 0;
    tabentry[index].dstaddress = 0;
    tabentry[index].dstport = 0;
    tabentry[index].tunnelsport = 0;
}
```
- The free_session_entry() function releases resources associated with a session entry identified by the provided index.
- It sets the fields of the session entry to 0, effectively marking it as available for reuse.
- These functions ensure efficient management of tunneling sessions, allowing the server to handle multiple sessions concurrently while maintaining resource usage.

---

## Makefile

The provided Makefile automates the compilation process for the tunnelc and tunnels programs along with their dependencies.

### How to Use the Makefile
1. Compile the executables
    * `make all`
2. Running server & client (used amber09 as client and amber10 as server)
   - Tunnels
```bash
./tunnels <IPv4 address> <port number> <secret key>
```
   - Tunnelc
```bash
./tunnelc <tunneling server IPv4 address> <tunneling server port number> <secret key> <client IPv4 address> <final destination IPv4 address> <final destination port number>
```
3. Cleaning unuseful files in the end
    * `make clean`