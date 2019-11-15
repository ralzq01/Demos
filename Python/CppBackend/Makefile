all: libdemo.so test

libdemo.so: include/demo.h src/demo.cc
	mkdir -p lib
	g++ -fPIC -std=c++11 -pthread -Iinclude/ -c src/demo.cc -o lib/demo.o
	g++ -shared lib/demo.o -o lib/libdemo.so

test: test.cc include/demo.h src/demo.cc
	g++ -std=c++11 -pthread -Iinclude/ test.cc -o test -Llib/ -ldemo