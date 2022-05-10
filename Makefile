CXXFLAGS=-std=c++2a -O2 -Iinclude/ -Ivendor/

.DEFAULT: build/lab

build/%.o: %.cpp vendor/json.hpp vendor/rapidyaml.hpp include/profile.hpp include/scpi.hpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

build/lab: build/profile.o build/scpi.o
