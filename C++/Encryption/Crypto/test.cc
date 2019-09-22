#include "aes.h"
#include <iostream>
#include <string>

int main(){
  std::string plaintext = "Hello! World!";
  unsigned char key[] = "aaaabbbbccccdddd";
  std::cout << plaintext << std::endl;
  std::string ciphertext = encryption(plaintext, key);
  std::cout << "cipher text: " << ciphertext << std::endl;
  std::string decodetext = decryption(ciphertext, key);
  std::cout << "decryption text: " << decodetext << std::endl;
}