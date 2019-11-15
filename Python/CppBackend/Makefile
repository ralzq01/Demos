all: libdemo.so test

libdemo.so: include/demo.h src/demo.cc
	mkdir -p lib
	g++ -Iinclude/ src/demo.cc src/c_api_wrapper.cc -shared -lpthread -o lib/libdemo.so -fPIC -std=c++11

test: test.cc include/demo.h src/demo.cc
	g++ -std=c++11 -lpthread -Iinclude/ test.cc -o test -Llib/ -ldemo