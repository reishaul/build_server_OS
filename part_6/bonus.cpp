//reishaul1@gmail.com
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
#include <sys/un.h> // for UNIX domain sockets
#include <fstream> // for file operations
#include <fcntl.h> // for file control operations
#include <sys/file.h> // for file locking

#include <sys/mman.h>
#include <sys/stat.h>
#include <cerrno>

#include <thread>
#include <atomic>


// Constants
#define BACKLOG 5//how many pending connections the queue will hold
#define BUFFER_SIZE 1024// size of the buffer for reading and writing
#define MAX_CLIENTS 25
#define MAX_CAPACITY 1000000000000000000ULL
std::string save_file = ""; // to hold the save file path in string format- NEW

using namespace std;

atomic<bool> running(true); // global variable to control the server loop, atomic for thread safety

//instead of map, we use a struct to hold the inventory
struct Inventory {
    unsigned long long carbon;
    unsigned long long hydrogen;
    unsigned long long oxygen;
};



/*
    * Function to get a molecule from the bank.
    * It checks if the required atoms are available in sufficient quantity.
    * If available, it deducts the required atoms from the bank and returns true.
    * If not available, it returns false.
    * @param input_name: The name of the molecule to be created. 
    * @param quantity: The quantity of the molecule to be created.
    * @param inventory: A pointer to the Inventory structure representing the available atoms in the bank.
    * @return: true if the molecule can be created, false otherwise.
*/
bool get_molecule(const string& input_name, unsigned long long quantity, Inventory* inventory) {
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
        unsigned long long needed = static_cast<unsigned long long>(pair.second) * quantity;//using static_cast to convert the int to unsigned long long

        if (atom == "CARBON" && inventory->carbon < needed) {
            return false;
        } else if (atom == "OXYGEN" && inventory->oxygen < needed) {
            return false;
        } else if (atom == "HYDROGEN" && inventory->hydrogen < needed) {
            return false;
        }
    }

    for (const auto& pair : required_atoms) {// deduct the required atoms from the bank
        const string& atom = pair.first;
        unsigned long long used = static_cast<unsigned long long>(pair.second) * quantity;
        if (atom == "CARBON") {
            inventory->carbon -= used;
        } else if (atom == "OXYGEN") {
            inventory->oxygen -= used;
        } else if (atom == "HYDROGEN") {
            inventory->hydrogen -= used;
        }
    }

    return true;
}

/**
 * Function to calculate the amount of a drink that can be created based on the available atoms in the bank.
 * @param s: The name of the drink (e.g., "SOFT DRINK", "VODKA", "CHAMPAGNE").
 * @param inventory: A pointer to the Inventory structure representing the available atoms in the bank.
 * @return: The maximum amount of the drink that can be created.
 * Returns 0 if the drink is unknown or if there are not enough atoms to create it.
 */
unsigned long long get_amount(const  string& s, const Inventory* inventory) {
    unsigned long long amount = 0;
    int C=0, O=0, H=0;//to hold the amount of each atom

    if(s=="SOFT DRINK"){C=7;O=9;H=14;}//the amount of each atom in the drink
    else if(s=="VODKA"){C=2;O=2;H=8;}
    else if(s=="CHAMPAGNE"){C=3;O=4;H=8;}
    else{
        cerr << "Unknown drink: " << s << endl;
        return 0;//if no found return 0
    }
    amount = min(min(inventory->carbon/C, inventory->oxygen/O), inventory->hydrogen/H);//get the amount of the molecule that we can create
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


/**
 * Function to load or create an inventory from a text file.
 * If the file does not exist or is empty, it initializes the inventory with the given values.
 * @param path: The path to the text file.
 * @param inventory: A pointer to the Inventory structure representing the initial inventory.
 * @return: bool indicating success or failure.
 */
bool read_save_file(const string& path, Inventory* inv) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        // if the file does not exist, we cannot read it
        return false;
    }

    flock(fd, LOCK_SH); // Lock the file for reading

    FILE* file = fdopen(fd, "r");// convert file descriptor to FILE pointer
    if (!file) {
        close(fd);// close the file descriptor if fdopen fails
        return false;
    }
    char buffer[128];
    while(fgets(buffer, sizeof(buffer), file)) {//read each line from the file
       istringstream iss(buffer);

        string atom;
        unsigned long long amount;
        iss >> atom >> amount;// read the atom and its amount

        if (atom == "CARBON") inv->carbon = amount;
        else if (atom == "OXYGEN") inv->oxygen = amount;
        else if (atom == "HYDROGEN") inv->hydrogen = amount;
        
    }

    flock(fd, LOCK_UN);// Unlock the file after reading
    fclose(file);// close the FILE pointer
    return true;
}


/*@brief Function to write the inventory to a save file
 * It locks the file for writing, writes the inventory data, and unlocks the file.
 * @param path: The path to the save file.
 * @param inventory: A pointer to the Inventory structure representing the current inventory.
 * @return: bool indicating success or failure.

*/
bool write_save_file(const string& path, const Inventory* inventory) {

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);// open the file for writing, create it if it doesn't exist, and truncate it to zero length
     // O_WRONLY: open for writing only
     // O_CREAT: create the file if it does not exist
     // O_TRUNC: truncate the file to zero length if it already exists
     // 0644: file permissions (owner can read/write, group/others can read)
    if (fd == -1) {
        cerr << "Error opening file for writing: " << path << endl;
        return false;
    }

    flock(fd, LOCK_EX); // Lock for writing

    FILE* file = fdopen(fd, "w");// convert file descriptor to FILE pointer
    if (!file) {
        close(fd);
        return false;
    }

    fprintf(file, "CARBON %llu\n", inventory->carbon);// write the inventory data to the file
    fprintf(file, "OXYGEN %llu\n", inventory->oxygen);
    fprintf(file, "HYDROGEN %llu\n", inventory->hydrogen);

    fflush(file);// flush the output buffer to ensure all data is written to the file
    flock(fd, LOCK_UN);// Unlock the file after writing
    fclose(file);// close the FILE pointer
    return true;
}

/**
 * Function to parse the command and execute it.
 * It checks if the command is valid and if the required atoms are available in the bank.
 * If valid, it returns a success message, otherwise an error message.
 * @param com: The command string to be parsed.
 * @param inventory: A pointer to the Inventory structure representing the available atoms in the bank.
 * @return: A string containing the result of the command execution.
 */
string parse_command(const string& com, Inventory* inventory){
    istringstream iss(com);// create an input string stream from the command string
    string command;
    iss >> command;// get the command from the input string

    if (command != "DELIVER") {
        return "Error: invalid command - Usage: DELIVER <molecule_name> <quantity>\n";
    }
    // Check if the save file is provided and read it if it exists
    if (!inventory)// check if the inventory struct is initialized
    {
        return "Error: inventory not initialized\n";
    }

    string rest;// get the rest of the command after DELIVER
    getline(iss, rest);
    istringstream rest_iss(rest);// create an input string stream from the rest of the command
    vector<string> tokens;
    string token;

    while (rest_iss >> token) {// split the rest of the command into tokens
        tokens.push_back(token);// add the token to the vector
    }

    if (tokens.size() < 2) {
        return "Error: invalid molecule name or quantity - Usage: DELIVER <molecule_name> <quantity>\n";
    }

    string quantity_str = tokens.back();
    tokens.pop_back();

    unsigned long long quantity;
    try {
        quantity = stoull(quantity_str);// stoull is used to convert the string to unsigned long long
    } catch (...) {
        return "Error: invalid quantity\n";
    }

    string item;
    for (size_t i = 0; i < tokens.size(); ++i) {// get the molecule name from the tokens
        if (i > 0) item += " ";
        item += tokens[i];
    }

    string molecule;
    // map the molecule name to its chemical formula
    if (item == "WATER") molecule = "H2O";
    else if (item == "CARBON DIOXIDE") molecule = "CO2";
    else if (item == "ALCOHOL") molecule = "C2H6O";
    else if (item == "GLUCOSE") molecule = "C6H12O6";
    else {
        return "Error: invalid molecule name. Valid names are: WATER, CARBON DIOXIDE, ALCOHOL, GLUCOSE\n";
    }

    bool success = get_molecule(molecule, quantity, inventory);// check if the molecule can be created with the available atoms in the supply

    if (success) {//print the success message if the molecule can be created
        cout << "Delivered " << quantity << " " << item << ", Current bank status:\n";
        cout << " CARBON: "   << inventory->carbon   << "\n";
        cout << " HYDROGEN: " << inventory->hydrogen << "\n";
        cout << " OXYGEN: "   << inventory->oxygen   << "\n";

        if (!save_file.empty()) {//check if the save file is provided
            write_save_file(save_file, inventory);// update the inventory to the save file
        }
        return "Delivered " + to_string(quantity) + " " + item + "\n";
    } else {
        return "Error: not enough atoms to deliver\n";
    }
}

/**
 * @brief Function to load or create the inventory from a binary file
 * @param path_bin: The path to the binary file.
 * @param c: Initial amount of carbon atoms
 * @param h: Initial amount of hydrogen atoms
 * @param o: Initial amount of oxygen atoms.
 * @return: A pointer to the Inventory structure representing the current inventory.
 */
Inventory* load_or_create_inventory(const char* path_bin, unsigned long long c, unsigned long long h, unsigned long long o) {
    int fd = open(path_bin, O_RDWR | O_CREAT, 0666);// open the file for reading and writing, create it if it doesn't exist
    if (fd < 0) {
        perror("Failed to open/create inventory file");
        return nullptr;
    }

    bool need_init = false;

    // Check if the file exists or is empty (size 0)
    struct stat st;
    if (fstat(fd, &st) == -1) {//fstat is used to get the status of the file
        perror("fstat failed");
        close(fd);
        return nullptr;
    }

    if ((size_t)st.st_size < sizeof(Inventory)) {
        // If the file is new or too small, we need to initialize it
        if (ftruncate(fd, sizeof(Inventory)) == -1) {// ftruncate is used to change the size of the file
            perror("ftruncate failed");
            close(fd);
            return nullptr;
        }
        need_init = true;
    }

    void* map = mmap(nullptr, sizeof(Inventory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);// mmap is used to map the file into memory,
    // PROT_READ | PROT_WRITE means we want to read and write to the mapped memory, MAP_SHARED means changes are visible to other processes
    if (map == MAP_FAILED) {
        perror("mmap failed");
        close(fd);// close the file descriptor if mmap fails
        return nullptr;
    }

    close(fd); // close the file descriptor after mapping

    Inventory* inv = reinterpret_cast<Inventory*>(map);// cast the mapped memory to Inventory pointer
    if (need_init) {
        inv->carbon = c;// initialize the inventory with the given values
        inv->hydrogen = h;
        inv->oxygen = o;
        msync(inv, sizeof(Inventory), MS_SYNC); // write the data to disk, MS_SYNC ensures that the changes are written to the file immediately
    }

    return inv;
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
       
        {"stream-path", required_argument, nullptr, 's'},
        {"datagram-path", required_argument, nullptr, 'd'},
        // NEW: save file option
        {"save-file", required_argument, nullptr, 'f'},

        {nullptr, 0, nullptr, 0} // end of options
    };

    // Define variables for UDS paths
    string uds_stream_path, uds_dgram_path;
    // Flags to check if UDS paths are provided
    bool has_tcp = false, has_udp = false, has_uds_stream = false, has_uds_dgram = false;


    // Parse command line arguments
    while((input=getopt_long(argc, argv, "T:U:t:o:h:c:s:d:f:",long_options, nullptr))!=-1) {

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
            case 's':// UDS stream path
                uds_stream_path = optarg;
                has_uds_stream = true;
                break;
            case 'd':// UDS datagram path
                uds_dgram_path = optarg;
                has_uds_dgram = true;
                break;

            case 'f': // save file path (not used in this code, but can be implemented later)
                save_file= optarg;
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

    // Check if UDS paths are provided and validate them- NEW
    if ((has_tcp && has_uds_stream) || (has_udp && has_uds_dgram)) {
    cerr << "Error: Cannot use both TCP and UDS stream (or UDP and UDS datagram).\n";
    return 1;
    }

    //create inventory object and initialize it with the given values
    Inventory* inventory = load_or_create_inventory("inventory.bin", carbon, hydrogen, oxygen);
    if (!inventory) {
        cerr << "Failed to initialize inventory\n";
        return 1;
    }


    // Check if the save file is provided and read it if it exists
    if (!save_file.empty() && access(save_file.c_str(), F_OK) == 0) {
        cout << "Updating inventory from file: " << save_file << endl;
        if (!read_save_file(save_file, inventory)) {//check if the read_save_file function was successful
            cerr << "Error loading bank file\n";
            return 1;
        }
    } else {
        // Initialize according to the options from step 4
        inventory->oxygen = oxygen;
        inventory->hydrogen = hydrogen;
        inventory->carbon = carbon;

        // create the save file if it doesn't exist
        if (!save_file.empty()) {
            ofstream(save_file).close(); // create an empty file if it doesn't exist
            msync(inventory, sizeof(Inventory), MS_SYNC);// write the initial inventory to the save file, MS_YNC ensures that the changes are written to the file immediately
        }
    }


    int num_clients = 0; // to count the number of clients

    // Define the vector of available molecules
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

    cout << "TCP server is listening on port " << tcp_port << "...\n";

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

    // Create UNIX domain sockets if paths are provided
    int uds_fd = -1;
    struct sockaddr_un uds_stream_addr;
    if (has_uds_stream) {
        uds_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (uds_fd < 0) {
            perror("UDS stream socket creation failed");
            return 1;
        }

        memset(&uds_stream_addr, 0, sizeof(uds_stream_addr));// clear the structure
        uds_stream_addr.sun_family = AF_UNIX;// UNIX domain socket family
        strncpy(uds_stream_addr.sun_path, uds_stream_path.c_str(), sizeof(uds_stream_addr.sun_path) - 1);// copy the path to the structure

        unlink(uds_stream_path.c_str()); // Remove old file
        if (bind(uds_fd, (struct sockaddr*)&uds_stream_addr, sizeof(uds_stream_addr)) < 0) {// bind the socket to the address and path
            perror("UDS stream bind failed");
            return 1;
        }

        if (listen(uds_fd, BACKLOG) < 0) {
            perror("UDS stream listen failed");
            return 1;
        }

        cout << "UDS stream server is listening on " << uds_stream_path << "\n";
    }

    // Create UDS datagram socket if path is provided
    int uds_dgram_fd = -1;// int for UDS datagram socket
    struct sockaddr_un uds_dgram_addr;
    if (has_uds_dgram) {// check if the UDS datagram path is provided
        uds_dgram_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (uds_dgram_fd < 0) {// create the UDS datagram socket
            perror("UDS datagram socket creation failed");
            return 1;
        }

        memset(&uds_dgram_addr, 0, sizeof(uds_dgram_addr));// clear the structure
        uds_dgram_addr.sun_family = AF_UNIX;
        strncpy(uds_dgram_addr.sun_path, uds_dgram_path.c_str(), sizeof(uds_dgram_addr.sun_path) - 1);

        unlink(uds_dgram_path.c_str());// Remove old file
        if (bind(uds_dgram_fd, (struct sockaddr*)&uds_dgram_addr, sizeof(uds_dgram_addr)) < 0) {// bind the socket to the address and path
            perror("UDS datagram bind failed");
            return 1;
        }

        cout << "UDS datagram server is listening on " << uds_dgram_path << "\n";
    }
    cout<<"type \"exit\" to close the server\n";

    // main loop to accept and handle client connections
    while (running) {

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

        //before the select- checking if provided and add UDS sockets to the set if they exist
        if (has_uds_stream) {
            FD_SET(uds_fd, &read_fds); // add the UDS stream socket
            if (uds_fd > max_sd) max_sd = uds_fd; // update max if UDS stream socket is larger
        }
        if (has_uds_dgram) {
            FD_SET(uds_dgram_fd, &read_fds); // add the UDS datagram socket
            if (uds_dgram_fd > max_sd) max_sd = uds_dgram_fd; // update max if UDS datagram socket is larger
        }
        
        int activity =select(max_sd + 1, &read_fds, nullptr, nullptr, nullptr);// wait for activity on the sockets

        // Check if there was activity on the UDS stream socket
        if (has_uds_stream && FD_ISSET(uds_fd, &read_fds)) {
            int new_socket = accept(uds_fd, nullptr, nullptr);// accept a new connection on the UDS stream socket
            if (new_socket >= 0) {// check if the accept was successful
                cout << "New UDS stream client connected: " << new_socket << "\n";
                if(num_clients < MAX_CLIENTS) {
                    client_socket[num_clients++] = new_socket;// add the new socket to the client sockets array
                } else {
                    cerr << "Max clients reached. Rejecting.\n";
                    close(new_socket);// close the socket if max clients reached
                }
            }
        }
        
        // Check if there was activity on the UDS datagram socket
        if (has_uds_dgram && FD_ISSET(uds_dgram_fd, &read_fds)) {
            char uds_buffer[BUFFER_SIZE];// buffer for UDS datagram
            struct sockaddr_un client_addr;// struct to hold the client address
            socklen_t client_len = sizeof(client_addr);// length of the client address structure

            int bytes = recvfrom(uds_dgram_fd, uds_buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);// receive data from the UDS datagram socket
            if (bytes > 0) {
                uds_buffer[bytes] = '\0';
                // send response back
                string response = parse_command(uds_buffer, inventory);// parse the command and get the response
                sendto(uds_dgram_fd, response.c_str(), response.length(), 0, (struct sockaddr*)&client_addr, client_len);// send response to the client
            }
        }


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
                    int amount = get_amount("SOFT DRINK", inventory);
                    cout << "You can generate " << amount << " SOFT DRINK(s)" << endl;
                }
                else if (command == "GEN VODKA") {// check if the command is GEN VODKA
                    int amount = get_amount("VODKA", inventory);
                    cout << "You can generate " << amount << " VODKA(s)" << endl;
                }
                else if (command == "GEN CHAMPAGNE") {// check if the command is GEN CHAMPAGNE
                    int amount = get_amount("CHAMPAGNE", inventory);
                    cout << "You can generate " << amount << " CHAMPAGNE(s)" << endl;
                }
                else if(command=="exit") {//NEW
                    cout << "Exiting server...\n";
                    running = false;
                    break;
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

            cout << "Client added to the list of sockets\n";
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

            string response = parse_command(udp_buffer, inventory);

            sendto(udp_socket, response.c_str(), response.length(), 0,
                (struct sockaddr*)&udp_client_addr, udp_addr_len);
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

                    //if the save file is provided, read it first
                    // this is to ensure that the inventory is updated with the latest values from the save file
                    if (!save_file.empty()) {
                        read_save_file(save_file, inventory);// read the save file to get the latest values
                    }

                    if(a=="ADD"){// check if the command is ADD 
                        bool valid_type = false;
                        unsigned long long* ty=nullptr; // pointer to hold the type of atom
                        // check the type of atom and assign the pointer to the corresponding atom in the inventory
                        if(type == "CARBON") {
                            ty = &inventory->carbon;
                            valid_type = true;
                        } else if(type == "OXYGEN") {
                            ty = &inventory->oxygen;
                            valid_type = true;
                        } else if(type == "HYDROGEN") {
                            ty = &inventory->hydrogen;
                            valid_type = true;
                        }
                        if(valid_type && ty) {// check if the type is valid and the pointer is not null
                            
                            // check if the quantity is within the maximum capacity
                            if(*ty + quantity <= MAX_CAPACITY) {// check if the quantity is within the maximum capacity
                                *ty += quantity;//add the quantity to the bank
                                if(!save_file.empty()){// check if the save file is provided and update it
                                    write_save_file(save_file, inventory);// write the bank to the save file
                                }
                                string response= "Added\n";
                                send(new_socket, response.c_str(), response.length(), 0);

                            }                            
                            else {// if the quantity is over the maximum capacity
                                string response = "Error: get over of maximum capacity\n";
                                send(new_socket, response.c_str(), response.length(), 0);
                            }
                            //print the current bank status
                            cout << "Current bank status:\n";
                            cout << "CARBON: " << inventory->carbon << "\n";
                            cout << "HYDROGEN: " << inventory->hydrogen << "\n";
                            cout << "OXYGEN: " << inventory->oxygen << "\n";
                        }
                        else {// if the command is not like the format that we expect
                            string response = "Error: invalid command or atom type, Usage: ADD <atom_type_(capital)> <quantity>\n";
                            send(new_socket, response.c_str(), response.length(), 0);//send response to client
                        }
                    }
                }
            }
        } 
    }

    // Close all client sockets
    close(server_fd);
    close(udp_socket);

    
    if(has_uds_stream) {
        unlink(uds_stream_path.c_str()); // remove the UDS stream socket file after closing the server
    }
    if(has_uds_dgram) {
        unlink(uds_dgram_path.c_str()); // remove the UDS datagram socket file after closing the server
    }
 
    return 0;
}
