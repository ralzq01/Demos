all: test encfile

test: aes.cc
	g++ -o demo test.cc -lcrypto

encfile: encfile.cc
	g++ -o encfile encfile.cc -lcrypto
