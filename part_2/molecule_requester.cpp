
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {// Check if the number of arguments is correct
        cerr << "Usage: " << argv[0] << " <server_ip_or_hostname> <port>" << endl;
        return 1;
    }

    const char* server_host = argv[1];// Server IP or hostname
    const char* port = argv[2];// Port number

    // Set up address info for UDP
    struct addrinfo hints{}, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_DGRAM;   // UDP

    int status = getaddrinfo(server_host, port, &hints, &res);//using getaddrinfo to resolve the server address
    if (status != 0) {
        cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
        return 1;
    }

    // Create UDP socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    cout << "Connected to server at " << server_host << " on port " << port << " (UDP)\n";

    char buffer[BUFFER_SIZE];

    while (true) {
        cout << "Enter command (such as: DELIVER WATER 3 or 'exit' to quit): ";
        string input;
        getline(cin, input);

        if (input == "exit") {
            cout << "Exiting...\n";
            break;
        }

        // Send command to server
        if (sendto(sockfd, input.c_str(), input.size(), 0, res->ai_addr, res->ai_addrlen) < 0) {
            perror("sendto");
            break;
        }

        // Receive response
        sockaddr_storage server_addr;
        socklen_t addr_len = sizeof(server_addr);
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&server_addr, &addr_len);//using recvfrom to receive data from the UDP socket
        if (bytes_received < 0) {
            perror("recvfrom");
            break;
        }

        buffer[bytes_received] = '\0';// Null-terminate the buffer
        cout << "Server response: " << buffer << "\n";
    }

    close(sockfd);// Close the socket
    freeaddrinfo(res);// Free the address info structure
    cout << "Connection closed.\n";
    return 0;
}
