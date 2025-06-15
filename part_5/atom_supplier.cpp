
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm> // for to lower case conversion

#include <sys/un.h>// for UNIX domain sockets



// Constants
#define BUFFER_SIZE 1024// size of the buffer for reading and writing

using namespace std;


int main(int argc, char *argv[]) {
    const char* host_name = nullptr;// Host name or IP address of the server
    const char* port = nullptr;// Port number
    int input;
    const char* uds_path = nullptr;//for UNIX domain socket path

    while ((input = getopt(argc, argv, "h:p:f:")) != -1) {//parse the command line arguments
        switch (input) {
            case 'h':
                host_name = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'f':// get UNIX domain socket path 
                uds_path = optarg; // path for UNIX domain socket
                break;
            default:
                cerr << "Usage: " << argv[0] << " -h <host_name_or_ip> -p <port>\n";
                return 1;
        }
    }

    if ((host_name || port) && uds_path) {// Check if both host name/port and UDS path are provided
        cerr << "Error: Cannot use both IP/port and UDS path.\n";
        return 1;
    }
    // Check if either UDS path or host name/port is provided
    if (!(uds_path || (host_name && port))) {// Check if either UDS path or host name/port is provided
        cerr << "Usage: " << argv[0] << " -h <host> -p <port>  OR  -f <uds_path>\n";
        return 1;
    }

    // if (!host_name || !port) {// Check if both host name and port are specified
    //     cerr << "Usage: " << argv[0] << " -h <host_name_or_ip> -p <port>\n";
    //     return 1;
    // }

    struct addrinfo hints, *res;//using addrinfo structure to hold the resolved address
    memset(&hints, 0, sizeof(hints)); // clear the hints structure
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    int status = getaddrinfo(host_name, port, &hints, &res); // resolve the host name or IP address

    if (status != 0) {// check if getaddrinfo was successful
        cerr << "getaddrinfo failed: " << gai_strerror(status) << endl;
        return 1;
    }

    // create a socket
    int new_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (new_socket < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    if (connect(new_socket, res->ai_addr, res->ai_addrlen) < 0) {// connect to the server
        perror("connect");
        freeaddrinfo(res);
        return 1;
    }

    freeaddrinfo(res); // free the memory
    cout << "Connected to server at " << host_name << " on port " << port << endl;

    if (uds_path) {
        int client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_socket < 0) {
            perror("socket");
            return 1;
        }

        struct sockaddr_un server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, uds_path, sizeof(server_addr.sun_path) - 1);

        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            return 1;
        }

        cout << "Connected to UDS server at " << uds_path << endl;
        new_socket = client_socket;
    }


    // main loop to read and write data 
    char buffer[BUFFER_SIZE] = {0}; // buffer for reading and writing
    while(true) {
        cout << "Enter command (or 'exit' to quit): ";
        string command;
        getline(cin, command); // read command from user input

        if (command == "exit") {
            cout << "Exiting...\n";
            break; // exit the loop if the user types 'exit'
        }

        send(new_socket, command.c_str(), command.size(), 0); // send the command to the server
        memset(buffer, 0, BUFFER_SIZE); // clear buffer before reading

        int valread = read(new_socket, buffer, BUFFER_SIZE); // read response from the server
        if (valread < 0) {
            perror("Read error");
            break;
        }
        buffer[valread] = '\0'; // null-terminate the buffer
        cout << "Server response: " << buffer << "\n"; // print the server response
    }

    close(new_socket); // close the socket
    cout << "Connection closed.\n";
    return 0; // exit the program
 
}