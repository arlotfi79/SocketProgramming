<p align="center"><strong>CS 536 Spring 2024</strong></p>
<p align="center"><strong>Lab 6 (Bonus): Application Layer Routing [10 pts]</strong></p>

# Problem 1 (10 pts)

## 2.1 Overview

The application layer router has two components: approutes which is the router that runs on a number of lab machines (in general, machines configured with IP interfaces), and approutec which instructs approutes how to route packets. As in lab4, the data plane will use UDP, and we will use our UDP ping app to test application layer routing. The control plane uses TCP to configure approutes.

The set-up procedure for application layer routing is as follows: first run approutec on a lab machine, say, amber01

```bash
% approutec
```

where approutec will print a prompt "? " on stdout and await user input from stdin.

Second, open terminal windows to several lab machines, say, amber02, amber03, amber04, and execute approutes at each machine

```bash
% approutes router-address port-num
```

where router-address is the IP address of a lab machine (amber02, amber03, amber04) and port-num is a starting port number used to configure 3 different sockets. approutes will create a TCP socket, bind to an unused port number starting at port-num (use the incrementing method), then output the port number to stdout. approutes will create two UDP sockets, bind them to two unused port numbers starting at port-num, then output the two port numbers to stdout. The first UDP port number we will refer to as port-in, the second as port-out.

Third, at the machine where the approutec runs (amber01) enter on stdin data plane forwarding instructions for each of the application layer routers (amber02, amber03, amber04) one by one:

? router-ip tcp-port next-hop-IP next-hop-port

where router-ip is the IP address of an application layer router (e.g., 128.10.112.132 for amber02) and tcp-port is its port number (output earlier to stdout by approutec on amber02). next-hop-IP specifies the IPv4 address of the next hop router and its UDP port (port-in) to forward a UDP packet to. For example, if the next hop is amber03 then router-ip is 128.10.112.133 and port-in is the port number output to stdout by approutec running at amber03 in the first step. approutec opens a TCP connection to router-ip tcp-port and calls write() to transmit 6 bytes containing next-hop-IP and next-hop-port which instructs router-ip how to forward UDP data plane packets.

This is repeated for the all the application layer routers (amber03 and amber04). For the last application layer router (amber04), next-hop-IP specifies the IP address of host (e.g., amber07) where an app process acting as receiver runs, in our case, the UDP ping server, pings. next-hop-port specifies the UDP port number that pings awaits UDP ping packets.

## 2.2 Behavior of approutes

After printing one TCP and two UDP port numbers for the three sockets approutes has created, it listens on the TCP socket for an instruction from the configuration manager approutec. When accept() returns, approutes calls read() to read 6 bytes where the first 4 bytes represent the IP address of the next hop to forward UDP packets to and the next hop's port number. The payload that approutes forwards on the UDP data plane are UDP packets arriving on its port-in UDP port configured in the second step. We will call UDP packets arriving on port-in the forward path which are sent out using the socket bound to port-out.

When the first UDP packet arrives at port-in, approutes remembers the senders IP address and port number which will be used to forward UDP packets arriving on port-out which constitutes the reverse path. Thus the next hop IP address and port number of the reverse path are learned through the first packet arriving on the forward path. Each time a UDP packet arrives on the forward or reverse path, approutes outputs to stdout the sender's IP address (in dotted decimal form) and port number for monitoring purpose.

## 2.3 Configuring UDP ping app

After all the application layer routers have been configured, run the UDP ping app on two lab machines (e.g., amber09, amber10). For example, on amber10 run the ping server

```bash
pings 128.10.112.140 port-out
```

where port-out is the next-hop-port of last application layer router instructed by approutec in the third step. On amber09 run the ping client

```bash
pingc first-hop-ip port-in 128.10.112.139
```

where first-hop-ip is the IPv4 address of the first application layer router and port-in is the UDP port number output by the first hop router approutes for accepting data plane packets in the forward path. 128.10.112.139 is the address of amber09.

## 2.4 Testing

Place your code along with Makefile and README in subdirectory v1/. Test and verify that your application layer router works with 1, 3, 5 intermediate routers. Note the RTT values returned by pingc. Discuss your results in lab6.pdf.