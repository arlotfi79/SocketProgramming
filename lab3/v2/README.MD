# Small Size File Transfer Protocol (SSFTP)

---

## Introduction

This repository contains a simple implementation of a small size file transfer protocol (SSFTP) using datagram sockets in C. The protocol allows clients to request files from a server, which then sends the requested file in chunks to the client.

---

### Client (ssftp.c)

The client program (`ssftp.c`) initiates file transfer requests to the server. It takes command-line arguments for the filename, server IP, server port, client IP, and payload size.

#### How it Works

- **Preparing Filename:** The filename is prepared and padded with 'Z' characters if necessary to match a fixed length.

- **Creating Socket:** A socket is created and bound for communication with the server.

- **Timeout Mechanism:** A timeout mechanism is set up to handle potential packet loss during transmission.

- **Sending Request:** The client sends a request to the server for the specified file.

- **Receiving File Size:** The client receives the file size from the server and validates it.

- **Receiving File Content:** The client receives the file content in chunks from the server and reconstructs the file.

#### Additional Explanation

- **Removing Padding Characters from Filename:**

```c
int original_filename_length = strnlen(filename, MAX_FILENAME_LENGTH + 1);
while (original_filename_length > 0 && filename[original_filename_length - 1] == 'Z') {
    filename[--original_filename_length] = '\0';
}
```
- **Handling Out-of-Order Packets:**
```c
memcpy(file_content + sequence_number * payload_size, buf + 1, numbytes - 1);
```
The starting position within the buffer is calculated based on the sequence number of the 
packet (sequence_number) and the payload size (payload_size). By using this calculation, 
the code accommodates packets that may arrive out of order and ensures that the file 
content is correctly reconstructed even in such scenarios.

### Server (ssftpd.c)

The server program (`ssftpd.c`) listens for file transfer requests from clients and responds by sending the requested files. It takes command-line arguments for the server IP and port.

#### How it Works

- **Creating Sample File:** A sample file is created for testing purposes.

- **Binding Socket:** The socket is bound to the specified port, with retries if necessary.

- **Handling Requests:** The server waits for incoming requests, receives the filename, and checks if the file exists.

- **Sending File Size:** The server sends the file size to the client and sends the file content in chunks.

- **Continued Listening:** The server continues listening for new requests in an infinite loop.

#### Additional Explanation

- **Calculating File Size (Server):**

```c
fseek(file, 0L, SEEK_END); 
unsigned long file_size = ftell(file); 
rewind(file); 
```
This code snippet calculates the size of the file by manipulating the file position indicator.

- `fseek(file, 0L, SEEK_END);` moves the indicator to the end of the file.
- `ftell(file);` returns the current position of the indicator, which represents the file size.
- `rewind(file);` resets the indicator to the beginning of the file for further operations.

---

## Signal Handling

Both the server and client side of the SSFTP implementation use signal handling to manage timeouts and retransmissions.

### Server Side

The server side uses two signal handlers: `sigpoll_handler` and `nack_handler`.

#### sigpoll_handler

The `sigpoll_handler` function is registered as a signal handler for the `SIGIO` signal. It is responsible for handling retransmit requests from clients. When a retransmit request is received, the server resends the requested packet to the client.

#### nack_handler

The `nack_handler` function is registered as a signal handler for the `SIGALRM` signal. It is responsible for changing the `transmit_mode` variable to 0 when the alarm signal is received, indicating that the server should stop transmitting.

### Client Side

The client side uses one signal handler: `handle_timeout`.

#### handle_timeout

The `handle_timeout` function is registered as a signal handler for the `SIGALRM` signal. It is responsible for handling timeouts when the client does not receive a response from the server. When a timeout occurs, the client exits with an error message.

## Packet Tracking

The client side of the SSFTPD implementation uses packet tracking to keep track of the packets that have been received and to detect missing packets.

### Initializing Packet Tracker

The `init_packet_tracker` function initializes a `PacketTracker` struct with a given status. It sets the `received` array of the struct to the specified status using the `memset` function.

### Checking Packet Status

The `packet_status` function checks if a packet is missing by looking up its sequence number in the `received` array of the `PacketTracker` struct. It returns the status of the packet (`RECEIVED` or `NOT_RECEIVED`).

### Marking Packets as Received

The `mark_packet` function marks a packet as received by setting its corresponding element in the `received` array of the `PacketTracker` struct to `RECEIVED`. It also prints a message to the console indicating that the packet has been received.

## Non-Blocking Sockets

The server side of the SSFTP implementation uses non-blocking sockets to handle multiple clients concurrently.

### Setting Non-Blocking Mode

The `set_non_blocking` function sets a socket to non-blocking mode using the `fcntl` function. It first sets the process ID to receive `SIGIO` signals for the socket using the `F_SETOWN` command. Then, it retrieves the current flags for the socket using the `F_GETFL` command. It sets the `O_NONBLOCK` flag to enable non-blocking mode and the `O_ASYNC` flag to enable asynchronous I/O. Finally, it sets the new flags for the socket using the `F_SETFL` command.



### How to Use the Makefile
Testing the application was done through macOS.
1. Compile the executables
    * `make all`
2. Running server & client (for testing on localhost)
    * `./ssftp.bin testfile 127.0.0.1 50000 127.0.0.1 1000`
    * `./ssftpd.bin 127.0.0.1 <server retured port>`
3. Cleaning unuseful files in the end
    * `make clean`
