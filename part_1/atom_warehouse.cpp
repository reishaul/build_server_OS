#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <map>
#include <sstream>


// Constants
#define PORT 3333
#define BACKLOG 5//how many pending connections the queue will hold
#define BUFFER_SIZE 1024// size of the buffer for reading and writing
#define MAX_CLIENTS 25
#define MAX_CAPACITY 1000000000000000000ULL

using namespace std;


int main(int argc, char *argv[]) {
    if(argc!=2){//check if the number of arguments is correct
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }
    int port = atoi(argv[1]);//take the port number from the command line argument
    int num_clients = 0; // to count the number of clients

    map<string,unsigned long long> bank={//atoms bank structer
        {"CARBON", 0}, {"OXYGEN", 0}, {"HYDROGEN", 0}
    }; 


    int server_fd, new_socket, client_socket[MAX_CLIENTS]={0};// int of file descriptor array to hold client sockets
    struct sockaddr_in address;// struct to hold the address of the server
    socklen_t addrlen = sizeof(address);// length of the address structure
    fd_set read_fds; // set of sockets for reading
    char buffer[BUFFER_SIZE] = {0}; // buffer for reading and writing

    // create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket creation failed");
        return 1;
    }
   
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // local address
    address.sin_port = htons(port); // convert port to network byte order

    // bind the socket to the address and port
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    // listen for incoming connections
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "atom_Warehouse server is listening on port " << port << "...\n";

    // main loop to accept and handle client connections
    while (true) {

        FD_ZERO(&read_fds); // reset the set 
        FD_SET(server_fd, &read_fds); // add the server socket 
        int max_sd = server_fd; // the maximum socket

        for (int i = 0; i < num_clients; i++) {//new
            int sd = client_socket[i];
            if (sd > 0) FD_SET(sd, &read_fds);
            if (sd > max_sd){ max_sd = sd;} // update max
        }

        int activity =select(max_sd + 1, &read_fds, nullptr, nullptr, nullptr);// wait for activity on the sockets
        if (activity < 0) {//mean that there no activity on the sockets
            perror("select error");
            continue;
        }

        // check if there is activity on the server socket
        if(FD_ISSET(server_fd, &read_fds)) {
            if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
                perror("accept");
                continue;
            }
            cout << "New client connected to socket: " << new_socket << "\n";
            // If there is activity on the server socket, then there is a new client
            if(num_clients > MAX_CLIENTS) {
                std::cerr << "Maximum number of clients reached. Cannot accept new client.\n";
                close(new_socket); // close the socket if max clients reached
                break;;
            }
            //bool add = false;
            client_socket[num_clients] = new_socket; // add the new socket to the client sockets array
            num_clients++;// increment the number of clients

            cout << "Client added to the list of sockets\n";
        }

        for (int i=0;i<num_clients;i++) {//new
            if (FD_ISSET(client_socket[i], &read_fds)) {
                // If there is activity on the client socket
                new_socket = client_socket[i];   
                    
                int valread = read(new_socket, buffer, BUFFER_SIZE);// read data from the client
                if(valread <= 0) {
                    cout<<"Client disconnected from socket: " << new_socket << "\n";
                    close(new_socket);
                    client_socket[i] = client_socket[num_clients - 1]; // remove the client from the list
                    client_socket[num_clients - 1] = 0; // clear the last socket
                    num_clients--; // decrement the client count
                }
                else{// if there is data from the client
                    buffer[valread] = '\0'; 
                    cout << "Received from client: " << buffer << endl;

                    string a, type;
                    long long temp_quantity;//to hold the quantity temporarily and handle negative values
                    stringstream ss(buffer);
                    ss >> a >> type >> temp_quantity; // divide to parts
                    string response;

                    // validate input
                    if (ss.fail()||temp_quantity< 0) {// check if the input is valid
                        response = "Error: invalid quantity.\n";
                        send(new_socket, response.c_str(), response.length(), 0);
                        cerr << "Invalid quantity received from client: " << buffer << endl;
                        continue;
                    }
                    unsigned long long quantity = static_cast<unsigned long long>(temp_quantity);

                    if(a=="ADD"&&bank.count(type)){// check if the command is ADD and the atom type is valid
                        if(bank[type] + quantity <= MAX_CAPACITY) {// check if the quantity is within the maximum capacity
                            bank[type] += quantity;//add the quantity to the bank
                            string response= "Added\n";
                            send(new_socket, response.c_str(), response.length(), 0);
                        }                            
                        else {// if the quantity is over the maximum capacity
                            response = "Error: get over of maximum capacity.\n";
                            send(new_socket, response.c_str(), response.length(), 0);
                        }
                        cout<< "Current bank status:\n";//for get info of the bank
                        for (const auto& k : bank) {
                            cout << k.first << ": " << k.second << endl;
                        }
                    } 
                    else {// if the command is not like the format that we expect
                        response = "Error: invalid command or atom type, Usage: ADD <atom_type_(capital)> <quantity>\n";
                        send(new_socket, response.c_str(), response.length(), 0);//send response to client
                        cerr << "Received invalid command from client: " << buffer << endl;
                    }

                }
            }
        }
    }
    return 0;
}
