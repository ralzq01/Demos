#include "aes.h"

int main(int argc, char* argv[]) {
  std::string plaintext = "This is a sample. I am a programer.\n";
	unsigned char passwd[] = "0123456789ABCDEFGHIJK";
  
  std::string ciphertext = encryption(plaintext, passwd);
  std::string deciphertext = decryption(ciphertext, passwd);
  std::cout << "plain text:    " << plaintext << std::endl;
  std::cout << "cipher text:   " << ciphertext << std::endl;
  std::cout << "decipher text: " << deciphertext << std::endl;

  return 0;
}