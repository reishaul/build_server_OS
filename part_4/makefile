CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

#if we want that all the .cpp file compile
all: atom_supplier drinks_bar molecule_requester

#compile the atom_supplier, drinks_bar  and molecule_requester programs

atom_supplier: atom_supplier.cpp
	$(CXX) $(CXXFLAGS) -o atom_supplier atom_supplier.cpp

drinks_bar: drinks_bar.cpp
	$(CXX) $(CXXFLAGS) -o drinks_bar drinks_bar.cpp

molecule_requester: molecule_requester.cpp
	$(CXX) $(CXXFLAGS) -o molecule_requester molecule_requester.cpp


#run the atom_supplier and molecule_requester programs

run_atom_supplier: atom_supplier
	./atom_supplier -h 127.0.0.1 -p 1111

#change the port number if needed
run_drinks_bar: drinks_bar
	./drinks_bar -t 60 -T 1111 -U 2222 -o 100 -h 100 -c 100

run_molecule_requester: molecule_requester
	./molecule_requester -h 127.0.0.1 -p 2222

#clean up the compiled files
clean:
	rm -f atom_supplier drinks_bar molecule_requester
.PHONY: all run_molecule_requester run_drinks_bar run_atom_supplier clean