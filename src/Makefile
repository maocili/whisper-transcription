CXX = clang++
CXXFLAGS = -std=c++17 -Wall -I../include

test: test/test.cpp cpp_code.cc
	$(CXX) $(CXXFLAGS) test.cpp cpp_code.cc -o test

clean:
	rm -f test