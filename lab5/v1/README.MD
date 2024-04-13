# Audio Streaming System 
This system consists of two main components: a server-side application responsible for streaming audio data to clients, and a client-side application responsible for receiving and playing the audio stream. Below are the details of each component:

## Server Side (audiostreams.c)
### Overview:
The server-side application implements a UDP-based audio streaming server. It listens for client connections, handles multiple client requests concurrently, and streams audio data to connected clients.

## Client Side (audiostreamc.c)
Overview:
The client-side application connects to the server, receives audio data packets, and plays the audio in real-time. It also provides feedback to the server to control the audio streaming rate based on buffer occupancy.

## Queue

### Overview:
The `queue` module provides a simple implementation of a queue data structure in C.

### Functions:

1. **`Queue* createQueue(int capacity)`**

    - **Description:** Creates a new queue with the specified capacity.
    - **Parameters:**
        - `capacity`: The maximum number of elements the queue can hold.
    - **Returns:** A pointer to the newly created queue.

2. **`void destroyQueue(Queue* queue)`**

    - **Description:** Destroys the specified queue, freeing all memory allocated to it.
    - **Parameters:**
        - `queue`: Pointer to the queue to be destroyed.

3. **`int isEmpty(Queue* queue)`**

    - **Description:** Checks if the specified queue is empty.
    - **Parameters:**
        - `queue`: Pointer to the queue.
    - **Returns:** 1 if the queue is empty, 0 otherwise.

4. **`int isFull(Queue* queue)`**

    - **Description:** Checks if the specified queue is full.
    - **Parameters:**
        - `queue`: Pointer to the queue.
    - **Returns:** 1 if the queue is full, 0 otherwise.

5. **`void display(Queue* queue)`**

    - **Description:** Displays the elements of the specified queue.
    - **Parameters:**
        - `queue`: Pointer to the queue.

6. **`void enqueue(Queue* queue, int item)`**

    - **Description:** Adds an element to the rear of the specified queue.
    - **Parameters:**
        - `queue`: Pointer to the queue.
        - `item`: The item to be enqueued.

7. **`int dequeue(Queue* queue)`**

    - **Description:** Removes and returns the element from the front of the specified queue.
    - **Parameters:**
        - `queue`: Pointer to the queue.
    - **Returns:** The dequeued item.

8. **`int front(Queue* queue)`**

    - **Description:** Returns the element at the front of the specified queue without removing it.
    - **Parameters:**
        - `queue`: Pointer to the queue.
    - **Returns:** The item at the front of the queue.

9. **`int rear(Queue* queue)`**

    - **Description:** Returns the element at the rear of the specified queue without removing it.
    - **Parameters:**
        - `queue`: Pointer to the queue.
    - **Returns:** The item at the rear of the queue.



## Concurrency Utilities

### Overview:
The `concurrency_utils` module provides utilities for handling concurrency-related tasks, such as managing semaphores and handling alarms.

### Functions:

1. **`sem_t* create_semaphore(const char* sem_name)`**

    - **Description:** Creates a semaphore with the specified name.
    - **Parameters:**
        - `sem_name`: The name of the semaphore.
    - **Returns:** A pointer to the created semaphore.
    - **Notes:**
        - If the semaphore already exists, it is unlinked and recreated.
        - The initial value of the semaphore is set to 1.

2. **`int handle_received_data(Queue* buffer, uint8_t* block, int num_bytes_received, sem_t* buffer_sem, int buffersize)`**

    - **Description:** Handles received data by enqueueing it into a buffer.
    - **Parameters:**
        - `buffer`: Pointer to the buffer (queue) where data will be enqueued.
        - `block`: Pointer to the data block received.
        - `num_bytes_received`: The number of bytes received.
        - `buffer_sem`: Semaphore for controlling access to the buffer.
        - `buffersize`: Maximum size of the buffer.
    - **Returns:** 0 on success, -1 on buffer overflow.
    - **Notes:**
        - This function ensures thread-safe access to the buffer using the provided semaphore.
        - It checks for buffer overflow before enqueuing the received data.

## Congestion Control

### Overview:
The `congestion_control` module provides functions for adjusting the sending rate of the audio streaming server based on feedback from the client, as well as for preparing and sending feedback packets to the server.

### Functions:

1. **`double adjust_sending_rate(double lambda, double epsilon, double gamma, double beta, unsigned short bufferstate)`**

    - **Description:** Adjusts the sending rate lambda based on feedback from the client buffer state.
    - **Parameters:**
        - `lambda`: Current sending rate parameter.
        - `epsilon`: Parameter for adjusting the sending rate.
        - `gamma`: Target sending rate.
        - `beta`: Parameter for adjusting the sending rate.
        - `bufferstate`: Current buffer state received from the client.
    - **Returns:** The adjusted sending rate lambda.

2. **`int prepare_feedback(int Q_t, int targetbuf, int buffersize)`**

    - **Description:** Prepares feedback packet based on the current buffer state and target buffer occupancy.
    - **Parameters:**
        - `Q_t`: Current buffer state.
        - `targetbuf`: Target buffer occupancy.
        - `buffersize`: Size of the buffer.
    - **Returns:** The feedback value to be sent to the server.

3. **`long get_file_size(FILE *file)`**

    - **Description:** Retrieves the size of the specified file.
    - **Parameters:**
        - `file`: Pointer to the file.
    - **Returns:** The size of the file in bytes.

4. **`int calculate_expected_number_of_packets(const char *filename, int blocksize)`**

    - **Description:** Calculates the expected number of packets needed to transmit the file data.
    - **Parameters:**
        - `filename`: Path to the file.
        - `blocksize`: Size of each data block.
    - **Returns:** The expected number of packets.


## Makefile

The provided Makefile automates the compilation process for the client and server programs along with their dependencies.

### How to Use the Makefile
1. Compile the executables
    * `make all`
2. Running server & client
    * `./audiostreams.bin <lambda> <epsilon> <gamma> <beta> <logfileS> <server IPv4 address> <server-port>`
    * `./audiostreamc.bin <audiofile> <blocksize> <buffersize> <targetbuf> <server-IP> <server-port> <logfileC>`
3. Cleaning unuseful files in the end
    * `make clean`

