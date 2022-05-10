CXXFLAGS=-std=c++2a -O2 -Iinclude/ -Ivendor/

.DEFAULT: build/lab

build/lab: build/scpi.o build/profile.o build/executor.o build/yaml.o
	$(CXX) $(LDFLAGS) $(CFLAGS) $(CXXFLAGS) $^ -o $@

build/%.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

build/scpi.o: vendor/rapidyaml.hpp include/yaml.hpp include/scpi.hpp

build/profile.o: vendor/rapidyaml.hpp include/yaml.hpp include/profile.hpp

build/executor.o: vendor/rapidyaml.hpp include/yaml.hpp include/scpi.hpp include/profile.hpp include/executor.hpp

build/yaml.o: vendor/rapidyaml.hpp
