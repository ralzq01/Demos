all: test encfile

test: aes.h test.cc
	g++ -o demo test.cc -lcrypto

encfile: aes.h encfile.cc
	g++ -o encfile encfile.cc -lcrypto
