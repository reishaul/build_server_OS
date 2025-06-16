//reishaul1@gmail.com
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm> // for to lower case conversion


// Constants
#define BUFFER_SIZE 1024// size of the buffer for reading and writing

using namespace std;


int main(int argc, char *argv[]) {
    if(argc!=3){//check if the number of arguments is correct
        std::cerr << "Usage: " << argv[0] << " <host_name_ or_ip_address> <port>\n";
        return 1;
    }
    const char* host_name = argv[1];
    const char* port = argv[2];//take the port number from the command line arg

    struct addrinfo hints, *res;//using addrinfo structure to hold the resolved address
    memset(&hints, 0, sizeof(hints)); // clear the hints structure
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    int status = getaddrinfo(argv[1], argv[2], &hints, &res); // resolve the host name or IP address

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