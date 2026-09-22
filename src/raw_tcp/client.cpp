#define __FAVOR_BSD  // Use BSD-style TCP/IP headers
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// Define IP addresses and ports for both the server and client
#define SERVER_PORT 12345
#define CLIENT_PORT 1234
#define SERVER_IP "127.0.0.1"
#define CLIENT_IP "127.0.0.1"

//---------------------------------------------------------------------
// Function: print_tcp_flags
// Steps:
// 1. Examine the TCP header flags (SYN, ACK, FIN, RST).
// 2. Convert the sequence number from network to host byte order.
// 3. Output the flag status and sequence number to the console.
//---------------------------------------------------------------------
void print_tcp_flags(struct tcphdr *tcp) {
    std::cout << "[+] TCP Flags:"
              << " SYN: " << (tcp->th_flags & TH_SYN ? 1 : 0)
              << " ACK: " << (tcp->th_flags & TH_ACK ? 1 : 0)
              << " FIN: " << (tcp->th_flags & TH_FIN ? 1 : 0)
              << " RST: " << (tcp->th_flags & TH_RST ? 1 : 0)
              << " SEQ: " << ntohl(tcp->th_seq)  // Convert seq num from network order
              << std::endl;
}

//---------------------------------------------------------------------
// Function: send_ack
// Purpose: Craft and send a TCP ACK packet to complete the handshake.
// Steps:
// 1. Create a packet buffer sized to contain both the IP and TCP headers.
// 2. Clear the packet memory using memset.
// 3. Set up pointers to the IP and TCP headers within the packet.
// 4. Convert and assign source and destination IP addresses.
// 5. Fill in the IP header fields (header length, version, total length, etc.).
// 6. Fill in the TCP header fields, including:
//    - Source/destination ports.
//    - Sequence number (preset to 600 for this demo).
//    - Acknowledgment number (using received seq number + 1).
//    - Data offset and TCP flags (ACK).
//    - Window size and other fields (checksum is not calculated).
// 7. Use sendto() to transmit the ACK packet.
// 8. Print status messages or errors.
//---------------------------------------------------------------------
void send_ack(int sock, struct sockaddr_in *server_addr, struct tcphdr *recv_tcp) {
    char packet[sizeof(struct ip) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));  // Step 2: Clear packet memory

    // Step 3: Set up pointers for IP and TCP header within the packet
    struct ip *ip = (struct ip *)packet;
    struct tcphdr *tcp_ack = (struct tcphdr *)(packet + sizeof(struct ip));

    // Step 4: Convert and assign IP addresses for client and server
    struct in_addr src_addr, dst_addr;
    inet_aton(CLIENT_IP, &src_addr);
    inet_aton(SERVER_IP, &dst_addr);

    // Step 5: Fill in the IP header fields
    ip->ip_hl = 5;                        // IP header length: 5 * 4 = 20 bytes
    ip->ip_v = 4;                         // IPv4
    ip->ip_tos = 0;                       // Type of service
    ip->ip_len = htons(sizeof(packet));   // Total packet length
    ip->ip_id = htons(54322);             // Identification field (can be random)
    ip->ip_off = 0;                       // Fragmentation offset
    ip->ip_ttl = 64;                      // Time To Live
    ip->ip_p = IPPROTO_TCP;               // Protocol: TCP
    ip->ip_src = src_addr;                // Source IP address
    ip->ip_dst = dst_addr;                // Destination IP address

    // Step 6: Fill in the TCP header fields
    tcp_ack->th_sport = htons(CLIENT_PORT);                     // Source port
    tcp_ack->th_dport = htons(SERVER_PORT);                     // Destination port
    tcp_ack->th_seq = htonl(600);                               // Predefined sequence number
    tcp_ack->th_ack = htonl(ntohl(recv_tcp->th_seq) + 1);         // ACK number (received seq + 1)
    tcp_ack->th_off = 5;                                        // TCP header length (5 * 4 = 20 bytes)
    tcp_ack->th_flags = TH_ACK;                                 // Set the ACK flag
    tcp_ack->th_win = htons(8192);                              // Window size
    tcp_ack->th_sum = 0;                                        // Checksum not calculated
    tcp_ack->th_urp = 0;                                        // Urgent pointer

    // Step 7: Send the ACK packet via the raw socket
    if (sendto(sock, packet, sizeof(packet), 0,
               (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed (ACK)");
    } else {
        // Step 8: Print success message
        std::cout << "[+] Sent ACK to complete handshake" << std::endl;
    }
}

//---------------------------------------------------------------------
// Function: send_syn
// Purpose: Craft and send a TCP SYN packet to initiate the handshake.
// Steps:
// 1. Create a packet buffer to hold the IP and TCP headers.
// 2. Clear the buffer using memset.
// 3. Set up pointers to the IP and TCP header sections.
// 4. Convert and assign the client and server IP addresses.
// 5. Fill in the IP header fields (version, header length, total length, etc.).
// 6. Fill in the TCP header fields, including:
//    - Source/destination ports.
//    - Sequence number (preset to 200).
//    - Data offset, and set the SYN flag.
// 7. Transmit the SYN packet using sendto().
// 8. Output either an error or a success message.
//---------------------------------------------------------------------
void send_syn(int sock, struct sockaddr_in *server_addr) {
    char packet[sizeof(struct ip) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));  // Step 2: Clear the buffer

    // Step 3: Set up pointers for IP and TCP headers inside the packet
    struct ip *ip = (struct ip *)packet;
    struct tcphdr *tcp_req = (struct tcphdr *)(packet + sizeof(struct ip));

    // Step 4: Convert and assign the IP addresses
    struct in_addr src_addr, dst_addr;
    inet_aton(CLIENT_IP, &src_addr);
    inet_aton(SERVER_IP, &dst_addr);

    // Step 5: Fill in the IP header fields
    ip->ip_hl = 5;                        // Header length: 20 bytes
    ip->ip_v = 4;                         // IPv4
    ip->ip_tos = 0;                       // Type of service
    ip->ip_len = htons(sizeof(packet));   // Total packet length
    ip->ip_id = htons(54321);             // Packet identifier
    ip->ip_off = 0;                       // No fragmentation
    ip->ip_ttl = 64;                      // Time To Live
    ip->ip_p = IPPROTO_TCP;               // TCP protocol
    ip->ip_src = src_addr;                // Client IP address
    ip->ip_dst = dst_addr;                // Server IP address

    // Step 6: Fill in the TCP header fields
    tcp_req->th_sport = htons(CLIENT_PORT);  // Client's source port
    tcp_req->th_dport = htons(SERVER_PORT);  // Server's destination port
    tcp_req->th_seq = htonl(200);              // Initial sequence number (preset)
    tcp_req->th_ack = 0;                     // No acknowledgment yet
    tcp_req->th_off = 5;                     // TCP header length: 20 bytes
    tcp_req->th_flags = TH_SYN;              // Set SYN flag to initiate connection
    tcp_req->th_win = htons(8192);           // Window size for flow control
    tcp_req->th_sum = 0;                     // Checksum (not calculated)
    tcp_req->th_urp = 0;                     // Urgent pointer

    // Step 7: Send the SYN packet using the raw socket
    if (sendto(sock, packet, sizeof(packet), 0,
               (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed (SYN)");
    } else {
        // Step 8: Print a success message
        std::cout << "[+] Sent SYN" << std::endl;
    }
}

//---------------------------------------------------------------------
// Function: receive_synack_and_send_ack
// Purpose: Manage the three-step TCP handshake process.
// Steps:
// 1. Create a raw socket configured for the TCP protocol.
// 2. Enable IP_HDRINCL to indicate that we’re providing our own IP header.
// 3. Set up a sockaddr_in structure with the server's IP and port.
// 4. Call send_syn() to send the SYN packet to the server.
// 5. Enter a loop to receive packets from the network using recvfrom().
// 6. For each received packet, parse the IP and TCP headers.
// 7. Filter out packets that are not from the expected source/destination.
// 8. If a packet is identified as a SYN-ACK with the expected sequence, print
//    the flags, output a success message, call send_ack() to complete the handshake,
//    and then break out of the loop.
// 9. Close the socket once the handshake is complete.
//---------------------------------------------------------------------
void receive_synack_and_send_ack() {
    // Step 1: Create a raw TCP socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Step 2: Enable IP_HDRINCL (manual IP header inclusion)
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }

    // Step 3: Configure server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_aton(SERVER_IP, &server_addr.sin_addr);

    // Step 4: Initiate handshake by sending SYN
    send_syn(sock, &server_addr);

    char buffer[65536];
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);

    // Step 5: Receive packets until the expected SYN-ACK is found
    while (true) {
        int data_size = recvfrom(sock, buffer, sizeof(buffer), 0,
                                 (struct sockaddr *)&source_addr, &addr_len);
        if (data_size < 0) {
            perror("recvfrom failed");
            continue;
        }

        // Step 6: Parse the IP header to locate the TCP header
        struct ip *ip = (struct ip *)buffer;
        struct tcphdr *tcp = (struct tcphdr *)(buffer + (ip->ip_hl * 4));

        // Step 7: Filter packets based on expected server/client ports
        if (ntohs(tcp->th_sport) != SERVER_PORT || ntohs(tcp->th_dport) != CLIENT_PORT)
            continue;

        // Debug: Print the flags of the received TCP segment
        print_tcp_flags(tcp);

        // Step 8: Check if the packet is a SYN-ACK with expected sequence value
        if ((tcp->th_flags & TH_SYN) && (tcp->th_flags & TH_ACK) && ntohl(tcp->th_seq) == 400) {
            std::cout << "[+] Received SYN-ACK from server" << std::endl;
            // Complete the handshake by sending an ACK
            send_ack(sock, &server_addr, tcp);
            break;
        }
    }

    // Step 9: Close the socket after handshake completion
    close(sock);
}

//---------------------------------------------------------------------
// Function: main
// Purpose: Application entry point; initialize the client and start the handshake.
// Steps:
// 1. Display a startup message indicating the client port.
// 2. Begin the handshake process by calling receive_synack_and_send_ack().
// 3. Terminate the program after the handshake is complete.
//---------------------------------------------------------------------
int main() {
    std::cout << "[+] Client active on port " << CLIENT_PORT << "..." << std::endl;
    receive_synack_and_send_ack();  // Manage the handshake process
    return 0;
}
