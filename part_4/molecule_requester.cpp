
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
    const char* host_name = nullptr;// Host name or IP address of the server
    const char* port = nullptr;// Port number

    int input;
    while((input = getopt(argc, argv, "h:p:")) != -1) {//parse the command line arguments
        switch (input) {
            case 'h': // Server host
                host_name = optarg;
                break;
            case 'p': // UDP port
                port = optarg;
                break;
            default:
                cerr << "Usage: " << argv[0] << " -h <host_name> -p <udp_port> \n";
                return 1;
        }
    }

    if (!host_name || !port) {// Check if both UDP port and host name are specified
        cerr << "Usage: " << argv[0] << " -h <host_name_or_ip> -p <port>\n";
        return 1;
    }

    // Set up address info for UDP
    struct addrinfo hints{}, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_DGRAM;   // UDP

    int status = getaddrinfo(host_name, port, &hints, &res);//using getaddrinfo to resolve the server address
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

    cout << "Connected to server at " << host_name << " on port " << port << " (UDP)\n";

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
