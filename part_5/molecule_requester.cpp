
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>  // for sockaddr_un


#define BUFFER_SIZE 1024

using namespace std;

int main(int argc, char *argv[]) {
    const char* host_name = nullptr;// Host name or IP address of the server
    const char* port = nullptr;// Port number
    const char* uds_path = nullptr; // for UNIX domain socket path

    int input;
    while((input = getopt(argc, argv, "h:p:f:")) != -1) {//parse the command line arguments
        switch (input) {
            case 'h': // Server host
                host_name = optarg;
                break;
            case 'p': // UDP port
                port = optarg;
                break;
            case 'f': // UNIX domain socket path
                uds_path = optarg;
                break;
            default:
                cerr << "Usage: " << argv[0] << " -h <host_name> -p <udp_port> \n";
                return 1;
        }
    }

    // Check if both host name/port and UDS path are provided
    if ((host_name || port) && uds_path) {
        cerr << "Error: Cannot specify both IP/port and UDS path\n";
        return 1;
    }
    // Check if either UDS path or host name/port is provided
    if (!(uds_path || (host_name && port))) {
        cerr << "Usage: " << argv[0] << " -h <host> -p <port>  OR  -f <UDS path>\n";
        return 1;
    }


    // if (!host_name || !port) {// Check if both UDP port and host name are specified
    //     cerr << "Usage: " << argv[0] << " -h <host_name_or_ip> -p <port>\n";
    //     return 1;
    // }

    char buffer[BUFFER_SIZE];

    if (uds_path) {
        int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
            return 1;
        }

        // נזהר ממסלול זמני בשביל לקוח
        string client_path = "/tmp/udp_client_" + to_string(getpid());

        sockaddr_un client_addr{};
        client_addr.sun_family = AF_UNIX;
        strncpy(client_addr.sun_path, client_path.c_str(), sizeof(client_addr.sun_path) - 1);

        unlink(client_path.c_str()); // למחוק אם כבר קיים

        if (bind(sockfd, (sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
            perror("bind");
            return 1;
        }

        sockaddr_un server_addr{};
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, uds_path, sizeof(server_addr.sun_path) - 1);

        cout << "Connected to UDS-DGRAM server at " << uds_path << endl;

        
        while (true) {
            cout << "Enter command (e.g. DELIVER WATER 3 or 'exit'): ";
            string input;
            getline(cin, input);

            if (input == "exit") break;

            if (sendto(sockfd, input.c_str(), input.size(), 0,
                       (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("sendto");
                break;
            }

            ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, nullptr, nullptr);
            if (bytes_received < 0) {
                perror("recvfrom");
                break;
            }

            buffer[bytes_received] = '\0';
            cout << "Server response: " << buffer << endl;
        }

        close(sockfd);
        unlink(client_path.c_str()); // Remove the temporary client socket file
    }

    else{// If host name and port are provided, use UDP sockets

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
    }
 
    cout << "Connection closed.\n";
    return 0;
}
