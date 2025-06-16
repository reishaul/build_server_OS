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
#include <vector>
#include <sstream>
#include <signal.h>
#include <getopt.h>

// Constants
#define BACKLOG 5//how many pending connections the queue will hold
#define BUFFER_SIZE 1024// size of the buffer for reading and writing
#define MAX_CLIENTS 25
#define MAX_CAPACITY 1000000000000000000ULL

using namespace std;

/*
    * Function to get a molecule from the bank.
    * It checks if the required atoms are available in sufficient quantity.
    * If available, it deducts the required atoms from the bank and returns true.
    * If not available, it returns false.
    * @param input_name: The name of the molecule to be created. 
    * @param quantity: The quantity of the molecule to be created.
    * @param bank: A map representing the available atoms in the bank.
    * @return: true if the molecule can be created, false otherwise.
*/
bool get_molecule(const string& input_name, unsigned long long quantity, map<string, unsigned long long>& bank) {
    map<string, map<string, int>> molecules = {
        {"H2O", {{"HYDROGEN", 2}, {"OXYGEN", 1}}},
        {"CO2", {{"CARBON", 1}, {"OXYGEN", 2}}},
        {"C6H12O6", {{"CARBON", 6}, {"HYDROGEN", 12}, {"OXYGEN", 6}}},
        {"C2H6O", {{"CARBON", 2}, {"HYDROGEN", 6}, {"OXYGEN", 1}}}
    };

    map<string, string> aliases = {//mapping common names to their chimical formoula
        {"WATER", "H2O"},
        {"GLUCOSE", "C6H12O6"},
        {"ETHANOL", "C2H6O"},
        {"CARBON DIOXIDE", "CO2"}
    };

    string molecule = input_name;
    if (aliases.find(input_name) != aliases.end()) {// check if the input name is an alias
        molecule = aliases[input_name];
    }

    if (molecules.find(molecule) == molecules.end()) {//if no found return false
        cerr << "Unknown molecule: " << molecule << endl;
        return false;
    }

    const auto& required_atoms = molecules[molecule];

    for (const auto& pair : required_atoms) {// check if the required atoms are available in sufficient quantity
        const string& atom = pair.first;
        unsigned long long amount = pair.second;
        unsigned long long needed = static_cast<unsigned long long>(amount) * quantity;
        if (bank[atom] < needed) {
            return false;
        }
    }

    for (const auto& pair : required_atoms) {// deduct the required atoms from the bank
        const string& atom = pair.first;
        unsigned long long amount = pair.second;
        bank[atom] -= static_cast<unsigned long long>(amount) * quantity;
    }

    return true;
}

/**
 * Function to calculate the amount of a drink that can be created based on the available atoms in the bank.
 * @param s: The name of the drink (e.g., "SOFT DRINK", "VODKA", "CHAMPAGNE").
 * @param bank: A map representing the available atoms in the bank.
 * @return: The maximum amount of the drink that can be created.
 * Returns 0 if the drink is unknown or if there are not enough atoms to create it.
 */

unsigned long long get_amount(const  string& s, const map<string, unsigned long long>& bank) {
    unsigned long long amount = 0;
    int C=0, O=0, H=0;//to hold the amount of each atom

    if(s=="SOFT DRINK"){C=7;O=9;H=14;}//the amount of each atom in the drink
    else if(s=="VODKA"){C=2;O=2;H=8;}
    else if(s=="CHAMPAGNE"){C=3;O=4;H=8;}
    else{
        cerr << "Unknown drink: " << s << endl;
        return 0;//if no found return 0
    }
    amount = min(min(bank.at("CARBON")/C, bank.at("OXYGEN")/O), bank.at("HYDROGEN")/H);//get the amount of the molecule that we can create
    return amount;
}

/* * Signal handler for timeout.
 * This function is called when the timeout is reached.
 * It prints a message and exits the program.
 * @param: The signal number (not used in this case).
*/
void timeout_handler(int) {
    cerr << "Server timed out. Exiting...\n";
    exit(1);// exit the program if the timeout is reached
}

int main(int argc, char *argv[]) {
    //first define the ports by -1 as long as they are not given by the user
    int tcp_port=-1;
    int udp_port=-1;

    long long oxygen=0, hydrogen=0, carbon=0;//to hold the amount of each atom
    int time_out=-1;//to hold the timeout value
    int input;

    //struct option is used to define the long options for getopt_long
    struct option long_options[] = {
        {"tcp-port", required_argument, nullptr, 'T'},
        {"udp-port", required_argument, nullptr, 'U'},
        {"timeout", required_argument, nullptr, 't'},
        {"oxygen", required_argument, nullptr, 'o'},
        {"hydrogen", required_argument, nullptr, 'h'},
        {"carbon", required_argument, nullptr, 'c'},
        {nullptr, 0, nullptr, 0} // end of options
    };

    
    while((input=getopt_long(argc, argv, "T:U:t:o:h:c:",long_options, nullptr))!=-1) {//parse the command line arguments

        switch(input) {
            case 'T': // TCP port
                tcp_port = atoi(optarg);
                if(tcp_port <= 0 || tcp_port > 65535) {
                    cerr << "Invalid TCP port number It should be between 1 and 65535\n";
                    return 1;
                }
                break;
            case 'U': // UDP port
                udp_port = atoi(optarg);//using atoi to convert the string to int
                if(udp_port <= 0 || udp_port > 65535) {
                    cerr << "Invalid UDP port number It should be between 1 and 65535\n";
                    return 1;
                }
                break;
            case 't': // timeout value
                time_out = atoi(optarg);
                if(time_out < 0) {// check if the timeout value is valid
                    cerr << "Invalid timeout value, it should be a non-negative integer\n";
                    return 1;
                }
                break;
            case 'o': // oxygen amount
                oxygen= atoi(optarg);
                if(oxygen < 0 ) {
                    cerr << "Invalid oxygen amount, it should be a non-negative integer\n";
                    return 1;
                }
                break;
            case 'c': // carbon amount
                carbon = atoi(optarg);
                if(carbon < 0) {
                    cerr << "Invalid carbon amount It should be a non-negative integer\n";
                    return 1;
                }
                break;
            case 'h': // hydrogen amount
                hydrogen = atoi(optarg); // using atoi to convert the string to int
                if(hydrogen < 0) { // check if the conversion was successful and if the value is non-negative
                    cerr << "Invalid hydrogen amount, it should be a non-negative integer\n";
                    return 1;
                }
                break;
            case '?': // unknown option
                cout << "Usage: " <<"program_name"<< " -T <TCP port> -U <UDP port> -t <timeout(sec)>(optional) -o <oxygen amount>(optional) -h <hydrogen amount>(optional) -c <carbon amount>(optional)\n";
                return 1;
            default:
                cout << "Usage: " <<"program_name"<< " -T <TCP port> -U <UDP port> -t <timeout(sec)>(optional) -o <oxygen amount>(optional) -h <hydrogen amount>(optional) -c <carbon amount>(optional)\n";
                return 1;
        }
    }
    if(tcp_port == -1 || udp_port == -1) {//check if the TCP and UDP ports are given
        cerr << "Error: TCP and UDP ports are required. closing the server...\n";
        return 1;
    }

    // Check if TCP and UDP ports are different
    if (tcp_port == udp_port) {
        cerr << "Error: TCP and UDP ports must be different." << endl;
        return 1;
    }
    if(time_out!=-1){
        signal(SIGALRM, timeout_handler); // set the signal handler for the timeout
        alarm(time_out); // set the timeout for the server
    }

    map<string,unsigned long long> bank={{"CARBON", 0}, {"OXYGEN", 0}, {"HYDROGEN", 0}}; //atoms bank structure

    if(oxygen!=0){bank["OXYGEN"]=oxygen;} // set the oxygen amount in the bank
    if(hydrogen!=0){bank["HYDROGEN"]=hydrogen;} // set the hydrogen amount in the bank
    if(carbon!=0){bank["CARBON"]=carbon;} // set the carbon amount in the bank


    int num_clients = 0; // to count the number of clients


    vector<string> mol_bank = {{"WATER"}, {"CARBON DIOXIDE"}, {"ALCOHOL"}, {"GLUCOSE"}};

   
    int udp_socket;// int for UDP socket
    struct sockaddr_in udp_addr;

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
   
    address.sin_family = AF_INET;// IPv4
    address.sin_addr.s_addr = INADDR_ANY; // local address
    address.sin_port = htons(tcp_port); // convert port to network byte order

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

    std::cout << "atom_Warehouse server is listening on port " << tcp_port << "...\n";

    // create UDP socket
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set up the address structure
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(udp_port); // port received from user


    // bind the UDP socket to the address and port
    if(bind(udp_socket, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind failed");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    cout << "UDP server is listening on port " << udp_port << "...\n";

    // main loop to accept and handle client connections
    while (true) {

        FD_ZERO(&read_fds); // reset the set 
        FD_SET(server_fd, &read_fds); // add the server socket 

        FD_SET(udp_socket, &read_fds); // add the UDP socket
        int max_sd = server_fd; // the maximum socket

        FD_SET(STDIN_FILENO, &read_fds);
        if (STDIN_FILENO > max_sd) max_sd = STDIN_FILENO;


        for (int i = 0; i < num_clients; i++) {// loop through the client sockets and add them to the set
            int sd = client_socket[i];
            if (sd > 0) FD_SET(sd, &read_fds);
            if (sd > max_sd){ max_sd = sd;} // update max
        }

        if (udp_socket > max_sd) {max_sd = udp_socket;} // update max if UDP socket is larger
        
        int activity =select(max_sd + 1, &read_fds, nullptr, nullptr, nullptr);// wait for activity on the sockets
        if (activity < 0) {//mean that there no activity on the sockets
            perror("select error");
            continue;
        }

        // Reset alarm if there was activity (only if timeout is set)
        if (time_out != -1 && activity > 0) {
            alarm(time_out);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));// clear the buffer
            if (fgets(buffer, sizeof(buffer), stdin)) {
                string command(buffer);
                command.erase(command.find_last_not_of(" \n\r\t")+1); // remove trailing whitespace and newline characters

                if (command == "GEN SOFT DRINK") {// check if the command is GEN SOFT DRINK
                    int amount = get_amount("SOFT DRINK", bank);
                    cout << "You can generate " << amount << " SOFT DRINK(s)." << endl;
                }
                else if (command == "GEN VODKA") {// check if the command is GEN VODKA
                    int amount = get_amount("VODKA", bank);
                    cout << "You can generate " << amount << " VODKA(s)." << endl;
                }
                else if (command == "GEN CHAMPAGNE") {// check if the command is GEN CHAMPAGNE
                    int amount = get_amount("CHAMPAGNE", bank);
                    cout << "You can generate " << amount << " CHAMPAGNE(s)." << endl;
                }
                else {
                    cout << "Unknown command: " << command << endl;
                }
            }
        }


        // check if there is activity on the server socket
        else if(FD_ISSET(server_fd, &read_fds)) {
            if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
                perror("accept");
                continue;
            }
            cout << "New client connected to socket: " << new_socket << "\n";
            
            if(num_clients > MAX_CLIENTS) {
                std::cerr << "Maximum number of clients reached. Cannot accept new client.\n";
                close(new_socket); // close the socket if max clients reached
                break;;
            }
            client_socket[num_clients] = new_socket; // add the new socket to the client sockets array
            num_clients++;// increment the number of clients
            // If there is activity on the server socket, then there is a new client

            cout << "Client added to the list of sockets.\n";
        }

        // check if there is activity on the UDP socket
        else if(FD_ISSET(udp_socket, &read_fds)) {
            char udp_buffer[BUFFER_SIZE];
            struct sockaddr_in udp_client_addr;
            socklen_t udp_addr_len = sizeof(udp_client_addr);

            int udp_valread = recvfrom(udp_socket, udp_buffer, BUFFER_SIZE, 0, (struct sockaddr*)&udp_client_addr, &udp_addr_len);//using recvfrom to receive data from the UDP socket
            if (udp_valread < 0) {
                perror("recvfrom");
                continue;
            }
            udp_buffer[udp_valread] = '\0'; // null-terminate the buffer
            cout << "Received UDP message: " << udp_buffer << "\n";

            // Parse the UDP message in a clever way
            //first we get the DELIVER command then the molecule name and after the quantity- while the molecule name can be multiple words

            string input(udp_buffer);
            istringstream iss(input);

            string a;
            iss>>a;

            if (a != "DELIVER") {
                string response = "Error: invalid command - Usage: DELIVER <molecule_name> <quantity>\n";
                sendto(udp_socket, response.c_str(), response.length(), 0,
                    (struct sockaddr*)&udp_client_addr, udp_addr_len);
                cerr << "Invalid command from UDP client: " << udp_buffer << endl;
                continue;
            }

            string rest;// to hold the rest of the line after the command
            getline(iss, rest); // get the rest of the line after the command
            istringstream rest_iss(rest);
            vector<string> tokens;// to hold the tokens of the line
            string token;

            while(rest_iss>>token) {// split the item by spaces
                tokens.push_back(token);
            }
            if(tokens.size() < 2) {// check if the item is valid
                string response = "Error: invalid molecule name or quantity - Usage: DELIVER <molecule_name> <quantity>\n";
                sendto(udp_socket, response.c_str(), response.length(), 0, (struct sockaddr*)&udp_client_addr, udp_addr_len);
                cerr << "Received invalid molecule name from UDP client: " << udp_buffer << endl;
                continue;
            }

            // The last token is the quantity, and the rest is the molecule name
            string quantity_str = tokens.back();
            tokens.pop_back();
            unsigned long long quantity;
            try {
                quantity = stoull(quantity_str);
            } catch (...) {
                string response = "Error: invalid quantity.\n";
                sendto(udp_socket, response.c_str(), response.length(), 0,
                    (struct sockaddr*)&udp_client_addr, udp_addr_len);
                cerr << "Invalid quantity from UDP client: " << udp_buffer << endl;
                continue;
            }

            string item; // the first token is the molecule name
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (i > 0) item += " ";
                item += tokens[i];
            }
            string molecule; // to hold the molecule name

            if(item=="WATER"){molecule="H2O";} // map the name to the molecule
            else if(item=="CARBON DIOXIDE"){molecule="CO2";}
            else if(item=="ALCOHOL"){molecule="C2H6O";}
            else if(item=="GLUCOSE"){molecule="C6H12O6";}
            else {
                string response = "Error: invalid molecule name. Valid names are: WATER, CARBON DIOXIDE, ALCOHOL, GLUCOSE.\n";
                sendto(udp_socket, response.c_str(), response.length(), 0, (struct sockaddr*)&udp_client_addr, udp_addr_len);
                cerr << "Received invalid molecule name from UDP client: " << udp_buffer << endl;
                continue;
            }

            bool c=get_molecule(molecule, quantity, bank);// check if we can get the molecule
            if(!c) {
                cout << "Not enough atoms to create molecule " << molecule << ".\n";
            }

            string s;
            if(c){s= "Delivered " + to_string(quantity) + " " + item + "\n";}
            else{s= "Error: not enough atoms to deliver ""\n";}
            sendto(udp_socket, s.c_str(), s.length(), 0, (struct sockaddr*)&udp_client_addr, udp_addr_len);

            cout<<"Current bank status after UDP delivery:\n";//to get info of the bank
            for (const auto& k : bank) {
                cout << k.first << ": " << k.second << endl;
            }
        }

        for (int i=0;i<num_clients;i++) {
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
                    cout << "Received from client: " << buffer << std::endl;

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
                            response = "Error: get over of maximum capacity\n";
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
    close(server_fd);
    close(udp_socket);
    return 0;
}
