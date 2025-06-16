CXX = g++
CXXFLAGS = -Wall -std=c++17 

#if we want that all the .cpp file compile
all: drinks_bar bonus atom_supplier molecule_requester

drinks_bar: drinks_bar.cpp
	$(CXX) $(CXXFLAGS) -o drinks_bar drinks_bar.cpp

bonus: bonus.cpp
	$(CXX) $(CXXFLAGS) -o bonus bonus.cpp

#compile the atom_supplier, drinks_bar  and molecule_requester programs
atom_supplier: atom_supplier.cpp
	$(CXX) $(CXXFLAGS) -o atom_supplier atom_supplier.cpp

molecule_requester: molecule_requester.cpp
	$(CXX) $(CXXFLAGS) -o molecule_requester molecule_requester.cpp


#run the drinks_bar servers programs
run_drinks_bar1:
	./drinks_bar -T 9999 -U 8888 -s /tmp/sock_stream -d /tmp/sock_dgram -f inventory.txt -o 100 -h 100 -c 100

run_drinks_bar2:
	./drinks_bar -T 9998 -U 8887 -s /tmp/sock_stream2 -d /tmp/sock_dgram2 -f inventory.txt -o 120 -h 120 -c 120

run_drinks_bar3:
	./drinks_bar -T 9997 -U 8886 -s /tmp/sock_stream3 -d /tmp/sock_dgram3 -f inventory.txt -o 30 -h 30 -c 30

#run the drinks_bar servers programs
run_bonus1:
	./bonus -T 1111 -U 2222 -s /tmp/sock_stream -d /tmp/sock_dgram -f inventory.txt -o 100 -h 100 -c 100

run_bonus2:
	./bonus -T 1112 -U 2223 -s /tmp/sock_stream2 -d /tmp/sock_dgram2 -f inventory.txt -o 120 -h 120 -c 120

run_bonus3:
	./bonus -T 1113 -U 2224 -s /tmp/sock_stream3 -d /tmp/sock_dgram3 -f inventory.txt -o 30 -h 30 -c 30

#run the atom_supplier and molecule_requester programs
run_atom_supplier:
	./atom_supplier -f /tmp/sock_stream

run_atom_supplier2:
	./atom_supplier -f /tmp/sock_stream2

run_molecule_requester:
	./molecule_requester -f /tmp/sock_dgram

run_molecule_requester2:
	./molecule_requester -f /tmp/sock_dgram2

# clean up the compiled files
clean:
	rm -f drinks_bar bonus atom_supplier molecule_requester 
.PHONY: all clean \
	run_drinks_bar1 run_drinks_bar2 run_drinks_bar3 \
	run_bonus1 run_bonus2 run_bonus3 \
	run_atom_supplier run_molecule_requester