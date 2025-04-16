# TCP Handshake Client

This project implements the client side of a simplified TCP three-way handshake using raw sockets. It works in tandem 
with the provided `server.cpp` to demonstrate the TCP handshake by manually constructing and parsing IP and TCP headers. 
The client includes retransmission handling for lost packets and timeouts.

## Requirements
- Linux operating system
- C++ compiler (e.g., g++)
- Root privileges (raw sockets require administrative permissions)

## Design Decisions
- **Use of Raw Sockets**: The project uses raw sockets to simulate the TCP handshake manually, bypassing the operating system's TCP stack. This allows us to have full control over the TCP headers.
- **Sequence Numbers**: The sequence numbers for the packets are hardcoded to specific values (e.g., 200 for SYN, 400 for SYN-ACK, 600 for ACK). This is done to track the flow of the handshake explicitly.
- **SYN, SYN-ACK, ACK Packets**: The server listens for SYN packets, responds with SYN-ACK packets, and waits for the final ACK to complete the handshake. This mirrors the behavior of a standard TCP three-way handshake.
- **Timeout Handling**: The client implements a simple timeout mechanism to handle cases where the SYN-ACK packet is not received. If the SYN-ACK is not received within a specified time, the client retransmits the SYN packet.

## Implementation
- **Server Side**: The server listens on a specific port (12345). It uses a raw socket to receive and respond to SYN packets, sending SYN-ACK in response and waiting for the final ACK to complete the handshake.
- **Client Side**: The client sends a SYN packet to the server, waits for the SYN-ACK, and then sends an ACK to complete the handshake.
- **Socket Setup**: Both the client and the server use raw sockets, which require root privileges to operate. They manually construct the IP and TCP headers before sending or receiving packets.


## Compilation
Open a terminal in the project directory and compile using:

    g++ -o client client.cpp

## Running the Program
Make sure the provided server program (`server.cpp`) is compiled and running.
Then run the client with root privileges:

     ./client or sudo ./client

## How It Works
1. **SYN Packet**: The client constructs an SYN packet with sequence number 200 and sends
   it to the server (listening on port 12345).

2. **SYN-ACK Packet**: The client waits to receive a SYN-ACK packet from the server.
   The expected response has:
    - A sequence number of 400.
    - An acknowledgment number of 201 (200 + 1).
    - If the response is not received within the timeout, the SYN packet is retransmitted, up to a maximum of `MAX_RETRIES` attempts.

3. **Final ACK**: Upon receiving the SYN-ACK, the client sends a final ACK packet with:
    - A sequence number of 600.
    - An acknowledgment number of 401 (server’s sequence 400 + 1).

Once the final ACK is sent, the handshake is considered complete.

## Testing
- **Basic Testing**: Run the server and client on the same machine using the loopback address (127.0.0.1). The client sends a SYN, the server responds with SYN-ACK, and the client sends an ACK to complete the handshake.
- **Network Configuration**: Ensure no firewall or network restrictions interfere with the raw socket communication. The server and client must operate on the same local network or on the same machine using loopback.
- **Edge Case Testing**: We recommend testing scenarios where the sequence numbers do not match or the packets are dropped to observe how the system handles retransmissions and timeouts.

## Restrictions in Server
- **Permissions**: Raw sockets require root or superuser privileges on most operating systems. Make sure to run both the client and the server with elevated permissions.
- **Loopback Only**: This implementation is configured to use the loopback IP address (`127.0.0.1`). To make it work on a different machine, you'll need to modify the IP addresses in both the client and the server code.

## Contribution of Each Member

- **Divyansh Verma (210360)**: developed the mechanism to send the SYN packet, handled retries for retransmissions, and ensured the correct configuration of IP and TCP headers for communication.

- **Deepika Sahu (210314)**: focused on validating the SYN-ACK response, handling errors for malformed packets, and ensuring the correct transmission of the final ACK packet to complete the handshake.

- **Satyam Gupta (218170942)**: contributed to the setup of socket communication, defining server/client IPs, ports, and configuring necessary socket options for raw packet transmission.



## Sources Referred
- **Linux Socket Programming**: https://www.geekhideout.com/socket/
- **Raw Socket Programming in C**: https://www.binarytides.com/raw-socket-programming-in-linux/
- **TCP Handshake Details**: https://en.wikipedia.org/wiki/TCP_three-way_handshake

## Declaration
We hereby declare that the code submitted is our original work and has not been plagiarized.
 
---
