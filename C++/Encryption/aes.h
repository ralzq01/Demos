#include <iostream>
#include <iomanip>
#include <string>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>

std::string encryption(std::string plaintext, const unsigned char* key){
  CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
  memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
  CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

  std::string ciphertext;
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(ciphertext));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plaintext.length());
  stfEncryptor.MessageEnd();

  return ciphertext;
}

std::string decryption(std::string ciphertext, const unsigned char* key) {
  CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
  memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
  CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

  std::string decryptedtext;
  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext) );
  stfDecryptor.Put( reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
  stfDecryptor.MessageEnd();

  return decryptedtext;
}