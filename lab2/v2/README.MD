### Small Size File Transfer Protocol (SSFTP)

---

#### Introduction

This repository contains a simple implementation of a small size file transfer protocol (SSFTP) using datagram sockets in C. The protocol allows clients to request files from a server, which then sends the requested file in chunks to the client.

---

#### Client (ssftp.c)

The client program (`ssftp.c`) initiates file transfer requests to the server. It takes command-line arguments for the filename, server IP, server port, client IP, and payload size.

##### How it Works

- **Preparing Filename:** The filename is prepared and padded with 'Z' characters if necessary to match a fixed length.

- **Creating Socket:** A socket is created and bound for communication with the server.

- **Timeout Mechanism:** A timeout mechanism is set up to handle potential packet loss during transmission.

- **Sending Request:** The client sends a request to the server for the specified file.

- **Receiving File Size:** The client receives the file size from the server and validates it.

- **Receiving File Content:** The client receives the file content in chunks from the server and reconstructs the file.

##### Additional Explanation

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

#### Server (ssftpd.c)

The server program (`ssftpd.c`) listens for file transfer requests from clients and responds by sending the requested files. It takes command-line arguments for the server IP and port.

##### How it Works

- **Creating Sample File:** A sample file is created for testing purposes.

- **Binding Socket:** The socket is bound to the specified port, with retries if necessary.

- **Handling Requests:** The server waits for incoming requests, receives the filename, and checks if the file exists.

- **Sending File Size:** The server sends the file size to the client and sends the file content in chunks.

- **Continued Listening:** The server continues listening for new requests in an infinite loop.

##### Additional Explanation

- **Calculating File Size (Server):**

```c
// Calculate file size
fseek(file, 0L, SEEK_END); // Move the file position indicator to the end of the file
unsigned long file_size = ftell(file); // Returns the current value of the file position indicator
rewind(file); // Reset the file position indicator to the beginning of the file
```
This code snippet calculates the size of the file by manipulating the file position indicator.

- `fseek(file, 0L, SEEK_END);` moves the indicator to the end of the file.
- `ftell(file);` returns the current position of the indicator, which represents the file size.
- `rewind(file);` resets the indicator to the beginning of the file for further operations.

### How to Use the Makefile
1. Compile the executables
    * `make all`
2. Running server & client (Used my IP at the time of testing here)
    * `./ssftp.bin testfile 128.10.112.135 50001 128.10.112.133 1000`
    * `./ssftpd.bin 128.10.112.135 50001`
3. Cleaning unuseful files in the end
    * `make clean`

---

This README provides an overview of the files, functions, Makefile, and usage of the ping server-client application.
