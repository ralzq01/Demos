#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include "aes.h"

std::string readFile(std::string filename){
  std::ifstream f(filename.c_str());
  if(f.is_open()) {
    std::stringstream s;
    s << f.rdbuf();
    f.close();
    return s.str();
  }
  else{
    std::cerr << "[" << filename << "] No such a file." << std::endl;
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  if(argc != 4){
    std::cout << "Usage: [enc | dec] [filename] [key]" << std::endl
              << "\t[enc | dec] for encryption and decryption" << std::endl
              << "\t[filename] filefor encryption / decryption" << std::endl
              << "\t[key] key file for encryption or decryption" << std::endl;
    exit(0);
  }
  std::string inst = std::string(argv[1]);
  std::string filename = std::string(argv[2]);
  std::string keyfile = std::string(argv[3]);

  // read file content
  std::string content = readFile(filename);
  std::cout << content << std::endl;
  // read key file
  std::string key = readFile(keyfile);
  const unsigned char* aeskey = (unsigned char*)key.c_str();
  // choose mode
  if(inst == std::string("enc")) {
    std::string ciphertext = encryption(content, aeskey);
    std::ofstream enfile((filename + std::string(".enc")).c_str(),
                          std::ios::binary);
    if(enfile.is_open()){
      enfile << ciphertext;
    } 
    else {
      std::cerr << "Can't create encryption file." << std::endl;
      exit(-1);
    }
    enfile.close();
  } 
  else if(inst == std::string("dec")) {
    std::string plaintext = decryption(content, aeskey);
    std::ofstream defile((filename + std::string(".dec")).c_str(),
                          std::ios::binary);
    if(defile.is_open()){
      defile << plaintext;
    } 
    else {
      std::cerr << "Can't create decryption file." << std::endl;
      exit(-1);
    }
    defile.close();
  }
  else {
    std::cerr << "Unknown instruction [" << inst << "]. Should be [enc] or [dec]"
              << std::endl;
    exit(-1);
  }
  return 0;
}