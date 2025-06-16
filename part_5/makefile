CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

#if we want that all the .cpp file compile
all: atom_supplier drinks_bar molecule_requester

#compile the atom_supplier, drinks_bar  and molecule_requester programs
atom_supplier: atom_supplier.cpp
	$(CXX) $(CXXFLAGS) -o atom_supplier atom_supplier.cpp

drinks_bar: drinks_bar.cpp
	$(CXX) $(CXXFLAGS) -o drinks_bar drinks_bar.cpp

molecule_requester: molecule_requester.cpp
	$(CXX) $(CXXFLAGS) -o molecule_requester molecule_requester.cpp

# run the atom_supplier, molecule_requester and drinks_bar programs
run_drinks_bar: drinks_bar
	./drinks_bar -T 4444 -U 5555 -o 100 -h 100 -c 100   --stream-path /tmp/sock_stream --datagram-path /tmp/sock_dgram

# change the port number if needed
run_atom_supplier_tcp: atom_supplier
	./atom_supplier -h 127.0.0.1 -p 4444

# run atom_supplier with UNIX domain socket
run_atom_supplier_uds: atom_supplier
	./atom_supplier -f /tmp/sock_stream

# run molecule_requester with UDP
run_molecule_requester_udp: molecule_requester
	./molecule_requester -h 127.0.0.1 -p 5555

# run molecule_requester with UNIX domain socket
run_molecule_requester_uds: molecule_requester
	./molecule_requester -f /tmp/sock_dgram

# clean up the compiled files
clean:
	rm -f atom_supplier drinks_bar molecule_requester

.PHONY: all clean run_drinks_bar run_atom_supplier_tcp run_atom_supplier_uds run_molecule_requester_udp run_molecule_requester_uds
