## Encryption and Decryption

### Prerequest

* Crypto++

* OpenSSL

### Compile and Run

* For Crypto++ AES Cryption and Decryption

```sh
$ cd Crypto
$ make
$ ./test
```

* For OpenSSL AES Cryption and Decryption

```sh
$ cd OpenSSL
$ make
$ ./test
```

The `demo` executable program shows a basic usage. The `encfile` is a tool for encryption and decryption on files.

`encfile` Usage:

```
Usage: [enc | dec] [filename] [key]
       [enc | dec] for encryption and decryption
       [filename] filefor encryption / decryption
       [key] key file for encryption or decryption
```