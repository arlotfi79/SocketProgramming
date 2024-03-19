# Ping Server-Client Application

This is a simple client-server application for sending and receiving ping messages over UDP.


## Changes Made from IPV4 to IPV6:
* Address Structures:
  - Updated the address structures from sockaddr_in to sockaddr_in6 to support IPv6 addresses.
  - In the client and server code, replaced sockaddr_in with sockaddr_in6 for both server and client addresses.

* Socket Creation:
  - Modified socket creation to use AF_INET6 address family for IPv6 support.

* Address Conversion:
  - Appended the appropriate zone identifier (e.g., "%eth0") to the IPv6 address before conversion.

## Files and Functions

### pings.c

#### `int main(int argc, char *argv[])`
- **Description**: Main function of the server program.
- **Parameters**:
    - `argc`: Number of command-line arguments.
    - `argv[]`: Array of command-line arguments.
- **Role**:
    - Initializes server socket and binds it to a port.
    - Listens for incoming ping requests.
    - Responds to the ping requests based on the command received.

### pingc.c

#### `int main(int argc, char *argv[])`
- **Description**: Main function of the client program.
- **Parameters**:
    - `argc`: Number of command-line arguments.
    - `argv[]`: Array of command-line arguments.
- **Role**:
    - Initializes client socket and binds it to a port.
    - Sends a ping request to the server.
    - Receives the response from the server and calculates the round-trip time.

### message_utils.c

#### `void encode_packet(uint8_t *buffer, const int16_t *sequence_number, const uint8_t *command)`
- **Description**: Encodes ping packet with sequence number and command.
- **Parameters**:
    - `buffer`: Buffer to store the encoded packet.
    - `sequence_number`: Pointer to the sequence number.
    - `command`: Pointer to the command.
- **Role**: Encodes the sequence number and command into the buffer.

#### `void decode_packet(const uint8_t *buffer, int16_t *sequence_number, uint8_t *command)`
- **Description**: Decodes ping packet to extract sequence number and command.
- **Parameters**:
    - `buffer`: Buffer containing the encoded packet.
    - `sequence_number`: Pointer to store the sequence number.
    - `command`: Pointer to store the command.
- **Role**: Decodes the sequence number and command from the buffer.

### socket_utils.c

#### `int build_address(const char *const ip, const char *const port, struct addrinfo **info)`
- **Description**: Builds a socket address structure using the provided IP address and port.
- **Parameters**:
    - `ip`: IP address in string format.
    - `port`: Port number in string format.
    - `info`: Pointer to store the address information.
- **Return Value**: Returns an integer representing the status of the operation (0 for success, non-zero for failure).
- **Role**: Converts the IP address and port into a socket address structure and stores the information in the `info` pointer.

#### `int create_socket(const struct addrinfo *const info)`
- **Description**: Creates a socket using the provided address information.
- **Parameters**:
    - `info`: Address information.
- **Return Value**: Returns the socket file descriptor on success, or -1 on failure.
- **Role**: Creates a socket based on the provided address information (such as IP version and socket type).

#### `int bind_socket(const struct addrinfo *const info, const int sockfd)`
- **Description**: Binds a socket to the provided address information.
- **Parameters**:
    - `info`: Address information.
    - `sockfd`: Socket file descriptor.
- **Return Value**: Returns 0 on success, or -1 on failure.
- **Role**: Binds the socket to the address information obtained from `build_address`. This step is necessary before using the socket to send or receive data.

---

## Makefile

The provided Makefile automates the compilation process for the client and server programs along with their dependencies. 

### How to Use the Makefile
1. Compile the executables
    * `make all`
2. Running server & client (used amber09 as client and amber10 as server)
    * `./pings.bin fe80::a6bb:6dff:fe42:eead 5000`
    * `./pingc.bin fe80::a6bb:6dff:fe42:eead 5009 fe80::a6bb:6dff:fe42:e66f`
3. Cleaning unuseful files in the end
    * `make clean`