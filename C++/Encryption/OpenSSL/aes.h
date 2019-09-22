#include <memory.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <openssl/aes.h>

#pragma once

std::string aesKey(const unsigned char* key) {
  std::size_t len = strlen((char*)key);
  unsigned char* key_buff = new unsigned char[len];
  memcpy(key_buff, key, len);
  std::string aes_key((char*)key_buff);
  aes_key.resize(32, 0x90);
  delete[] key_buff;
  return aes_key;
}

std::string encryption(std::string plaintext, const unsigned char* key) {
  // set key
  AES_KEY aeskey;
  std::string aes_key = aesKey(key);
  AES_set_encrypt_key((const unsigned char*)aes_key.c_str(), 256, &aeskey);
  // set plaintext
  std::size_t block_size = 16;
  if(plaintext.size() % block_size != 0) {
    std::size_t append_size = block_size - plaintext.size() % block_size;
    plaintext.resize(plaintext.size() + append_size, 0x00);
  }
  // encryption
  unsigned char* cipher_buffer = new unsigned char[plaintext.size()];
  for(int i = 0; i < plaintext.size() / block_size; ++i) {
    AES_encrypt((unsigned char*)plaintext.c_str() + i * block_size,
                cipher_buffer + i * block_size,
                &aeskey);
  }
  std::string ciphertext = std::string((char*)cipher_buffer, plaintext.size());
  delete[] cipher_buffer;
  return ciphertext;
}

std::string decryption(std::string ciphertext, const unsigned char* key) {
  // set key
  AES_KEY aeskey;
  std::string aes_key = aesKey(key);
  AES_set_decrypt_key((unsigned char*)aes_key.c_str(), 256, &aeskey);
  // create plaintext
  std::size_t block_size = 16;
  unsigned char* plain_buffer = new unsigned char[ciphertext.size()];
  // decryption
  for(int i = 0; i < ciphertext.size() / block_size; ++i) {
    AES_decrypt((unsigned char*)ciphertext.c_str() + i * block_size,
                plain_buffer + i * block_size,
                &aeskey);
  }
  std::string plaintext((char*)plain_buffer);
  delete[] plain_buffer;
  return plaintext;
}