#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/ip.h>    // For struct iphdr (Internet Protocol Header)
#include <netinet/tcp.h>   // For struct tcphdr (Transmission Control Protocol Header)
#include <arpa/inet.h>     // For inet_addr()
#include <unistd.h>
#include <sys/time.h>      // For time-related functions
#include <errno.h>

#define SERVER_PORT 12345         // Server port number, should match server's port
#define CLIENT_PORT 54321         // Arbitrary client port number
#define SERVER_IP "127.0.0.1"     // Loopback IP for the server (localhost)
#define CLIENT_IP "127.0.0.1"     // Loopback IP for the client (localhost)
#define MAX_RETRIES 3             // Maximum number of retries for sending SYN packet
#define TIMEOUT_SEC 2             // Timeout duration in seconds for retransmissions

// Function to send SYN packet to the server with sequence number 200
void send_syn(int sock, struct sockaddr_in *server_addr) {
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));  // Clear the packet buffer

    // Setup the IP header
    struct iphdr *ip = (struct iphdr *)packet;
    ip->ihl = 5;                              // 5 * 4 = 20 bytes for the IP header
    ip->version = 4;                          // IPv4
    ip->tos = 0;                              // Type of service
    ip->tot_len = htons(sizeof(packet));      // Total packet length
    ip->id = htons(12345);                    // Identification field
    ip->frag_off = 0;                         // No fragmentation
    ip->ttl = 64;                             // Time to live
    ip->protocol = IPPROTO_TCP;               // TCP protocol
    ip->saddr = inet_addr(CLIENT_IP);         // Source IP (client)
    ip->daddr = inet_addr(SERVER_IP);         // Destination IP (server)

    // Setup the TCP header
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));
    tcp->source = htons(CLIENT_PORT);         // Client source port
    tcp->dest = htons(SERVER_PORT);           // Server destination port
    tcp->seq = htonl(200);                    // Sequence number for SYN (200)
    std::cout << "Sending SYN with sequence number: " << ntohl(tcp->seq) << std::endl;
    tcp->ack_seq = 0;                         // No acknowledgment in SYN
    tcp->doff = 5;                            // Data offset (5 * 4 = 20 bytes)
    tcp->syn = 1;                             // Set SYN flag
    tcp->ack = 0;                             // ACK flag is not set for SYN
    tcp->fin = 0;                             // FIN flag not set
    tcp->rst = 0;                             // RST flag not set
    tcp->psh = 0;                             // PSH flag not set
    tcp->urg = 0;                             // URG flag not set
    tcp->window = htons(8192);                // Window size for receiving
    tcp->check = 0;                           // Checksum, kernel will calculate
    tcp->urg_ptr = 0;                         // Urgent pointer is not used

    // Send the SYN packet to the server
    if (sendto(sock, packet, sizeof(packet), 0,
               (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed for SYN packet");
    } else {
        std::cout << "[+] Sent SYN packet" << std::endl;
    }
}

// Function to receive the server's SYN-ACK response with a timeout mechanism
bool receive_syn_ack(int sock, uint32_t &server_seq) {
    char buffer[65536];  // Buffer to hold incoming packets
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;  // Set timeout to TIMEOUT_SEC
    timeout.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);  // Clear the set of file descriptors
    FD_SET(sock, &readfds);  // Add the socket to the set

    while (true) {
        // Wait for data or timeout using select()
        int result = select(sock + 1, &readfds, nullptr, nullptr, &timeout);
        if (result < 0) {
            perror("select() failed");
            continue;
        }

        if (result == 0) { // Timeout condition
            std::cout << "Timeout waiting for SYN-ACK, retransmitting..." << std::endl;
            return false;  // Return false if timed out
        }

        // Data is ready to be read
        int data_size = recvfrom(sock, buffer, sizeof(buffer), 0,
                                 (struct sockaddr *)&addr, &addr_len);
        if (data_size < 0) {
            perror("recvfrom() failed");
            continue;
        }

        // Parse the IP header
        struct iphdr *ip = (struct iphdr *)buffer;
        int ip_header_len = ip->ihl * 4;
        // Parse the TCP header (located after the IP header)
        struct tcphdr *tcp = (struct tcphdr *)(buffer + ip_header_len);

        // Check if the packet is from the server and intended for our client port
        if (ntohs(tcp->source) == SERVER_PORT && ntohs(tcp->dest) == CLIENT_PORT) {
            // Check if it's a valid SYN-ACK packet
            if (tcp->syn == 1 && tcp->ack == 1 &&
                ntohl(tcp->ack_seq) == 201 && ntohl(tcp->seq) == 400) {
                std::cout << "[+] Received SYN-ACK from server" << std::endl;
                server_seq = ntohl(tcp->seq);  // Store the server's sequence number
                return true;  // Return true if a valid SYN-ACK is received
            } else {
                std::cout << "[-] Received invalid SYN-ACK. Packet ignored." << std::endl;
            }
        }
    }
    return false;  // Return false if no valid SYN-ACK is received
}

// Function to send the final ACK packet to complete the handshake
void send_ack(int sock, struct sockaddr_in *server_addr, uint32_t server_seq) {
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));  // Clear the packet buffer

    // Build the IP header
    struct iphdr *ip = (struct iphdr *)packet;
    ip->ihl = 5;                              // IP header length (5 * 4 = 20 bytes)
    ip->version = 4;                          // IPv4
    ip->tos = 0;                              // Type of service
    ip->tot_len = htons(sizeof(packet));      // Total packet length
    ip->id = htons(54321);                    // Identification field
    ip->frag_off = 0;                         // No fragmentation
    ip->ttl = 64;                             // Time to live
    ip->protocol = IPPROTO_TCP;               // TCP protocol
    ip->saddr = inet_addr(CLIENT_IP);         // Source IP (client)
    ip->daddr = inet_addr(SERVER_IP);         // Destination IP (server)

    // Build the TCP header
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));
    tcp->source = htons(CLIENT_PORT);         // Client source port
    tcp->dest = htons(SERVER_PORT);           // Server destination port
    tcp->seq = htonl(600);                    // Final ACK packet sequence number
    tcp->ack_seq = htonl(server_seq + 1);     // Acknowledge server's SYN-ACK (400 + 1 = 401)
    tcp->doff = 5;                            // Data offset (5 * 4 = 20 bytes)
    tcp->syn = 0;                             // No SYN flag
    tcp->ack = 1;                             // Set ACK flag
    tcp->fin = 0;                             // No FIN flag
    tcp->rst = 0;                             // No RST flag
    tcp->psh = 0;                             // No PSH flag
    tcp->urg = 0;                             // No URG flag
    tcp->window = htons(8192);                // Window size for receiving
    tcp->check = 0;                           // Checksum, kernel will calculate
    tcp->urg_ptr = 0;                         // Urgent pointer is not used

    // Send the ACK packet
    if (sendto(sock, packet, sizeof(packet), 0,
               (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed for ACK packet");
    } else {
        std::cout << "[+] Sent ACK packet, handshake complete." << std::endl;
    }
}

int main() {
    // Create a raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to include the IP header
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt() failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Define the server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Step 1: Send SYN packet with retransmission handling
    int retries = 0;
    bool syn_sent = false;
    while (retries < MAX_RETRIES && !syn_sent) {
        send_syn(sock, &server_addr);

        uint32_t server_seq = 0;
        if (receive_syn_ack(sock, server_seq)) {
            syn_sent = true;  // SYN-ACK received, continue with ACK
            send_ack(sock, &server_addr, server_seq); // Send final ACK to complete handshake
        } else {
            retries++;
            std::cout << "Retry attempt " << retries << "/" << MAX_RETRIES << std::endl;
        }
    }
    // If we exhaust retries without receiving a valid SYN-ACK
    if (!syn_sent) {
        std::cerr << "Failed to complete handshake after " << MAX_RETRIES << " retries." << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }

    close(sock);
    return 0;
}
