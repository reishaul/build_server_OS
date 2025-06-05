CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

#if we want that all the .cpp file compile
all: atom_warehouse atom_supplier

#compile the atom_warehouse and atom_supplier programs

atom_warehouse: atom_warehouse.cpp
	$(CXX) $(CXXFLAGS) -o atom_warehouse atom_warehouse.cpp

atom_supplier: atom_supplier.cpp
	$(CXX) $(CXXFLAGS) -o atom_supplier atom_supplier.cpp

#run the atom_warehouse and atom_supplier programs

run_warehouse: atom_warehouse
	./atom_warehouse 3333

#change the port number if needed
run_supplier: atom_supplier
	./atom_supplier 127.0.0.1 3333

#clean up the compiled files
clean:
	rm -f atom_warehouse atom_supplier
.PHONY: all run_warehouse run_supplier clean