CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

#if we want that all the .cpp file compile
all: atom_supplier drinks_bar

#compile the atom_supplier and drinks_bar programs

atom_supplier: atom_supplier.cpp
	$(CXX) $(CXXFLAGS) -o atom_supplier atom_supplier.cpp


drinks_bar: drinks_bar.cpp
	$(CXX) $(CXXFLAGS) -o drinks_bar drinks_bar.cpp


#run the drinks_bar and atom_supplier programs
run_drinks_bar: drinks_bar
	./drinks_bar 3333 2222

#change the port number if needed
run_atom_supplier: atom_supplier
	./atom_supplier 127.0.0.1 3333

#clean up the compiled files
clean:
	rm -f atom_supplier drinks_bar
.PHONY: all run_drinks_bar run_atom_supplier clean