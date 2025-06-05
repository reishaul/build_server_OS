CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

#if we want that all the .cpp file compile
all: molecule_supplier molecule_requester atom_supplier

#compile the molecule_supplier, molecule_requester  and atom_supplier programs

molecule_supplier: molecule_supplier.cpp
	$(CXX) $(CXXFLAGS) -o molecule_supplier molecule_supplier.cpp

molecule_requester: molecule_requester.cpp
	$(CXX) $(CXXFLAGS) -o molecule_requester molecule_requester.cpp

atom_supplier: atom_supplier.cpp
	$(CXX) $(CXXFLAGS) -o atom_supplier atom_supplier.cpp


#run the molecule_supplier and molecule_requester programs

run_molecule_requester: molecule_requester
	./molecule_requester 127.0.0.1 2222

#change the port number if needed
run_molecule_supplier: molecule_supplier
	./molecule_supplier 1111 2222

run_atom_supplier: atom_supplier
	./atom_supplier 127.0.0.1 1111

#clean up the compiled files
clean:
	rm -f molecule_supplier molecule_requester atom_supplier
.PHONY: all run_molecule_requester run_molecule_supplier run_atom_supplier clean