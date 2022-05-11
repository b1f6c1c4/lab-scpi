CXXFLAGS=-std=c++2b -Wno-trigraphs -O2 -Iinclude/ -Ivendor/

.DEFAULT: build/lab

build/lab: build/scpi.o build/profile.o build/executor.o build/formattor.o build/yaml.o build/main.o
	$(CXX) $(LDFLAGS) $(CFLAGS) $(CXXFLAGS) $^ -o $@

build/%.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

build/scpi.o: vendor/rapidyaml.hpp include/yaml.hpp include/scpi.hpp

build/profile.o: vendor/rapidyaml.hpp include/yaml.hpp include/profile.hpp

build/executor.o: vendor/rapidyaml.hpp include/scpi.hpp include/profile.hpp include/executor.hpp

build/formattor.o: include/formattor.hpp

build/yaml.o: vendor/rapidyaml.hpp

build/main.o: include/executor.hpp include/formattor.hpp include/profile.hpp include/scpi.hpp include/yaml.hpp
